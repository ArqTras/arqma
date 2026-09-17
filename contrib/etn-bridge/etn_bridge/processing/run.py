"""Processing worker — Loki-style sweep/finalize for wrap intents."""

from __future__ import annotations

import argparse
import time

from etn_bridge.bridge_core import DEMO, get_swap, list_swaps, publish_attestation, update_swap


def process_once() -> int:
    done = 0
    for swap in list_swaps(200):
        if swap.status not in ("pending_processing", "awaiting_deposit"):
            continue
        if swap.status == "awaiting_deposit" and not DEMO:
            # Production: inspect wallet-rpc / ETH watcher for deposits.
            continue
        if DEMO and swap.status == "awaiting_deposit":
            # Leave awaiting until finalize endpoint is called.
            continue
        if swap.status == "pending_processing":
            if swap.direction == "mint" and not swap.eth_txid:
                swap.eth_txid = "processed-mint-" + swap.id[:8]
            if swap.direction == "redeem" and not swap.arq_txid:
                swap.arq_txid = "processed-payout-" + swap.id[:8]
            try:
                publish_attestation(swap)
                swap.status = "completed"
            except Exception as exc:  # noqa: BLE001
                swap.status = "failed"
                print("attestation failed", swap.id, exc)
            update_swap(swap)
            done += 1
    return done


def main() -> None:
    parser = argparse.ArgumentParser(description="Arqma ETN bridge processing worker")
    parser.add_argument("--once", action="store_true", help="single pass then exit")
    parser.add_argument("--interval", type=int, default=15, help="seconds between passes")
    args = parser.parse_args()
    if args.once:
        n = process_once()
        print(f"processed={n} demo={DEMO}")
        return
    while True:
        n = process_once()
        if n:
            print(f"processed={n}")
        time.sleep(max(1, args.interval))


if __name__ == "__main__":
    main()
