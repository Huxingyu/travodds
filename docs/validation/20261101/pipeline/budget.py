#!/usr/bin/env python3
"""Reserve bounded Modal attempts before launch; settle from returned evidence."""
from pathlib import Path
import argparse
import datetime
import json

ROOT = Path(__file__).resolve().parent.parent

def main():
    p = argparse.ArgumentParser()
    p.add_argument('action', choices=['reserve', 'settle'])
    p.add_argument('label')
    p.add_argument('--seconds', type=float, required=True)
    p.add_argument('--bytes', type=int, default=0)
    a = p.parse_args()
    path = ROOT / 'usage-ledger.json'
    plan = json.loads((ROOT / 'resource-plan.json').read_text())
    ledger = json.loads(path.read_text()) if path.exists() else {'entries': [], 'not_invoice': True}
    if a.action == 'reserve':
        assert not any(r['label'] == a.label for r in ledger['entries']), 'Attempt label already reserved'
        assert a.seconds > 0
        entry = {'label': a.label, 'reserved_seconds': a.seconds, 'settled': False}
        prospective = ledger['entries'] + [entry]
    else:
        entry = next(r for r in ledger['entries'] if r['label'] == a.label)
        assert not entry['settled'] and 0 <= a.seconds <= entry['reserved_seconds']
        entry.update(settled=True, observed_seconds=a.seconds, downloaded_bytes=a.bytes)
        prospective = ledger['entries']
    total_seconds = sum(r['observed_seconds'] if r['settled'] else r['reserved_seconds'] for r in prospective)
    transfer = sum(r.get('downloaded_bytes', 0) for r in prospective) / 2**30 * .04
    conservative = total_seconds * plan['provisional_conservative_rate_usd_per_s'] + transfer + plan['image_and_transfer_reserve_usd']
    assert conservative <= plan['budget_usd'], 'Stop before launch: conservative estimate exceeds authorized USD1'
    ledger['entries'] = prospective
    ledger.update(updated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        total_observed_or_reserved_seconds=total_seconds,
        requested_allocation_plus_egress_usd=round(total_seconds * plan['requested_allocation_rate_usd_per_second'] + transfer, 6),
        conservative_including_reserve_usd=round(conservative, 6), budget_usd=plan['budget_usd'])
    path.write_text(json.dumps(ledger, indent=2) + '\n')
    print(json.dumps(ledger, indent=2))

if __name__ == '__main__':
    main()
