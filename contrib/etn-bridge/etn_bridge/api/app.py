"""FastAPI surface for ETN wrap bridge + static frontend."""

from __future__ import annotations

import os
from pathlib import Path

from fastapi import FastAPI, Header, HTTPException
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel, Field

from etn_bridge.bridge_core import (
    BRIDGE_TOKEN,
    DAILY_LIMIT_ATOMIC,
    DEMO,
    check_daily_limit,
    create_swap,
    get_swap,
    list_swaps,
    publish_attestation,
    swap_to_dict,
    token_ok,
    update_swap,
)

FRONTEND = Path(__file__).resolve().parents[2] / "frontend"

app = FastAPI(title="Arqma ETN Bridge", version="0.1.0")


def _require_token(
    authorization: str | None = None,
    x_etn_token: str | None = None,
) -> None:
    provided = x_etn_token
    if authorization and authorization.lower().startswith("bearer "):
        provided = authorization.split(" ", 1)[1].strip()
    if not token_ok(provided):
        raise HTTPException(401, "unauthorized")


class SwapCreate(BaseModel):
    direction: str = Field(pattern="^(mint|redeem)$")
    amount_atomic: str
    dest_eth_address: str | None = None
    dest_arq_address: str | None = None


class SwapFinalize(BaseModel):
    arq_txid: str | None = None
    eth_txid: str | None = None


@app.get("/v1/status")
def status():
    return {
        "service": "arqma-etn-bridge",
        "ok": True,
        "demo": DEMO,
        "auth_required": bool(BRIDGE_TOKEN),
        "daily_limit_atomic": DAILY_LIMIT_ATOMIC,
        "audit_url": os.environ.get("ETN_AUDIT_URL", "http://127.0.0.1:22050"),
    }


@app.post("/v1/swap")
def swap_create(
    body: SwapCreate,
    authorization: str | None = Header(default=None),
    x_etn_token: str | None = Header(default=None),
):
    _require_token(authorization, x_etn_token)
    if body.direction == "mint":
        dest = body.dest_eth_address or ""
        if not dest:
            raise HTTPException(400, "dest_eth_address required for mint")
    else:
        dest = body.dest_arq_address or ""
        if not dest:
            raise HTTPException(400, "dest_arq_address required for redeem")
    if not body.amount_atomic.isdigit() or int(body.amount_atomic) <= 0:
        raise HTTPException(400, "amount_atomic must be a positive integer string")
    limit_err = check_daily_limit(body.direction, body.amount_atomic)
    if limit_err:
        raise HTTPException(429, limit_err)
    swap = create_swap(body.direction, body.amount_atomic, dest)
    return swap_to_dict(swap)


@app.get("/v1/swap/{swap_id}")
def swap_get(swap_id: str):
    swap = get_swap(swap_id)
    if not swap:
        raise HTTPException(404, "not found")
    return swap_to_dict(swap)


@app.get("/v1/swaps")
def swaps_list(
    authorization: str | None = Header(default=None),
    x_etn_token: str | None = Header(default=None),
):
    _require_token(authorization, x_etn_token)
    return {"swaps": [swap_to_dict(s) for s in list_swaps()]}


@app.post("/v1/swap/{swap_id}/finalize")
def swap_finalize(
    swap_id: str,
    body: SwapFinalize,
    authorization: str | None = Header(default=None),
    x_etn_token: str | None = Header(default=None),
):
    _require_token(authorization, x_etn_token)
    swap = get_swap(swap_id)
    if not swap:
        raise HTTPException(404, "not found")
    if body.arq_txid:
        swap.arq_txid = body.arq_txid
    if body.eth_txid:
        swap.eth_txid = body.eth_txid
    if DEMO:
        if swap.direction == "mint" and not swap.eth_txid:
            swap.eth_txid = "demo-mint-" + swap.id.replace("-", "")[:16]
        if swap.direction == "redeem" and not swap.arq_txid:
            swap.arq_txid = "demo-payout-" + swap.id.replace("-", "")[:16]
        swap.status = "completed"
        update_swap(swap)
        try:
            attestation = publish_attestation(swap)
        except Exception as exc:  # noqa: BLE001
            swap.status = "attestation_failed"
            update_swap(swap)
            raise HTTPException(502, f"audit publish failed: {exc}") from exc
        return {"swap": swap_to_dict(swap), "attestation": attestation}
    swap.status = "pending_processing"
    update_swap(swap)
    return {"swap": swap_to_dict(swap), "note": "queued for processing worker"}


if FRONTEND.is_dir():
    app.mount("/static", StaticFiles(directory=str(FRONTEND)), name="static")

    @app.get("/")
    def index():
        return FileResponse(FRONTEND / "index.html")
