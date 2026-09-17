# Arqma ETN path — models A + C

Status: **implementation scaffold** on branch `ETF`.  
Not consensus. Does **not** flip Pulse stage 3, default `--arqnet-backend`,
HF heights, or mainnet `NETWORK_ID`.

## Goal

Ship an EU-style **ETN / note** product path without making Arqma L1 transparent:

| Model | Meaning |
|-------|---------|
| **A — Custody ETN** | Issuer holds ARQ; investors hold a regulated note; audit via view-key / PoR |
| **C — Wrap leg** | Optional settlement on a transparent chain (wARQ) via burn↔mint |

## Architecture

```text
 Investor (KYC) ──► Issuer registrar / ETN ops
                         │
          ┌──────────────┼──────────────────────────┐
          ▼              ▼                          ▼
   arqma-wallet-rpc   arqma-etn-audit          etn-bridge API
   (view-only / cold) (PoR + attestations)     (swap UX + processing)
          │              │                          │
          └──────── ARQ custody ────────────────────┤
                                                    ▼
                                         Ethereum wARQ contracts
```

| Process | Role | In `arqmad`? |
|---------|------|--------------|
| `arqmad` | Consensus | — |
| `arqma-wallet-rpc` | Custody / view-only / `get_reserve_proof` | No |
| `arqma-etn-audit` | Model A PoR store + model C attestations (HTTP + JSON-RPC) | No |
| `contrib/etn-bridge` | Custodial wrap API / processing / frontend (Oxen/Loki-style) | No |
| ETH contracts | wARQ ERC-20 + bridge controller | External |

## Prior art (oxen-io)

| Project | Pattern |
|---------|---------|
| `loki-binance-bridge` | Custodial deposit→payout; `api` + `processing` + frontend; wallet-rpc hot wallet |
| wOXEN (`ethereum.oxen.io`) | 1:1 wrap to ERC-20 for DeFi / scannable leg |
| `eth-sn-contracts` / `ethyl` | Later Session/SESH staking on EVM — different problem |

Arqma A+C follows the **custodial wrap + issuer audit** family, not in-consensus HTLC.

## Repository map

| Path | Contents |
|------|----------|
| `docs/ETN_A_C.md` | This architecture overview |
| `docs/ETN_IMPLEMENTATION_PLAN.md` | Phased rollout |
| `docs/ETN_OPERATOR.md` | Runbooks |
| `docs/ETN_RPC.md` | HTTP + JSON-RPC reference |
| `docs/openapi/arq-etn.openapi.yaml` | OpenAPI 3 for `arqma-etn-audit` |
| `src/arq_etn/` | C++ library + `arqma-etn-audit` binary |
| `contrib/etn-bridge/` | Python API, processing, frontend, Solidity stubs |
| `tests/unit_tests/arq_etn.cpp` | Unit coverage |

## Hard locks

- Pulse `k_pulse_pow_stage = 2` (no PoW-off)
- Default `--arqnet-backend=legacy-arqnet`
- Mainnet HF20 = 4_000_000, HF21 = 5_000_000
- Mainnet `NETWORK_ID` historical fixed UUID
- Default user privacy (CLSAG / rings) unchanged
- Messenger UX unchanged (`gen` / `send` / `inbox` / `open`)

## Security notes

- Bridge hot wallets and mint keys are **issuer secrets** — never inside `arqmad`.
- Prefer view-only hot + cold/multisig spend for reserves (model A).
- Production mint on ETH must use audited contracts + multi-sig / timelock.
- This scaffold includes a **demo mode** that simulates mint/redeem without chain writes.
