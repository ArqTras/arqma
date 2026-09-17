# Arqma ETN path — models A + C (sketch)

Status: **design sketch** on branch `cursor/etf-f2c7`. Not consensus.
Does **not** flip Pulse stage 3, default `--arqnet-backend`, HF heights, or
mainnet `NETWORK_ID`.

## Goal

Enable an EU-style **ETN / note** product (Xetra-class AML/KYC expectations)
without making the Arqma L1 transparent by default.

| Model | Meaning |
|-------|---------|
| **A — Custody ETN** | Issuer holds ARQ; investors hold a regulated note; audit via view-key / PoR |
| **C — Wrap leg** | Optional settlement on a transparent chain (e.g. wARQ on Ethereum) via burn↔mint |

Together: **A is the product**, **C is an optional scannable settlement rail**.

## Non-goals

- Turning off CLSAG / ring signatures on mainnet
- Requiring view keys for ordinary users
- Putting bridge keys or swap state inside `arqmad`
- Full atomic-swap HTLC in consensus (phase 3+, separate decision)

## Architecture

```text
 Investor (KYC) ──► Issuer / ETN registrar
                         │
                         ├─► ARQ custody (multisig + cold)     [Model A]
                         │      view-only hot + get_reserve_proof
                         │
                         └─► Optional wARQ (ETH) mint/redeem   [Model C]
                                ▲
                         HF19 burn ARQ + attestation
```

Process boundaries match [`PROCESS_BOUNDARIES.md`](PROCESS_BOUNDARIES.md):

- `arqmad` — consensus only
- `arqma-wallet-rpc` — custody / view-only (existing)
- **`arqma-etn-audit`** (planned companion) — aggregate PoR, publish attestations
- External ETH contracts + relayer — wrap mint/redeem (not in consensus)

## Reuse from this tree (already shipped)

| Building block | Where |
|----------------|-------|
| Watch-only / view key | `wallet2`, simplewallet, wallet API |
| Reserve proof (PoR) | `get_reserve_proof` / `check_reserve_proof` (+ wallet-rpc) |
| Tx / spend proofs | `get_tx_key`, `check_tx_proof`, `get_spend_proof` |
| Cold flow | unsigned → sign → submit; Ledger |
| Multisig | `src/multisig/` + wallet-rpc (`gen_multisig` binary still off) |
| SN stake / rewards | wallet-rpc stake; SN list payouts |
| HF19 burn | `tx_extra_burn` — input to wrap mint narrative |

## In-repo sketch layout

| Path | Role |
|------|------|
| `src/arq_etn/` | Types + attestation helpers (opt-in CMake `BUILD_ARQ_ETN`) |
| `docs/openapi/arq-etn.openapi.yaml` | Planned companion HTTP/JSON surface |
| `contrib/etn-bridge/` | Placeholder for ETH contracts / relayer (out of consensus) |

Default builds leave `BUILD_ARQ_ETN=OFF` so operator/messenger UX stays unchanged.

## Model A — custody ETN (phase 1)

### Issuer wallets

1. **Cold / multisig spend** — holds spend authority for reserves.
2. **Hot view-only** — refreshes chain, runs `get_reserve_proof` on a schedule.
3. Optional **SN staking** pocket — yield path analogous to ETHC-style products
   (legal treatment is out of scope here).

### Audit package (conceptual)

```json
{
  "schema": "arqma-etn-por-v1",
  "as_of_height": 0,
  "reserve_proof": "<wallet2 reserve proof blob>",
  "liability_total": "0",
  "notes_outstanding": "0",
  "issuer_id": "example-etn-issuer",
  "signature": "<issuer ed25519 over canonical bytes>"
}
```

### Planned companion API (not implemented)

See [`openapi/arq-etn.openapi.yaml`](openapi/arq-etn.openapi.yaml):

- `GET /v1/etn/status` — process health
- `GET /v1/etn/reserve` — latest PoR summary
- `POST /v1/etn/reserve/refresh` — rebuild proof from view-only wallet
- `GET /v1/etn/attestations/{id}` — fetch published burn/mint attestation

## Model C — wrap light (phase 2)

### Flow

1. **Mint wARQ:** customer/issuer burns ARQ (HF19) → issuer/relayer verifies burn
   → mints wARQ on Ethereum → emits attestation linking `burn_txid` ↔ `mint_tx`.
2. **Redeem:** burn wARQ on ETH → issuer pays ARQ from custody (A) → attestation.

### Why not full atomic swap in phase 2

This tree has **no** HTLC / adaptor-sig / cross-chain swap RPC. A burn↔mint
wrap with issuer attestation is the only near-term path that stays integral to
Arqma without a consensus rewrite.

### Attestation type (sketch in `src/arq_etn`)

```text
BurnMintAttestation
  direction: mint | redeem
  arq_txid / arq_burn_amount
  eth_txid / warq_amount
  height / chain_id
  issuer_pubkey + signature
```

## Phase plan

| Phase | Deliverable | In consensus? |
|-------|-------------|---------------|
| **1** | `arq_etn` types + docs + `arqma-etn-audit` scaffold; PoR refresh loop | No |
| **2** | Burn receipt verify + attestation publish; ETH mint/redeem ops runbook | No |
| **3** | Optional true atomic swap (`arq_swap`) — separate product gate | Only if explicitly scheduled |

## Hard locks (must not flip)

- Pulse `k_pulse_pow_stage = 2` (no PoW-off)
- Default `--arqnet-backend=legacy-arqnet`
- Mainnet HF20 = 4_000_000, HF21 = 5_000_000
- Mainnet `NETWORK_ID` historical fixed UUID
- Companions stay out of `arqmad`; messenger default path unchanged

## Legal gate (blocks coding of product claims)

Confirm with counsel before claiming AML fitness:

1. Definition of “scannable history” for the **note**, not the whole L1.
2. Whether PoR + KYC of note holders is sufficient vs on-chain transparent ledger.
3. Treatment of SN/mining “fresh” rewards as provenance (supporting, not sufficient).

## Next engineering steps

1. Implement `arqma-etn-audit` binary behind `BUILD_ARQ_ETN`.
2. Wire view-only wallet path to scheduled `get_reserve_proof`.
3. Spec canonical attestation bytes + unit tests (no daemon change).
4. Add `contrib/etn-bridge/README.md` for ETH contract placeholders.
