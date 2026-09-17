"""Shared persistence and audit client for the ETN wrap bridge."""

from __future__ import annotations

import json
import os
import sqlite3
import time
import uuid
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Optional

import httpx

DEFAULT_DB = Path(os.environ.get("ETN_BRIDGE_DB", str(Path.home() / ".arqma" / "etn-bridge" / "swaps.db")))
AUDIT_URL = os.environ.get("ETN_AUDIT_URL", "http://127.0.0.1:22050").rstrip("/")
DEMO = os.environ.get("ETN_DEMO", "1") not in ("0", "false", "False")


@dataclass
class Swap:
    id: str
    direction: str  # mint | redeem
    amount_atomic: str
    dest_address: str
    deposit_hint: str
    status: str
    arq_txid: str = ""
    eth_txid: str = ""
    created_at: float = 0.0
    updated_at: float = 0.0


def connect(db_path: Path = DEFAULT_DB) -> sqlite3.Connection:
    db_path.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    conn.execute(
        """
        CREATE TABLE IF NOT EXISTS swaps (
          id TEXT PRIMARY KEY,
          direction TEXT NOT NULL,
          amount_atomic TEXT NOT NULL,
          dest_address TEXT NOT NULL,
          deposit_hint TEXT NOT NULL,
          status TEXT NOT NULL,
          arq_txid TEXT,
          eth_txid TEXT,
          created_at REAL,
          updated_at REAL
        )
        """
    )
    conn.commit()
    return conn


def create_swap(direction: str, amount_atomic: str, dest_address: str) -> Swap:
    now = time.time()
    swap_id = str(uuid.uuid4())
    if direction == "mint":
        deposit_hint = f"demo-arq-deposit:{swap_id}" if DEMO else f"issuer-arq-subaddress-for:{swap_id}"
    else:
        deposit_hint = f"demo-eth-deposit:{swap_id}" if DEMO else f"warq-bridge-contract:{swap_id}"
    swap = Swap(
        id=swap_id,
        direction=direction,
        amount_atomic=amount_atomic,
        dest_address=dest_address,
        deposit_hint=deposit_hint,
        status="awaiting_deposit",
        created_at=now,
        updated_at=now,
    )
    with connect() as conn:
        conn.execute(
            "INSERT INTO swaps VALUES (?,?,?,?,?,?,?,?,?,?)",
            (
                swap.id,
                swap.direction,
                swap.amount_atomic,
                swap.dest_address,
                swap.deposit_hint,
                swap.status,
                swap.arq_txid,
                swap.eth_txid,
                swap.created_at,
                swap.updated_at,
            ),
        )
        conn.commit()
    return swap


def get_swap(swap_id: str) -> Optional[Swap]:
    with connect() as conn:
        row = conn.execute("SELECT * FROM swaps WHERE id=?", (swap_id,)).fetchone()
    if not row:
        return None
    return Swap(**{k: row[k] for k in row.keys()})


def list_swaps(limit: int = 50) -> list[Swap]:
    with connect() as conn:
        rows = conn.execute("SELECT * FROM swaps ORDER BY created_at DESC LIMIT ?", (limit,)).fetchall()
    return [Swap(**{k: r[k] for k in r.keys()}) for r in rows]


def update_swap(swap: Swap) -> None:
    swap.updated_at = time.time()
    with connect() as conn:
        conn.execute(
            """
            UPDATE swaps SET status=?, arq_txid=?, eth_txid=?, updated_at=? WHERE id=?
            """,
            (swap.status, swap.arq_txid, swap.eth_txid, swap.updated_at, swap.id),
        )
        conn.commit()


def publish_attestation(swap: Swap) -> dict[str, Any]:
    """Publish burn↔mint attestation to arqma-etn-audit (model C → A store)."""
    if swap.direction == "mint":
        arq_txid = swap.arq_txid or ("demo-burn-" + swap.id[:8])
        eth_txid = swap.eth_txid or ("demo-mint-" + swap.id[:8])
    else:
        eth_txid = swap.eth_txid or ("demo-burn-eth-" + swap.id[:8])
        arq_txid = swap.arq_txid or ("demo-payout-" + swap.id[:8])
    payload = {
        "direction": swap.direction,
        "arq_txid": arq_txid,
        "arq_amount_atomic": swap.amount_atomic,
        "eth_txid": eth_txid,
        "warq_amount_atomic": swap.amount_atomic,
        "eth_chain_id": int(os.environ.get("ETN_ETH_CHAIN_ID", "11155111")),
        "arq_height": 0,
        "issuer_pubkey": os.environ.get("ETN_ISSUER_PUBKEY", "demo-issuer-pubkey"),
        "signature": os.environ.get("ETN_ISSUER_SIG", "demo-issuer-signature"),
    }
    with httpx.Client(timeout=10.0) as client:
        r = client.post(f"{AUDIT_URL}/v1/etn/attestations", json=payload)
        r.raise_for_status()
        return r.json()


def swap_to_dict(swap: Swap) -> dict[str, Any]:
    d = asdict(swap)
    d["demo"] = DEMO
    return d
