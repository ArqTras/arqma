"""Processing worker — Loki-style sweep/finalize for wrap intents."""

from __future__ import annotations

import argparse
import os
import time

from etn_bridge.bridge_core import DEMO, get_swap, list_swaps, publish_attestation, update_swap
from etn_bridge.bridge_core.wallet_rpc import WALLET_RPC_URL, find_incoming_tx


def sweep_deposits() -> int:
    """Attach ARQ deposit txids to awaiting mint swaps when wallet-rpc is configured."""
    if DEMO or not WALLET_RPC_URL:
        return 0
    n = 0
    for swap in list_swaps(200):
        if swap.status != "awaiting_deposit" or swap.direction != "mint":
            continue
        if swap.arq_txid:
            continue
        try:
            hit = find_incoming_tx(int(swap.amount_atomic), payment_id_hint=swap.id[:8])
        except Exception as exc:  # noqa: BLE001
            print("sweep error", swap.id, exc)
            continue
        if not hit or not hit.get("txid"):
            continue
        swap.arq_txid = hit["txid"]
        swap.status = "pending_processing"
        update_swap(swap)
        n += 1
    return n


def process_once() -> int:
    swept = sweep_deposits()
    done = 0
    for swap in list_swaps(200):
        if swap.status != "pending_processing":
            continue
        if swap.direction == "mint" and not swap.eth_txid:
            if DEMO:
                swap.eth_txid = "processed-mint-" + swap.id[:8]
            else:
                # Production: call ETH bridge mintWithBurnAttestation via operator key.
                swap.eth_txid = os.environ.get("ETN_FORCE_ETH_TXID", "")
                if not swap.eth_txid:
                    print("mint waiting for ETH operator mint", swap.id)
                    continue
        if swap.direction == "redeem" and not swap.arq_txid:
            if DEMO:
                swap.arq_txid = "processed-payout-" + swap.id[:8]
            else:
                swap.arq_txid = os.environ.get("ETN_FORCE_ARQ_TXID", "")
                if not swap.arq_txid:
                    print("redeem waiting for ARQ payout", swap.id)
                    continue
        try:
            publish_attestation(swap)
            swap.status = "completed"
        except Exception as exc:  # noqa: BLE001
            swap.status = "failed"
            print("attestation failed", swap.id, exc)
        update_swap(swap)
        done += 1
    return swept + done


def main() -> None:
    parser = argparse.ArgumentParser(description="Arqma ETN bridge processing worker")
    parser.add_argument("--once", action="store_true", help="single pass then exit")
    parser.add_argument("--interval", type=int, default=15, help="seconds between passes")
    args = parser.parse_args()
    if args.once:
        n = process_once()
        print(f"processed={n} demo={DEMO} wallet_rpc={bool(WALLET_RPC_URL)}")
        return
    while True:
        n = process_once()
        if n:
            print(f"processed={n}")
        time.sleep(max(1, args.interval))


if __name__ == "__main__":
    main()
