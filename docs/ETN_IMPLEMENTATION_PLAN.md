# ETN A+C — implementation plan

Branch: `ETF`. Language: English. Authorship: ArqTras (no Cursor co-author).

## Phase 0 — Legal gate (blocks production claims)

1. Confirm EU definition of “scannable history” for the **note**, not the whole L1.
2. Confirm PoR + investor KYC sufficiency vs on-chain transparent ledger.
3. Choose wrap jurisdiction / custodian for wARQ mint keys.

## Phase 1 — Model A in-tree (current scaffold)

| Deliverable | Status |
|-------------|--------|
| `src/arq_etn` types + attestation validation | Done |
| `arqma-etn-audit` HTTP + JSON-RPC companion | Done (scaffold) |
| File-backed reserve + attestation store | Done (scaffold) |
| OpenAPI + operator docs | Done |
| Wire to live `wallet-rpc` `get_reserve_proof` | Partial (HTTP proxy hook) |
| Production TLS / auth / multi-issuer | TODO |

**Exit criteria:** issuer can refresh a PoR blob, store it, and serve `/v1/etn/reserve` to an auditor.

## Phase 2 — Model C wrap light

| Deliverable | Status |
|-------------|--------|
| `contrib/etn-bridge` API + processing + frontend | Done (scaffold + demo mode) |
| Solidity `WARQ` + `EtNBridge` stubs | Done (stubs) |
| Burn detection via daemon/wallet RPC | Hook + demo |
| Attestation publish into `arqma-etn-audit` | Hook + demo |
| Mainnet ETH deploy + audits | TODO |

**Exit criteria:** demo end-to-end: create swap → simulate burn → attestation stored → simulated mint.

## Phase 3 — Hardening

- Daily limits, manual balance checks (Loki processing pattern)
- Multisig mint / pause / upgrade keys
- Monitoring + soak against stagenet
- Optional true atomic swap research (`arq_swap`) — **separate product gate**

## Phase 4 — Product launch

- Legal prospectus / KID
- Auditor runbook using PoR packages
- Public frontend behind issuer domain (not default Arqma UX)

## Engineering order (recommended)

1. Run `arqma-etn-audit` + wallet-rpc view-only against stagenet.
2. Run `contrib/etn-bridge` in `--demo`.
3. Replace demo mint with testnet ETH deploy.
4. Point processing at real HF19 burns.
5. Security review before any mainnet mint key ceremony.

## Non-goals for this branch

- Changing consensus / Pulse / arqnet defaults
- Requiring view keys for ordinary users
- Shipping messenger or Session-class UX as ETN front door
