# ETN operator guide

## Quick start (local demo)

```bash
# 1) Build companion (with the rest of the tree)
cmake -S . -B build/etf -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
cmake --build build/etf --parallel --target arqma_etn_audit unit_tests

# 2) Start audit companion
build/etf/bin/arqma-etn-audit --listen 127.0.0.1:22050 --data-dir ~/.arqma/etn-audit

# 3) Bridge API + UI (demo mode — no chain writes)
cd contrib/etn-bridge
python3 -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
export ETN_AUDIT_URL=http://127.0.0.1:22050
export ETN_DEMO=1
uvicorn etn_bridge.api.app:app --host 127.0.0.1 --port 8788
# UI: http://127.0.0.1:8788/
```

## Model A — custody PoR

1. Run `arqma-wallet-rpc` on a **view-only** issuer wallet (`--restricted-rpc` as appropriate).
2. Point audit companion at it:

```bash
arqma-etn-audit \
  --listen 127.0.0.1:22050 \
  --data-dir ~/.arqma/etn-audit \
  --wallet-rpc-url http://127.0.0.1:19991/json_rpc \
  --issuer-id example-etn-issuer
```

3. Refresh reserve (calls `get_height`, `get_balance`, `get_reserve_proof`):

```bash
curl -s -X POST http://127.0.0.1:22050/v1/etn/reserve/refresh | jq .
curl -s -X POST http://127.0.0.1:22050/v1/etn/reserve/liability \
  -H 'content-type: application/json' \
  -d '{"liability_atomic":"1000000000000"}' | jq .
curl -s http://127.0.0.1:22050/v1/etn/reconcile | jq .
curl -s http://127.0.0.1:22050/v1/etn/reserve | jq .
```

PoR JSON keeps `liability_atomic` as issuer-published liabilities and fills
`wallet_balance_atomic` + `reserve_proof` from wallet-rpc when configured.

Cold spend stays on a separate offline / multisig wallet. Do not put spend keys on the audit host.

## Model C — wrap (demo)

```bash
# Optional production-style auth + daily cap (atomic units)
export ETN_BRIDGE_TOKEN=dev-token
export ETN_DAILY_LIMIT_ATOMIC=50000000000000

# Create a mint swap intent
curl -s -X POST http://127.0.0.1:8788/v1/swap \
  -H 'content-type: application/json' \
  -H 'Authorization: Bearer dev-token' \
  -d '{"direction":"mint","dest_eth_address":"0xabc...","amount_atomic":"1000000000"}' | jq .

# Finalize / simulate (demo)
curl -s -X POST http://127.0.0.1:8788/v1/swap/<id>/finalize \
  -H 'content-type: application/json' \
  -H 'X-ETN-Token: dev-token' \
  -d '{"arq_txid":"deadbeef..."}' | jq .
```

Processing worker (demo finalize, or live deposit sweep when `ETN_DEMO=0`):

```bash
export ETN_WALLET_RPC_URL=http://127.0.0.1:19991/json_rpc   # mint deposit sweep
python -m etn_bridge.processing.run --once
```

Bridge unit tests:

```bash
cd contrib/etn-bridge && python -m unittest discover -s tests -v
```

## Production checklist

- [ ] TLS termination in front of audit + bridge API
- [ ] `ETN_BRIDGE_TOKEN` / stack token enabled
- [ ] `ETN_DAILY_LIMIT_ATOMIC` set and monitored
- [ ] Wallet-rpc bound to localhost only
- [ ] Mint keys in HSM / multisig — not on API box
- [ ] Manual balance reconcile (Loki-style)
- [ ] Contract audit before mainnet wARQ
- [ ] Legal sign-off (see implementation plan phase 0)
