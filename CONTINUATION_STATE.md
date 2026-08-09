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
- [x] HF20 scaffold (`network_version_20` / `HF_VERSION_NATIVE_ARQNET_MESH`)
  - Stagenet height **240**, testnet **1300**, mainnet **unscheduled**
  - Gates: `hf_permits_native_mesh` + `native_mesh_ready_at(hf)`
- [ ] Port Curve/ZAP + `vote_ob` relay onto SocketStack
- [ ] Stagenet HF20 parity → schedule mainnet height → default flip

## Quality gates

- Local `unit_tests` → **551** passed
