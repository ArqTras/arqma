# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **512** passed (~2.0s)
- `ninja daemon` / `wallet_rpc_server` → OK
- Always local-first before push/CI

## Architecture now

- Mesh: `arqnet::SNNetwork`; facade `arqmq` (both backends init; transport=`snnetwork`)
- ACL enforced in handlers: vote_ob / ping / pong
- Storage client: Remote TCP probe via `--storage-client-url`; InMemory for tests
- Messaging: envelope, onion validation, swarm map, libsodium identity + sealed-box
- RPC: `rpc_validation` in `rpc_base`; wallet soft caps + restricted auth catalog
- P2P: handshake peerlist truncated to `P2P_MAX_PEERS_IN_HANDSHAKE`

## Decision log

- Storage reachability = TCP connect (not full SS HTTP API)
- Wallet transfer destination soft cap = 100; consensus still BULLETPROOF_MAX_OUTPUTS=16
- Restricted wallet-rpc: explicit method catalog + per-handler deny_if_restricted
- Closed restricted gaps: relay_tx, get_tx_key, export_key_images, start/stop_mining

## Next exact implementation

1. **File:** `tests/unit_tests/fee.cpp` — re-enable in CMakeLists
2. **Validate:** `Blockchain::get_dynamic_per_kb_fee` against Arqma constants
3. **Commit:** `test: restore dynamic fee unit coverage`
4. Then `ban.cpp` / networking framing / ArqMQ request size caps

## Modified areas this stretch

wallet_rpc_auth/validation, test_tx_utils, rpc_base validation move, docs
