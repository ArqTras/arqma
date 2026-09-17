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

3. Refresh reserve:

```bash
curl -s -X POST http://127.0.0.1:22050/v1/etn/reserve/refresh | jq .
curl -s http://127.0.0.1:22050/v1/etn/reserve | jq .
```

Cold spend stays on a separate offline / multisig wallet. Do not put spend keys on the audit host.

## Model C — wrap (demo)

```bash
# Create a mint swap intent
curl -s -X POST http://127.0.0.1:8788/v1/swap \
  -H 'content-type: application/json' \
  -d '{"direction":"mint","dest_eth_address":"0xabc...","amount_atomic":"1000000000"}' | jq .

# Finalize / simulate (demo)
curl -s -X POST http://127.0.0.1:8788/v1/swap/<id>/finalize \
  -H 'content-type: application/json' \
  -d '{"arq_txid":"deadbeef..."}' | jq .
```

Processing worker:

```bash
python -m etn_bridge.processing.run --once
```

## Production checklist

- [ ] TLS termination in front of audit + bridge API
- [ ] Stack token / API auth enabled
- [ ] Wallet-rpc bound to localhost only
- [ ] Mint keys in HSM / multisig — not on API box
- [ ] Daily volume limits + manual balance reconcile (Loki-style)
- [ ] Contract audit before mainnet wARQ
- [ ] Legal sign-off (see implementation plan phase 0)
