# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **510** passed (~2.0s)
- `ninja daemon` / `wallet_rpc_server` → OK
- Always local-first before push/CI

## Architecture now

- Mesh: `arqnet::SNNetwork`; facade `arqmq` (both backends init; transport=`snnetwork`)
- ACL enforced in handlers: vote_ob / ping / pong
- Storage client: Remote TCP probe via `--storage-client-url`; InMemory for tests
- Messaging: envelope, onion validation, swarm map, libsodium identity + sealed-box
- RPC: `rpc_validation` lives in `rpc_base` (shared by daemon + wallet-rpc)
- Wallet RPC: `wallet_rpc_validation.h` soft caps + payment-id hex reuse
- P2P: handshake peerlist truncated to `P2P_MAX_PEERS_IN_HANDSHAKE`

## Decision log

- Storage reachability = TCP connect (not full SS HTTP API) to keep daemon decoupled
- ping/pong ACL Basic; vote_ob ServiceNode
- Preauth packet budget documented as 256KiB (Levin initial); full max remains 50MB
- Wallet transfer destination soft cap = 100 (DoS); consensus still BULLETPROOF_MAX_OUTPUTS=16 per tx
- `rpc_validation` moved into `rpc_base` so wallet-rpc links without pulling full daemon RPC

## Completed this stretch

1. Restored `test_tx_utils.cpp` for Arqma `construct_miner_tx(Blockchain*, …)` API
2. Added `wallet_rpc_validation.h` + wired caps into transfer / bulk payments / address book
3. Moved `rpc_validation` into `rpc_base`

## Next exact implementation

1. **File:** `src/wallet/wallet_rpc_auth.h` (new)
2. **Functions:** map restricted-mode denials to shared privilege helpers (mirror `rpc_auth.h`)
3. **Tests:** unit coverage for restricted vs unrestricted method sets
4. **Commit:** `feat: centralize wallet-rpc restricted method privilege checks`
5. Then restore more legacy fixtures (`fee.cpp` / `ban.cpp` if green) or expand ArqMQ request framing

## Modified areas this autonomous stretch

`src/wallet/wallet_rpc_*`, `src/rpc/CMakeLists.txt`, `tests/unit_tests/test_tx_utils.cpp`,
`tests/unit_tests/wallet_rpc_validation.cpp`, docs, CONTINUATION_STATE.md
