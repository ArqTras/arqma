#!/usr/bin/env bash
# Local ETN A+C demo: audit companion + optional bridge HTTP smoke.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/etf}"
AUDIT_BIN="${AUDIT_BIN:-$BUILD_DIR/bin/arqma-etn-audit}"
DATA_DIR="${ETN_DATA_DIR:-$HOME/.arqma/etn-audit}"
AUDIT_URL="${ETN_AUDIT_URL:-http://127.0.0.1:22050}"
BRIDGE_PORT="${ETN_BRIDGE_PORT:-8788}"
BRIDGE_URL="http://127.0.0.1:${BRIDGE_PORT}"
RUN_BRIDGE="${ETN_DEMO_BRIDGE:-1}"

if [[ ! -x "$AUDIT_BIN" ]]; then
  echo "missing $AUDIT_BIN — build with: cmake --build $BUILD_DIR --target arqma_etn_audit" >&2
  exit 1
fi

mkdir -p "$DATA_DIR"
"$AUDIT_BIN" --listen 127.0.0.1:22050 --data-dir "$DATA_DIR" --issuer-id demo-issuer &
AUDIT_PID=$!
cleanup() {
  kill "$AUDIT_PID" 2>/dev/null || true
  if [[ -n "${BRIDGE_PID:-}" ]]; then
    kill "$BRIDGE_PID" 2>/dev/null || true
  fi
}
trap cleanup EXIT
sleep 0.5

curl -sf -X POST "$AUDIT_URL/v1/etn/reserve/refresh" >/dev/null
curl -sf -X POST "$AUDIT_URL/v1/etn/reserve/liability" \
  -H 'content-type: application/json' \
  -d '{"liability_atomic":"1000"}' >/dev/null
RECON="$(curl -sf "$AUDIT_URL/v1/etn/reconcile")"
echo "audit ok: $AUDIT_URL  reconcile=$RECON"

if [[ "$RUN_BRIDGE" == "1" ]] && command -v uvicorn >/dev/null 2>&1; then
  export ETN_DEMO=1 ETN_AUDIT_URL="$AUDIT_URL" PYTHONPATH="$ROOT/contrib/etn-bridge${PYTHONPATH:+:$PYTHONPATH}"
  (cd "$ROOT/contrib/etn-bridge" && uvicorn etn_bridge.api.app:app --host 127.0.0.1 --port "$BRIDGE_PORT") &
  BRIDGE_PID=$!
  sleep 1
  STATUS="$(curl -sf "$BRIDGE_URL/v1/status")"
  SWAP="$(curl -sf -X POST "$BRIDGE_URL/v1/swap" \
    -H 'content-type: application/json' \
    -d '{"direction":"mint","dest_eth_address":"0xdemo","amount_atomic":"1000000000"}')"
  SWAP_ID="$(python3 -c 'import json,sys; print(json.load(sys.stdin)["id"])' <<<"$SWAP")"
  FINAL="$(curl -sf -X POST "$BRIDGE_URL/v1/swap/${SWAP_ID}/finalize" \
    -H 'content-type: application/json' \
    -d '{"arq_txid":"demo-burn-txid"}')"
  echo "bridge ok: $BRIDGE_URL  status=$STATUS"
  echo "swap=$SWAP_ID finalize_keys=$(python3 -c 'import json,sys; d=json.load(sys.stdin); print(",".join(d.keys()))' <<<"$FINAL")"
  echo "UI: $BRIDGE_URL/"
  if [[ "${ETN_DEMO_WAIT:-0}" == "1" ]]; then
    wait
  fi
else
  echo "start bridge: cd $ROOT/contrib/etn-bridge && ETN_DEMO=1 ETN_AUDIT_URL=$AUDIT_URL uvicorn etn_bridge.api.app:app --port $BRIDGE_PORT"
  if [[ "${ETN_DEMO_WAIT:-1}" == "1" ]]; then
    wait
  fi
fi
