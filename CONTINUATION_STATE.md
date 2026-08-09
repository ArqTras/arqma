# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Mainnet readiness locks

- Default `--arqnet-backend=legacy-arqnet`
- Peer mesh always `snnetwork` (`native_mesh_ready() == false`)
- Mainnet refuses `--arqnet-backend=arqmq` unless `--arqnet-allow-experimental`

## Post-B cutover progress

- [x] Deny-path unit coverage (`arqnet_auth` / `decide_incoming_curve_peer*`)
- [x] Explicit `native_mesh_blocker()` until Curve/ZAP peer relay is ported
- [ ] Port Curve/ZAP + `vote_ob` relay onto SocketStack
- [ ] Stagenet parity before default flip

## Quality gates

- Local `unit_tests` → **548** passed
