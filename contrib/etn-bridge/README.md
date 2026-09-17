# Arqma ETN bridge (model C scaffold)

Custodial wrap API inspired by oxen-io `loki-binance-bridge`:
`api` + `processing` + frontend, with attestations published to `arqma-etn-audit`.

**Not production-ready.** Demo mode (`ETN_DEMO=1`) simulates mint/redeem without
chain writes. See `docs/ETN_OPERATOR.md` and `docs/ETN_IMPLEMENTATION_PLAN.md`.

## Layout

| Path | Role |
|------|------|
| `etn_bridge/api` | FastAPI HTTP + static UI |
| `etn_bridge/processing` | Sweep / finalize worker |
| `etn_bridge/bridge_core` | Shared DB + audit client |
| `frontend/` | SPA assets |
| `contracts/` | Solidity stubs (wARQ + bridge controller) |

## Run (demo)

```bash
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
export ETN_DEMO=1
export ETN_AUDIT_URL=http://127.0.0.1:22050
# Optional: export ETN_BRIDGE_TOKEN=...  ETN_DAILY_LIMIT_ATOMIC=...
uvicorn etn_bridge.api.app:app --host 127.0.0.1 --port 8788
```

## Tests

```bash
python -m unittest discover -s tests -v
```

## Env knobs (operators)

| Variable | Role |
|----------|------|
| `ETN_DEMO` | `1` simulate mint/redeem (default) |
| `ETN_BRIDGE_TOKEN` | Bearer / `X-ETN-Token` auth |
| `ETN_DAILY_LIMIT_ATOMIC` | Soft daily volume cap (0 off) |
| `ETN_WALLET_RPC_URL` | Deposit sweep for mint (`ETN_DEMO=0`) |
| `ETN_AUDIT_URL` | `arqma-etn-audit` base URL |
