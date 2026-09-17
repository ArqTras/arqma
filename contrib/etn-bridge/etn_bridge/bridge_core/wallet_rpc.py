"""Wallet-rpc helpers for deposit sweep (model C mint)."""

from __future__ import annotations

import os
from typing import Any

import httpx

WALLET_RPC_URL = os.environ.get("ETN_WALLET_RPC_URL", "").rstrip("/")


def _url() -> str:
    if not WALLET_RPC_URL:
        return ""
    if WALLET_RPC_URL.endswith("/json_rpc"):
        return WALLET_RPC_URL
    return WALLET_RPC_URL + "/json_rpc"


def wallet_rpc(method: str, params: dict[str, Any] | None = None) -> dict[str, Any]:
    url = _url()
    if not url:
        raise RuntimeError("ETN_WALLET_RPC_URL not set")
    payload = {"jsonrpc": "2.0", "id": "0", "method": method, "params": params or {}}
    with httpx.Client(timeout=30.0) as client:
        r = client.post(url, json=payload)
        r.raise_for_status()
        data = r.json()
    if "error" in data and data["error"]:
        raise RuntimeError(str(data["error"]))
    return data.get("result") or {}


def find_incoming_tx(
    min_amount: int,
    payment_id_hint: str = "",
    address: str = "",
) -> dict[str, Any] | None:
    """Best-effort: scan recent incoming transfers for amount / address match.

    Prefer dedicated subaddresses per swap (create_deposit_subaddress).
    """
    result = wallet_rpc("get_transfers", {"in": True, "pending": True})
    incoming = list(result.get("in") or []) + list(result.get("pending") or [])
    for tx in incoming:
        amount = int(tx.get("amount") or 0)
        if amount < min_amount:
            continue
        txid = tx.get("txid") or tx.get("tx_hash") or ""
        tx_addr = str(tx.get("address") or "")
        if address and tx_addr and address != tx_addr:
            continue
        if payment_id_hint and not address:
            if payment_id_hint not in str(tx.get("payment_id", "")) and payment_id_hint not in txid:
                continue
        return {"txid": txid, "amount": amount, "height": tx.get("height") or 0, "address": tx_addr}
    return None


def create_deposit_subaddress(label: str) -> dict[str, Any]:
    """Allocate a fresh subaddress for one mint deposit (Loki-style)."""
    result = wallet_rpc("create_address", {"account_index": 0, "label": label})
    address = result.get("address") or ""
    if not address:
        raise RuntimeError("create_address returned no address")
    return {
        "address": address,
        "address_index": result.get("address_index"),
        "label": label,
    }
