#!/usr/bin/env bash
# Start companion processes for a local Arqma stack (same repo, separate PIDs).
set -euo pipefail

BIN_DIR="${ARQMA_BIN_DIR:-$(cd "$(dirname "$0")/.." && pwd)/build/upgrade-test/bin}"
STORAGE_LISTEN="${1:-127.0.0.1:22021}"
ROUTER_LISTEN="${2:-127.0.0.1:1090}"
STACK_DIR="${ARQMA_STACK_DIR:-${TMPDIR:-/tmp}/arqma-stack}"

if [[ ! -x "${BIN_DIR}/arqma-storage" || ! -x "${BIN_DIR}/arqma-router" ]]; then
  echo "Build companions first, e.g.:" >&2
  echo "  cmake --build build/upgrade-test --parallel --target arqma_storage arqma_router arqma_msg daemon" >&2
  exit 1
fi

mkdir -p "${STACK_DIR}/storage" "${STACK_DIR}/router"

"${BIN_DIR}/arqma-storage" --listen "${STORAGE_LISTEN}" --data-dir "${STACK_DIR}/storage" &
STORAGE_PID=$!
"${BIN_DIR}/arqma-router" --listen "${ROUTER_LISTEN}" --data-dir "${STACK_DIR}/router" \
  --storage-url "http://${STORAGE_LISTEN}" &
ROUTER_PID=$!

cleanup() {
  kill "${STORAGE_PID}" "${ROUTER_PID}" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "arqma-storage pid=${STORAGE_PID} http://${STORAGE_LISTEN} data=${STACK_DIR}/storage"
echo "arqma-router  pid=${ROUTER_PID} http://${ROUTER_LISTEN} data=${STACK_DIR}/router"
echo
echo "Point the consensus daemon at the companions (still a separate process):"
echo "  ${BIN_DIR}/arqmad --storage-client-url=http://${STORAGE_LISTEN} --arq-router"
echo
echo "Messenger CLI (after storage is up):"
echo "  ${BIN_DIR}/arqma-msg gen"
echo "  ${BIN_DIR}/arqma-msg send --url http://${STORAGE_LISTEN} --to <64-hex> --text hello"
echo "  ${BIN_DIR}/arqma-msg send --router http://${ROUTER_LISTEN} --to <64-hex> --text hello"
echo "  ${BIN_DIR}/arqma-msg inbox --url http://${STORAGE_LISTEN} --to <64-hex>"
echo "  ${BIN_DIR}/arqma-msg open --url http://${STORAGE_LISTEN} --to <pub> --secret <priv> --key <id>"
echo
wait
