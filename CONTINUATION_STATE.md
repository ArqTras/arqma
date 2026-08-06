# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`  
Tip: `b267c27f`

## Mainnet readiness locks

- Default `--arqnet-backend=legacy-arqnet`
- Peer mesh always `snnetwork` (quorum wire unchanged)
- Mainnet refuses `--arqnet-backend=arqmq` unless `--arqnet-allow-experimental`
- Testnet/stagenet may exercise `SocketStack`
- `ci.yml` published on `master` (`de971bd3`) so Actions can discover unit CI

## Local quality gate

- `ninja unit_tests` → **543** passed (reconfirmed)
- `ninja daemon` OK

## CI note

Fresh `workflow_dispatch` runs for `b267c27f` were stuck in **queued** (no
runner assigned). Re-dispatch when GitHub hosted runners accept jobs again:
`gh workflow run ci.yml --ref upgrade` and `gh workflow run depends.yml --ref upgrade`.

## Next

1. CI green on tip once runners drain
2. Stagenet soak before any default/mesh cutover
3. Milestone C only after product choice
