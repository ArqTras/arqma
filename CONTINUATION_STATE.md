# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate

- `ninja unit_tests` → **539** passed
- `ninja daemon` → OK
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Milestone B + dual-run (compatibility)

- [x] Dedicated `arqmq::SocketStack` behind `--arqnet-backend=arqmq`
- [x] Dual-run coexistence: native `transport=arqmq` + peer `mesh=snnetwork`
- [x] Default `--arqnet-backend=legacy-arqnet` unchanged (full wire compatibility)
- [x] RPC `get_arqnet_status` reports `transport` and `mesh` separately
- Remaining before default flip: stagenet mesh parity on native path

## Compatibility lock

Peer Curve/ZMQ quorum mesh stays on `arqnet::SNNetwork` for **both** backends
until an explicit cutover. Experimental ArqMQ never replaces the live mesh wire
protocol in this stage.
