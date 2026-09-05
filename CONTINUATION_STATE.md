# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Tip

- Stage 4 mesh gate on; HF20 **4 000 000** hybrid SN; HF21 **5 000 000** exclusive
- Pulse Milestone C started (`get_pulse_status`); PoW still required
- PR notes: `docs/PR_SUMMARY.md` (EN + PL)

## Mainnet locks until 4 000 000

- Default `--arqnet-backend=legacy-arqnet`
- Mainnet `arqmq` needs `--arqnet-allow-experimental`
- v19 wire / miner RandomARQ

## SN operating modes

| HF | Heights (mainnet) | Mode | Mesh | Block production |
|----|-------------------|------|------|------------------|
| 19 | now → 3 999 999 | legacy | SNNetwork | RandomARQ |
| 20 | 4 000 000 → 4 999 999 | hybrid | native if `arqmq`+CURVE else SNNetwork | RandomARQ + Pulse rounds (producer not signing yet) |
| 21 | ≥ 5 000 000 | exclusive | native intended; SNNetwork fallback if no CURVE | Pulse-only **when** `pow_replacement_ready`; else still RandomARQ |

## Next

- [ ] Stagenet soak on ≥2 SNs
- [ ] Pulse block producer + signatures → flip `k_pulse_pow_stage`
- [ ] Default `--arqnet-backend` flip (later; not required at HF20)

## Quality

- Local `unit_tests` → **577** passed
