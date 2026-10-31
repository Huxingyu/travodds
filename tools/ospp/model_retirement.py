#!/usr/bin/env python3
"""Finite SC counterexample check for registration retirement (not a C++ proof).

An old closer pauses after checking references. A second closer retires the
identity; it is registered again, takes a new Loan and closes. A phase-only CAS
can now retire that new identity. Holding an operation lease throughout the
scan, consumed by the final CAS, prevents this registration ABA.
"""
from collections import deque
import json


def explore(leased):
    # phase, epoch, inflight, loans, old_closer_pc, other_closer_pc
    initial = ('closing', 1, 0, 0, 0, 0)
    queue = deque([(initial, [])])
    visited = {initial}
    while queue:
        state, trace = queue.popleft()
        phase, epoch, count, loans, old, other = state
        if phase == 'retired' and loans:
            return {'states': len(visited), 'counterexample': trace}
        moves = []
        if old == 0 and phase == 'closing' and count == 0 and loans == 0:
            moves.append(((phase, epoch, count + int(leased), loans, 1, other),
                          'old closer enters and observes zero Loans'))
        if old == 1 and phase == 'closing' and count == int(leased):
            # This is the final phase/count CAS after the earlier scan. It
            # intentionally has no epoch or second Loan check in either model.
            moves.append((('retired', epoch, 0, loans, 2, other),
                          'old closer final phase/count CAS'))
        if other == 0 and phase == 'closing' and count == 0 and loans == 0:
            moves.append((('retired', epoch, count, loans, old, 1),
                          'other closer retires the old identity'))
        if other == 1 and phase == 'retired':
            moves.append((('open', epoch + 1, 0, 0, old, 2),
                          'register the same identity in a new epoch'))
        if other == 2 and phase == 'open':
            moves.append(((phase, epoch, count, 1, old, 3),
                          'new identity acquires a Loan'))
        if other == 3 and phase == 'open':
            moves.append((('closing', epoch, count, loans, old, 4),
                          'new identity closes while its Loan remains'))
        for next_state, event in moves:
            if next_state not in visited:
                visited.add(next_state)
                queue.append((next_state, trace + [event]))
    return {'states': len(visited), 'counterexample': None}


if __name__ == '__main__':
    old, fixed = explore(False), explore(True)
    assert old['counterexample'], 'The negative model must expose registration ABA'
    assert fixed['counterexample'] is None, 'The retirement lease must prevent this ABA'
    print(json.dumps({'scope': 'finite sequentially consistent abstraction only',
                      'phase_only_retirement': old, 'leased_retirement': fixed}, indent=2))
