# ETN auditor runbook (model A)

Use this against a live `arqma-etn-audit` instance. Read-only GETs do **not**
require `--token`. Issuer mutations (refresh / liability / publish) do when token is set.

## 1. Fetch the PoR package

```bash
AUDIT_URL=http://127.0.0.1:22050
curl -s "$AUDIT_URL/v1/etn/por-package" | jq .
```

Package schema: `arqma-etn-por-package-v1`. It embeds:

| Field | Meaning |
|-------|---------|
| `reserve` | Latest PoR summary (`reserve_proof`, heights, balances) |
| `reconcile` | `covered` / `surplus_atomic` vs liabilities |
| `attestation_ids` | Recent wrap attestations (up to 64) |

JSON-RPC equivalent: `etn_get_por_package`.

## 2. Verify reserve proof offline

1. Export `reserve.reserve_proof` from the package.
2. On an auditor workstation with `arqma-wallet-cli` / RPC:

```bash
# Conceptual — use check_reserve_proof against the issuer address / message
# used by arqma-etn-audit (default message: arqma-etn-por).
```

3. Confirm `as_of_height` is near chain tip for the custody wallet.
4. Confirm `wallet_balance_atomic` matches the unlocked balance you expect.

## 3. Liability coverage

```bash
curl -s "$AUDIT_URL/v1/etn/reconcile" | jq .
```

`covered: true` means reported wallet balance ≥ issuer-published `liability_atomic`.
Liabilities are **not** inferred from the chain — the issuer must publish them.

## 4. Wrap attestations (model C)

```bash
curl -s "$AUDIT_URL/v1/etn/attestations" | jq .
curl -s "$AUDIT_URL/v1/etn/attestations/<id>" | jq .
```

Each attestation links an ARQ burn/payout txid to an ETH mint/burn txid with issuer signature fields.

## 5. What this does *not* prove

- Investor KYC / note legal status (phase 0)
- That ETH mint keys are correctly held in HSM/multisig
- That the issuer will honor redemptions

Treat the package as **custody evidence**, not a prospectus.
