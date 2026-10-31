#!/usr/bin/env python3
"""Bounded SC registration models: proof aids, not C++ weak-memory proofs.

The return model deliberately includes hypothetical replay into the SAME
historical sample generation.  The experimental controller rejects replay;
this model explains why any future replay/bit-reuse design also needs the gate.
"""
import json
from collections import deque

OPEN, CLOSING, FREE = 'open', 'closing', 'free'
NONE, PENDING, LOAN = 'none', 'pending', 'loan'


def explore(start, successors, violation):
    queue = deque([(start, [])])
    seen = {start}
    transitions = 0
    while queue:
        state, trace = queue.popleft()
        if violation(state):
            return {'states': len(seen), 'transitions': transitions,
                    'counterexample': trace}
        for new_state, action in successors(state):
            transitions += 1
            if new_state not in seen:
                seen.add(new_state)
                queue.append((new_state, trace + [action]))
    return {'states': len(seen), 'transitions': transitions,
            'counterexample': None}


def old_take_model(gated):
    # epoch, gate mode, in-flight count, Reader bit, old take stage,
    # close cleanup completed, observed identity violation.
    start = (1, OPEN, 0, PENDING, 0, False, False)

    def steps(s):
        epoch, mode, count, bit, old, cleaned, bad = s
        moves = []
        if old == 0 and epoch == 1 and mode == OPEN:
            moves.append(((epoch, mode, count + int(gated), bit, 1, cleaned, bad),
                          'old take: atomically lease epoch 1' if gated else
                          'old take: check epoch 1, pause before core take'))
        if epoch == 1 and mode == OPEN:
            moves.append(((epoch, CLOSING, count, bit, old, cleaned, bad),
                          'Writer: mark epoch 1 CLOSING, preserve in-flight count'))
        if epoch == 1 and mode == CLOSING and not cleaned and (not gated or count == 0):
            newbit = NONE if bit == PENDING else bit
            moves.append(((epoch, mode, count, newbit, old, True, bad),
                          'Writer: clear unread pending; preserve Loans'))
        if epoch == 1 and mode == CLOSING and cleaned and bit == NONE and (not gated or count == 0):
            moves.append(((epoch, FREE, count, bit, old, cleaned, bad),
                          'Writer: CAS CLOSING count=0 to FREE'))
        if epoch == 1 and mode == FREE:
            moves.append(((2, OPEN, count, PENDING, old, cleaned, bad),
                          'Writer: register epoch 2 on same bit; publish new sample'))
        if old == 1:
            # Core takes only a numeric Reader id. Without an operation lease,
            # the identity can change while the earlier check is suspended.
            taken = bit == PENDING
            moves.append(((epoch, mode, count - int(gated), LOAN if taken else bit,
                           2 if taken else 3, cleaned, bad or (taken and epoch != 1)),
                          'old take: core pending-to-Loan CAS, then release lease'))
        if old == 2 and bit == LOAN and epoch == 1:
            moves.append(((epoch, mode, count, NONE, 3, cleaned, bad),
                          'old Reader: finish using and return its epoch-1 Loan'))
        return moves

    return explore(start, steps, lambda s: s[-1])


def stale_return_model(gated):
    # Sample generation stays 7 and Writer/history pin stays held throughout.
    # Only registration epoch changes. This permits complete slot-control ABA
    # after replay to the reused registration bit.
    # epoch, mode, count, bit, stale-return stage, legitimate-return done,
    # close cleanup completed, observed identity violation.
    start = (1, OPEN, 0, LOAN, 0, False, False, False)

    def steps(s):
        epoch, mode, count, bit, stale, returned, cleaned, bad = s
        moves = []
        if stale == 0 and epoch == 1 and mode in (OPEN, CLOSING) and bit == LOAN:
            moves.append(((epoch, mode, count + int(gated), bit, 1, returned, cleaned, bad),
                          'stale return: lease epoch 1 and snapshot sample-gen7+Loan' if gated else
                          'stale return: check epoch 1 and snapshot sample-gen7+Loan, pause'))
        if not returned and epoch == 1:
            # A genuine final owner wins the release. Its short-lived gate
            # lease is represented atomically; the stale request remains paused.
            moves.append(((epoch, mode, count, NONE if bit == LOAN else bit,
                           stale, True, cleaned, bad),
                          'legitimate epoch-1 return: clear old Loan bit'))
        if epoch == 1 and mode == OPEN:
            moves.append(((epoch, CLOSING, count, bit, stale, returned, cleaned, bad),
                          'Writer: mark epoch 1 CLOSING, preserve in-flight count'))
        if epoch == 1 and mode == CLOSING and not cleaned and (not gated or count == 0):
            moves.append(((epoch, mode, count, NONE if bit == PENDING else bit,
                           stale, returned, True, bad), 'Writer: finish pending cleanup'))
        if epoch == 1 and mode == CLOSING and cleaned and bit == NONE and (not gated or count == 0):
            moves.append(((epoch, FREE, count, bit, stale, returned, cleaned, bad),
                          'Writer: CAS CLOSING count=0 to FREE'))
        if epoch == 1 and mode == FREE:
            moves.append(((2, OPEN, count, NONE, stale, returned, cleaned, bad),
                          'Writer: register epoch 2 using same Reader bit'))
        if epoch == 2 and bit == NONE:
            moves.append(((epoch, mode, count, PENDING, stale, returned, cleaned, bad),
                          'hypothetical replay: add pending on SAME sample generation 7'))
        if epoch == 2 and bit == PENDING:
            moves.append(((epoch, mode, count, LOAN, stale, returned, cleaned, bad),
                          'new Reader: take replay; full slot control equals old snapshot'))
        if stale == 1:
            # Exact slot-control CAS compares sample generation and flags, not
            # registration identity. With replay it can match an old snapshot.
            succeeds = bit == LOAN
            moves.append(((epoch, mode, count - int(gated), NONE if succeeds else bit,
                           2, returned, cleaned, bad or (succeeds and epoch != 1)),
                          'stale return: CAS old sample-gen7+Loan control, then release lease'))
        return moves

    return explore(start, steps, lambda s: s[-1])


def main():
    result = {
        'old_take_without_operation_gate': old_take_model(False),
        'old_take_with_epoch_and_inflight_gate': old_take_model(True),
        'same_sample_replay_stale_return_without_gate': stale_return_model(False),
        'same_sample_replay_stale_return_with_gate': stale_return_model(True),
        'actual_gate_requirements': [
            'epoch, OPEN/CLOSING/FREE state and count compared in one atomic gate CAS',
            'take/discard enter OPEN; returns enter matching OPEN or CLOSING',
            'close preserves count and never waits synchronously for paused participants',
            'cleanup pending only after take leases quiesce; preserve all loaned bits',
            'reuse only after zero slot pins and CAS of exact CLOSING epoch/count0 to FREE',
            'no epoch/count wrap; initialization and Writer control actions serialized',
        ],
        'scope': [
            'bounded sequential-consistency abstract models, not weak-memory proofs',
            'the actual experimental controller rejects replay; replay model is a future-design counterexample',
            'no OS, mapping, crashes, process-death recovery, or performance model',
        ],
    }
    assert result['old_take_without_operation_gate']['counterexample']
    assert result['old_take_with_epoch_and_inflight_gate']['counterexample'] is None
    assert result['same_sample_replay_stale_return_without_gate']['counterexample']
    assert result['same_sample_replay_stale_return_with_gate']['counterexample'] is None
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
