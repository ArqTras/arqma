# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`  
Tip: `55fc5c89`

## Mainnet readiness locks

- Default `--arqnet-backend=legacy-arqnet`
- Peer mesh always `snnetwork` (quorum wire unchanged)
- Mainnet refuses `--arqnet-backend=arqmq` unless `--arqnet-allow-experimental`
- Testnet/stagenet may exercise `SocketStack`

## Quality gates

- Local `unit_tests` → **543** passed
- CI unit (Linux/macOS + ASan + format) → ✅
- CI depends (Windows/macOS/Linux + arm) → ✅
- PR [#3](https://github.com/ArqTras/arqma/pull/3) MERGEABLE / CLEAN

## Next

1. Stagenet soak before any default/mesh cutover
2. Milestone C only after product choice
