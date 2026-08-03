# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **493** passed (~2.0s)
- `ninja daemon` → `arqmad` OK
- Always local-first before push/CI

## Architecture now

- Mesh: `arqnet::SNNetwork`; facade `arqmq` (both backends init; transport=`snnetwork`)
- ACL enforced in handlers: vote_ob / ping / pong
- Storage client: Remote TCP probe via `--storage-client-url`; InMemory for tests
- Messaging: envelope, onion validation, swarm map, libsodium identity gen
- RPC: validation/DoS caps + AccessLevel defense-in-depth on mining/stop_daemon
- P2P: handshake peerlist truncated to `P2P_MAX_PEERS_IN_HANDSHAKE`

## Decision log

- Storage reachability = TCP connect (not full SS HTTP API) to keep daemon decoupled
- ping/pong ACL Basic; vote_ob ServiceNode
- Preauth packet budget documented as 256KiB (Levin initial); full max remains 50MB

## Next exact implementation

1. **File:** `src/arq_messaging/sealed_sender.hpp` + new `sealed_box.cpp`
2. **Functions:** `seal_payload` / `open_payload` using `crypto_box_seal` / `crypto_box_seal_open`
3. **Tests:** roundtrip in `arq_upgrade_modules.cpp` or `arq_messaging_envelope.cpp`
4. **Commit:** `feat: add libsodium sealed-box helpers for messaging payloads`
5. Then restore more unit fixtures (`test_tx_utils` AUTO_VAL_INIT may now compile)
6. Then wallet RPC shared validation helpers

## Modified areas this autonomous stretch

`src/arq_storage/*`, `src/arqmq/*`, `src/cryptonote_protocol/arqnet.cpp`, `src/p2p/net_node.inl`, `src/rpc/rpc_auth.h`, `src/rpc/core_rpc_server.cpp`, `src/arq_messaging/identity.*`, docs, CONTINUATION_STATE.md
