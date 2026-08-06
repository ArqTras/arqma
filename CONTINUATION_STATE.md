# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate

- `ninja unit_tests` → **535** passed
- `ninja daemon` → OK
- `BUILD_INTEGRATION_TESTS=OFF` (default)

## Milestone B

- [x] Dedicated `arqmq::SocketStack` (ZMQ workers + ACL) behind `--arqnet-backend=arqmq`
- [x] `transport_name()` / RPC report `arqmq` when native stack is active
- [x] Default path remains `legacy-arqnet` / `snnetwork` (no operator cutover)
- Remaining: dual-run mesh parity (`vote_ob` on SocketStack) before default flip

## Next

1. Dual-run / stagenet mesh parity on native transport
2. Milestone C product choice (Storage / Blink / Pulse / Router)
3. Migrate `core_tests` under `BUILD_INTEGRATION_TESTS=ON`
