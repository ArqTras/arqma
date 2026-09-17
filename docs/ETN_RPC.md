# ETN RPC / HTTP reference

Primary companion: **`arqma-etn-audit`** (default `127.0.0.1:22050`).  
Bridge API: **`contrib/etn-bridge`** (default `127.0.0.1:8788`).  
OpenAPI: [`openapi/arq-etn.openapi.yaml`](openapi/arq-etn.openapi.yaml).

## arqma-etn-audit — HTTP

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/status` | Liveness |
| GET | `/v1/etn/status` | Service + issuer id + counts |
| GET | `/v1/etn/reserve` | Latest `ReserveSummary` |
| POST | `/v1/etn/reserve/refresh` | Rebuild from wallet-rpc or demo stub |
| POST | `/v1/etn/reserve/liability` | Set issuer-published `liability_atomic` |
| GET | `/v1/etn/reconcile` | Compare wallet balance vs liabilities |
| GET | `/v1/etn/attestations` | List attestation ids |
| GET | `/v1/etn/attestations/{id}` | Fetch one attestation |
| POST | `/v1/etn/attestations` | Publish attestation (issuer) |

Optional query `token=` when `--token` is set (except `/status`).

## arqma-etn-audit — JSON-RPC (`POST /json_rpc`)

Compatible envelope: `{"jsonrpc":"2.0","id":"0","method":"...","params":{...}}`.

| Method | Params | Result |
|--------|--------|--------|
| `etn_get_status` | `{}` | same as `/v1/etn/status` |
| `etn_get_reserve` | `{}` | reserve summary |
| `etn_refresh_reserve` | `{}` | refreshed summary |
| `etn_set_liability` | `{ "liability_atomic": "..." }` | updated reserve |
| `etn_reconcile` | `{}` | covered / surplus vs PoR package |
| `etn_list_attestations` | `{}` | `{ "ids": [...] }` |
| `etn_get_attestation` | `{ "id": "..." }` | attestation object |
| `etn_publish_attestation` | attestation fields | `{ "id": "..." }` |

## Existing wallet-rpc methods used by model A

Do **not** reinvent these — call `arqma-wallet-rpc` (see `src/arq_etn/etn_wallet_rpc.*`):

| Method | Use |
|--------|-----|
| `get_height` | PoR `as_of_height` |
| `get_balance` | PoR `wallet_balance_atomic` |
| `get_reserve_proof` | PoR `reserve_proof` blob |
| `check_reserve_proof` | Verify PoR (auditor tooling) |
| `query_key` | view key export (offline ceremony) |
| `get_tx_proof` / `check_tx_proof` | Optional tx-level evidence |
| `sign_transfer` / `submit_transfer` | Cold spend path |
| `get_transfers` | Bridge mint deposit sweep (`ETN_WALLET_RPC_URL`) |

## etn-bridge — HTTP

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/` | Frontend |
| GET | `/v1/status` | Bridge health (`auth_required`, `daily_limit_atomic`) |
| POST | `/v1/swap` | Create mint/redeem intent |
| GET | `/v1/swap/{id}` | Swap status |
| POST | `/v1/swap/{id}/finalize` | Attach ARQ/ETH tx + trigger processing hook |
| GET | `/v1/swaps` | List recent swaps |

Auth (when `ETN_BRIDGE_TOKEN` is set): `Authorization: Bearer <token>` or `X-ETN-Token: <token>`
on mutate / list endpoints. Daily cap: `ETN_DAILY_LIMIT_ATOMIC` (0 = disabled).
