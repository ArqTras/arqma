# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Tip

- Stage 4 mesh gate on; HF20 **4 000 000** hybrid SN; HF21 **5 000 000** exclusive
- Pulse Milestone C: hybrid producer; majority certificate extra bound to miner
  payload hash; weight-neutral miner extra; round bound to block timestamp
- PR notes: GitHub PR https://github.com/ArqTras/arqma/pull/3 (EN + PL)

## Mainnet locks until 4 000 000

- Default `--arqnet-backend=legacy-arqnet`
- Mainnet `arqmq` needs `--arqnet-allow-experimental`
- v19 wire / miner RandomARQ

## SN operating modes

| HF | Heights (mainnet) | Mode | Mesh | Block production |
|----|-------------------|------|------|------------------|
| 19 | now → 3 999 999 | legacy | SNNetwork | RandomARQ |
| 20 | 4 000 000 → 4 999 999 | hybrid | native if `arqmq`+CURVE else SNNetwork | RandomARQ + Pulse signatures via `pulse_rnd` (PoW still required) |
| 21 | ≥ 5 000 000 | exclusive (mesh) | native intended; SNNetwork fallback if no CURVE | RandomARQ **+** Pulse (hybrid; PoW not dropped) |

## Next

- [x] Local loopback mesh/Pulse soak (operator may re-run live stagenet later)
- [x] Shadow `pulse_rnd` parse telemetry on `get_arqnet_status` / soak monitor
- [x] Pulse extra only after majority; honest `get_pulse_status.signature_count`
- [x] Canonical Pulse extra vote order (`validator_index` strictly increasing)
- [x] Fixed-size majority certificate extra + round bound to block wait-window
- [x] Pulse extra payload binding (quorum signs miner template hash; idle votes stay round-only)
- [x] Weight-neutral Pulse extra on miner templates + honest `get_pulse_status` certificate fields
- [x] Daemon `print_pulse` + Pulse fields on block headers
- [x] In-repo companions: `arqma-storage`, `arqma-router`, `arqma-msg` + Blink collector/`get_blink_status`
- [x] Storage `--data-dir` volume + router `POST /v1/peel` + `arqma-msg inbox`
- [x] Storage `--peer` replica fan-out + router `POST /v1/store` + `arqma-msg open`
- [x] Storage TTL (`?ttl=`) + inbox swarm fan-out (`/v1/snodes` members) + `GET /v1/swarm`
- [x] Cross-platform companion HTTP (IPv6 listen, 1 MiB body cap, socket timeouts, Windows stack.cmd)
- [x] Inbox swarm read fallback (`retrieve` / `list_keys` try `/v1/snodes` members)
- [ ] Keep Pulse hybrid (do **not** flip `k_pulse_pow_stage` to 3 / PoW-off)
- [ ] Default `--arqnet-backend` flip (later; not required at HF20)

## Quality

- Local `unit_tests` → **613** passed (inbox swarm read fallback)
