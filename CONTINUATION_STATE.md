# CONTINUATION_STATE

Branch: `upgrade`
Latest local gate: **493** unit tests passed; `arqmad` builds.

## Completed recently

- Storage TCP reachability (`--storage-client-url`) + daemon client
- Arq-Net ACL on vote_ob/ping/pong
- P2P handshake peerlist truncation + preauth budget docs
- AUTO_VAL_INIT restore + output_selection tests
- arq_messaging `generate_identity` (libsodium)

## Next exact work

1. File: `src/rpc/core_rpc_server.cpp`
2. Function: operator RPC entry points (`on_start_mining`, etc.)
3. Wire `rpc_auth::method_requires_operator` + AccessLevel from restricted flag; log denials
4. Then: wallet URI amount validation helpers shared with `rpc_validation`
5. Commit message: `feat: enforce operator access levels on privileged RPC`

## Priority queue (autonomous)

Networking → Wallet → Daemon → RPC → Storage Server binary scaffold tree → ArqMQ transport → Messaging crypto box seal → Testing restoration → Docs/CI
