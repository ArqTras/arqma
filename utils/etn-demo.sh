#!/usr/bin/env bash
# Local ETN A+C demo: expects arqma-etn-audit on PATH or BUILD_DIR.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/build/etf}"
AUDIT_BIN="${AUDIT_BIN:-$BUILD_DIR/bin/arqma-etn-audit}"
DATA_DIR="${ETN_DATA_DIR:-$HOME/.arqma/etn-audit}"
AUDIT_URL="${ETN_AUDIT_URL:-http://127.0.0.1:22050}"

if [[ ! -x "$AUDIT_BIN" ]]; then
  echo "missing $AUDIT_BIN — build with: cmake --build $BUILD_DIR --target arqma_etn_audit" >&2
  exit 1
fi

mkdir -p "$DATA_DIR"
"$AUDIT_BIN" --listen 127.0.0.1:22050 --data-dir "$DATA_DIR" &
AUDIT_PID=$!
trap 'kill $AUDIT_PID 2>/dev/null || true' EXIT
sleep 0.5
curl -sf -X POST "$AUDIT_URL/v1/etn/reserve/refresh" >/dev/null
echo "audit ok: $AUDIT_URL  (pid $AUDIT_PID)"
echo "start bridge: cd $ROOT/contrib/etn-bridge && ETN_DEMO=1 ETN_AUDIT_URL=$AUDIT_URL uvicorn etn_bridge.api.app:app --port 8788"
wait
