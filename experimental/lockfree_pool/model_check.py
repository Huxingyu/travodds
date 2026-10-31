#!/usr/bin/env python3
"""Small, bounded SC models; not a C++ weak-memory or whole DDS proof."""
import json
from collections import deque
from itertools import combinations


def stale_return(packed):
    # Two copies/replays of a generation-1 return race with reuse for generation 2.
    # Op = generation check/snapshot, then delayed mutation. Reader-2's loan is
    # never legitimately returned in this model, so its pin must remain set.
    start = (1, 1, 0, 0, False, False)  # gen, flags, A/B stage, reserved, committed
    todo = deque([(start, [])]); seen = {start}
    while todo:
        state, trace = todo.popleft()
        gen, flags, a, b, reserved, committed = state
        if committed and flags == 0:
            return {"states": len(seen), "counterexample": trace}
        moves = []
        for name, stage in [('A', a), ('B', b)]:
            if stage == 0 and gen == 1 and flags == 1:
                ns = (gen, flags, 1 if name == 'A' else a,
                      1 if name == 'B' else b, reserved, committed)
                moves.append((ns, name + ': snapshot generation=1, loan=1'))
            elif stage == 1:
                # Packed CAS compares generation AND flags at mutation. Naive
                # release only checks generation earlier, then fetch_and's bit.
                newflags = flags & ~1 if not packed or (gen, flags) == (1, 1) else flags
                ns = (gen, newflags, 2 if name == 'A' else a,
                      2 if name == 'B' else b, reserved, committed)
                moves.append((ns, name + (': CAS exact control' if packed else ': delayed clear bit')))
        if gen == 1 and flags == 0 and not reserved:
            moves.append(((2, 2, a, b, True, False), 'Writer: reserve generation=2'))
        if reserved and not committed:
            moves.append(((2, 1, a, b, True, True), 'Writer: publish generation=2, loan=1'))
        for ns, step in moves:
            if ns not in seen:
                seen.add(ns); todo.append((ns, trace + [step]))
    return {"states": len(seen), "counterexample": None}


def lifecycle():
    # Two fixed Readers. Flag layout matches the candidate's conceptual layout:
    # pending bits 0..1, loaned bits 2..3, writer bit 4, WRITING bit 5.
    P = [1, 2]; L = [4, 8]; W = 16; WRITING = 32
    # g, flags, payload generation, committed generation, Reader 0/1 stage.
    # stages 0=no loan, 1=reading, 2=last read complete but return not yet done.
    start = (0, 0, 0, 0, 0, 0)
    todo = deque([start]); seen = {start}; transitions = 0
    while todo:
        g, f, payload, committed, r0, r1 = todo.popleft()
        rs = [r0, r1]
        assert not any(rs) or payload == g == committed
        for r in range(2):
            assert not rs[r] or (f & L[r])
            assert not ((f & P[r]) and (f & L[r]))
        nxt = []
        if f == 0 and g < 2:
            assert rs == [0, 0], 'reuse while a Reader still holds storage'
            nxt.append((g + 1, WRITING | W, payload, committed, 0, 0))
        if f == (WRITING | W) and payload != g:
            assert rs == [0, 0]
            nxt.append((g, f, g, committed, r0, r1))
        if f == (WRITING | W) and payload == g:
            nxt.append((g, W | P[0] | P[1], payload, g, r0, r1))
        if not f & WRITING and f & W:
            nxt.append((g, f & ~W, payload, committed, r0, r1))
        for r in range(2):
            if not f & WRITING and f & P[r] and not rs[r]:
                nr = rs.copy(); nr[r] = 1
                nxt.append((g, (f & ~P[r]) | L[r], payload, committed, *nr))
            if rs[r] == 1:
                assert payload == g == committed, 'read sees uncommitted/overwritten payload'
                nr = rs.copy(); nr[r] = 2
                nxt.append((g, f, payload, committed, *nr))
            if rs[r] == 2:
                nr = rs.copy(); nr[r] = 0
                nxt.append((g, f & ~L[r], payload, committed, *nr))
        for state in nxt:
            transitions += 1
            if state not in seen:
                seen.add(state); todo.append(state)
    return {"states": len(seen), "transitions": transitions, "safety_violations": 0,
            "scope": "one slot, two Readers, two generations, atomic control steps under SC"}


def scan_order(gated):
    # All interleavings preserving the Writer and Reader program orders.
    # Writer commits A(1), advances watermark, commits B(2), advances watermark.
    writer = ['commit A', 'watermark A', 'commit B', 'watermark B']
    reader = ['snapshot', 'scan A', 'scan B']
    checked = 0; bad = []
    for writer_positions in combinations(range(7), 4):
        wi = ri = 0; wpos = set(writer_positions); trace = []
        for k in range(7):
            if k in wpos: trace.append(writer[wi]); wi += 1
            else: trace.append(reader[ri]); ri += 1
        slots = {'A': None, 'B': None}; wm = snapshot = 0; found = []
        for event in trace:
            action, *arg = event.split()
            if action == 'commit': slots[arg[0]] = 1 if arg[0] == 'A' else 2
            elif action == 'watermark': wm = 1 if arg[0] == 'A' else 2
            elif action == 'snapshot': snapshot = wm
            else:
                seq = slots[arg[0]]
                if seq is not None and (not gated or seq <= snapshot): found.append(seq)
        checked += 1
        if found and min(found) == 2: bad.append(trace)
    return {"interleavings": checked, "ordering_violations": len(bad),
            "first_counterexample": bad[0] if bad else None}


if __name__ == '__main__':
    result = {
        'naive_check_then_clear_stale_return': stale_return(False),
        'packed_control_CAS_stale_return': stale_return(True),
        'packed_control_lifecycle': lifecycle(),
        'ungated_scan': scan_order(False),
        'watermark_gated_scan': scan_order(True),
        'limitations': ['bounded sequential-consistency models',
                       'does not model C++ weak memory, futex, crashes, registration, mapping lifetime',
                       'no performance or formal whole-system lock-free claim']}
    assert result['naive_check_then_clear_stale_return']['counterexample']
    assert result['packed_control_CAS_stale_return']['counterexample'] is None
    assert result['ungated_scan']['ordering_violations'] > 0
    assert result['watermark_gated_scan']['ordering_violations'] == 0
    print(json.dumps(result, indent=2, ensure_ascii=False))
