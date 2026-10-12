#!/usr/bin/env python3
"""Independent order book reconstruction for the ITCH integration test.

Rebuilds aggregated books (price -> quantity, order count) from an ITCH 5.0
BinaryFILE with plain dicts, sharing no code or data structures with the C++
implementation, and writes snapshots at fixed times of day:

  checkpoint <time_ns> <live_orders> <active_books> <digest>
  level <time_ns> <stock_locate> <symbol> <B|S> <rank> <price> <quantity> <orders>

A snapshot at time T reflects every message with a timestamp before T. The
digest covers the full depth of all books (see Digest below); level lines list
the top levels of the most active books for readable failures.

Usage: python3 -I reference.py <itch_file> <output_file>
"""

import collections
import mmap
import struct
import sys

HOUR = 3_600_000_000_000
MINUTE = 60_000_000_000
END_OF_FILE = 2**64 - 1
CHECKPOINTS = [
    9 * HOUR + 30 * MINUTE,  # open
    11 * HOUR,
    13 * HOUR,
    15 * HOUR,
    16 * HOUR,  # close
    END_OF_FILE,
]
DETAIL_BOOKS = 10
DETAIL_LEVELS = 5

MASK = 2**64 - 1
DIGEST_SEED = 0xCBF29CE484222325
DIGEST_PRIME = 0x100000001B3


def Digest(books):
    """Hashes every non-empty book, in stock locate order, as the word sequence

      locate, #bid levels, (price, qty, orders)... best first,
              #ask levels, (price, qty, orders)... best first

    with h = (h ^ word) * prime mod 2^64 per word.
    """
    h = DIGEST_SEED
    for locate in sorted({locate for locate, _ in books}):
        bids = books.get((locate, 'B'), {})
        asks = books.get((locate, 'S'), {})
        if not bids and not asks:
            continue
        words = [locate, len(bids)]
        for price in sorted(bids, reverse=True):
            words += [price, *bids[price]]
        words.append(len(asks))
        for price in sorted(asks):
            words += [price, *asks[price]]
        for word in words:
            h = ((h ^ word) * DIGEST_PRIME) & MASK
    return h


def TopLevels(levels, side, n):
    prices = sorted(levels, reverse=(side == 'B'))[:n]
    return [(price, *levels[price]) for price in prices]


def main(itch_path, output_path):
    orders = {}  # order id -> [locate, side, price, quantity]
    books = collections.defaultdict(dict)  # (locate, side) -> {price: [qty, n]}
    symbols = {}
    adds = collections.Counter()
    snapshots = []
    unknown_refs = 0

    def AddLevel(locate, side, price, quantity):
        level = books[(locate, side)].setdefault(price, [0, 0])
        level[0] += quantity
        level[1] += 1

    def ReduceLevel(order, quantity, remove):
        levels = books[(order[0], order[1])]
        level = levels[order[2]]
        level[0] -= quantity
        if remove:
            level[1] -= 1
        if level[1] == 0:
            assert level[0] == 0, order
            del levels[order[2]]

    def TakeSnapshot(time):
        active = len({locate for (locate, _), levels in books.items() if levels})
        tops = {}
        for (locate, side), levels in books.items():
            if levels:
                tops[(locate, side)] = TopLevels(levels, side, DETAIL_LEVELS)
        snapshots.append((time, len(orders), active, Digest(books), tops))

    unpack = struct.unpack_from
    checkpoint = 0
    with open(itch_path, 'rb') as f, mmap.mmap(
            f.fileno(), 0, access=mmap.ACCESS_READ) as buf:
        size = len(buf)
        offset = 0
        while offset + 2 <= size:
            (length,) = unpack('>H', buf, offset)
            offset += 2
            if length == 0 or offset + length > size:
                sys.exit(f'truncated message at offset {offset}')
            msg = offset
            offset += length
            hi, lo = unpack('>HI', buf, msg + 5)
            while (hi << 32 | lo) >= CHECKPOINTS[checkpoint]:
                TakeSnapshot(CHECKPOINTS[checkpoint])
                checkpoint += 1

            kind = buf[msg]
            if kind not in b'RAFECXDU':
                continue  # does not change any book

            (locate,) = unpack('>H', buf, msg + 1)
            if kind == ord('A') or kind == ord('F'):
                order_id, side, quantity, price = unpack('>QcI8xI', buf, msg + 11)
                side = side.decode()
                assert order_id not in orders, order_id
                assert quantity > 0, order_id
                orders[order_id] = [locate, side, price, quantity]
                AddLevel(locate, side, price, quantity)
                adds[locate] += 1
            elif kind == ord('R'):
                assert not books.get((locate, 'B')) and not books.get((locate, 'S'))
                symbols[locate] = bytes(buf[msg + 11:msg + 19]).decode().rstrip(' ')
            else:
                (order_id,) = unpack('>Q', buf, msg + 11)
                order = orders.get(order_id)
                if order is None:
                    unknown_refs += 1
                    continue
                assert order[0] == locate, order_id
                if kind == ord('D'):
                    ReduceLevel(order, order[3], remove=True)
                    del orders[order_id]
                elif kind == ord('U'):
                    new_id, quantity, price = unpack('>QII', buf, msg + 19)
                    ReduceLevel(order, order[3], remove=True)
                    del orders[order_id]
                    assert new_id not in orders, new_id
                    assert quantity > 0, new_id
                    orders[new_id] = [locate, order[1], price, quantity]
                    AddLevel(locate, order[1], price, quantity)
                else:  # E, C, X
                    (quantity,) = unpack('>I', buf, msg + 19)
                    assert quantity <= order[3], order_id
                    order[3] -= quantity
                    ReduceLevel(order, quantity, remove=order[3] == 0)
                    if order[3] == 0:
                        del orders[order_id]

    while checkpoint < len(CHECKPOINTS):
        TakeSnapshot(CHECKPOINTS[checkpoint])
        checkpoint += 1

    detail = sorted(adds, key=lambda locate: (-adds[locate], locate))
    detail = detail[:DETAIL_BOOKS]
    with open(output_path, 'w') as out:
        out.write(f'# Generated by test/integration/reference.py from '
                  f'{itch_path.rsplit("/", 1)[-1]}\n')
        out.write(f'# unknown order references: {unknown_refs}\n')
        for time, live, active, digest, tops in snapshots:
            out.write(f'checkpoint {time} {live} {active} {digest:016x}\n')
            for locate in detail:
                for side in 'BS':
                    for rank, (price, qty, n) in enumerate(
                            tops.get((locate, side), [])):
                        out.write(f'level {time} {locate} {symbols[locate]} '
                                  f'{side} {rank} {price} {qty} {n}\n')
    print(f'{len(snapshots)} checkpoints, unknown order references: '
          f'{unknown_refs}', file=sys.stderr)


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
