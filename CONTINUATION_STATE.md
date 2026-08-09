# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Tip

- Branch tip CI green on `a7707a1b` (unit Debug/Release, ASan, format, depends, macOS, Windows gate)
- Follow-up: native mesh Stages A–B (Curve/ZAP + peer table) — see Post-B

## Mainnet readiness locks

- Default `--arqnet-backend=legacy-arqnet`
- Peer mesh always `snnetwork` (`native_mesh_ready() == false`)
- Mainnet refuses `--arqnet-backend=arqmq` unless `--arqnet-allow-experimental`

## Post-B cutover progress

- [x] Deny-path unit coverage (`arqnet_auth` / `decide_incoming_curve_peer*`)
- [x] Explicit staged `native_mesh_blocker()` (cutover stays off)
- [x] HF20 scaffold (`network_version_20` / `HF_VERSION_NATIVE_ARQNET_MESH`)
  - Stagenet height **240**, testnet **1300**, mainnet **unscheduled**
  - Gates: `hf_permits_native_mesh` + `native_mesh_ready_at(hf)`
- [x] Stage A: Curve/ZAP allow on SocketStack (`curve_zap`, `bind_curve`)
- [x] Stage B: peer endpoint table (`PeerTable`)
- [ ] Stage C: outbound peer send + `vote_ob` relay (blocker: `peer-send-path-missing`)
- [ ] Stagenet HF20 parity → schedule mainnet height → default flip

## Quality gates

- Local `unit_tests` → **556** passed
- Prior tip CI (`a7707a1b`): **green**; next tip includes Curve/ZAP Stages A–B
