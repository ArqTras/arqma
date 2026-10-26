# ETN A+C — implementation plan

Branch: `ETF`. Language: English. Authorship: ArqTras (no Cursor co-author).

## Phase 0 — Legal gate (blocks production claims)

1. Confirm EU definition of “scannable history” for the **note**, not the whole L1.
2. Confirm PoR + investor KYC sufficiency vs on-chain transparent ledger.
3. Choose wrap jurisdiction / custodian for wARQ mint keys.

## Phase 1 — Model A in-tree (current)

| Deliverable | Status |
|-------------|--------|
| `src/arq_etn` types + attestation validation | Done |
| `arqma-etn-audit` HTTP + JSON-RPC companion | Done |
| File-backed reserve + attestation store | Done |
| OpenAPI + operator docs | Done |
| Wire to live `wallet-rpc` `get_reserve_proof` + height/balance | Done (`etn_wallet_rpc`) |
| Unit test: HTTP status + demo reserve refresh | Done |
| Issuer liability publish + reconcile endpoint | Done |
| Auditor PoR package (`/v1/etn/por-package`) | Done |
| Public read-only auditor GETs | Done |
| Production TLS / multi-issuer registry | TODO |

**Exit criteria:** issuer can refresh a PoR blob, store it, and serve `/v1/etn/reserve` to an auditor.

## Phase 2 — Model C wrap light

| Deliverable | Status |
|-------------|--------|
| `contrib/etn-bridge` API + processing + frontend | Done (demo + auth hooks) |
| Solidity `WARQ` + `EtNBridge` stubs | Done (stubs) |
| `ETN_BRIDGE_TOKEN` + Bearer / `X-ETN-Token` auth | Done |
| Daily volume limit (`ETN_DAILY_LIMIT_ATOMIC`) | Done |
| Deposit sweep via wallet-rpc `get_transfers` | Done (processing worker) |
| Per-swap deposit subaddress (`create_address`) | Done (when `ETN_DEMO=0` + wallet-rpc) |
| Bridge pause (`ETN_BRIDGE_PAUSED`) | Done |
| Burn detection via daemon/wallet RPC | Hook + sweep |
| Attestation publish into `arqma-etn-audit` | Hook + demo |
| Mainnet ETH deploy + audits | TODO |

**Exit criteria:** demo end-to-end: create swap → simulate burn → attestation stored → simulated mint.

## Phase 3 — Hardening

- Manual balance reconcile + PoR package for auditors shipped
- Bridge pause + daily limits env-gated
- Multisig mint / pause / upgrade keys (on-chain)
- Monitoring + soak against stagenet
- Optional true atomic swap research (`arq_swap`) — **separate product gate**

## Phase 4 — Product launch

- Legal prospectus / KID
- Auditor runbook: [`ETN_AUDITOR.md`](ETN_AUDITOR.md)
- Public frontend behind issuer domain (not default Arqma UX)

## Engineering order (recommended)

1. Run `arqma-etn-audit` + wallet-rpc view-only against stagenet.
2. Run `contrib/etn-bridge` in demo with token + daily limit set.
3. Point processing at `ETN_WALLET_RPC_URL` for mint deposit sweep.
4. Replace demo mint with testnet ETH deploy.
5. Security review before any mainnet mint key ceremony.

## Non-goals for this branch

- Changing consensus / Pulse / arqnet defaults
- Requiring view keys for ordinary users
- Shipping messenger or Session-class UX as ETN front door
