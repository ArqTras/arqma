# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Mainnet readiness locks

- Default `--arqnet-backend=legacy-arqnet`
- Peer mesh always `snnetwork` (quorum wire unchanged)
- Mainnet refuses `--arqnet-backend=arqmq` unless `--arqnet-allow-experimental`
- Testnet/stagenet may exercise `SocketStack`

## Local quality gate

- `ninja unit_tests` → **543** passed
- `ninja daemon` OK

## Next

1. Wait for CI green on tip
2. Stagenet soak before any default/mesh cutover
3. Milestone C only after product choice
