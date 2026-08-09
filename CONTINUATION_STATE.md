# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Tip

- Tip `a190fc90` — CI **green** (unit Debug/Release, ASan, format, depends, macOS, Windows gate)
- Native mesh Stages A–C send + opt-in shadow dual-write landed; cutover still off

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
- [x] Stage C send path: CURVE `send_to_peer` + inbound handler (unit-tested)
- [x] Stage C shadow wiring: CURVE identity on active stack + opt-in `shadow_send_to_peer` (default off)
- [ ] Stage C live: enable shadow on stagenet, verify parity, then cutover (blocker: `vote-ob-parity-unverified`)
- [ ] Stagenet HF20 parity → schedule mainnet height → default flip

## Quality gates

- Local `unit_tests` → **558** passed
- Tip CI (`a190fc90`): **green**
