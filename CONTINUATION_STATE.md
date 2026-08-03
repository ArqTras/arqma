# CONTINUATION_STATE

Branch: `upgrade`

## Completed (this cycle)

- Storage endpoint parser + Remote TCP reachability (`--storage-client-url`)
- Daemon/global storage client for `get_storage_status`
- ArqMQ ACL enforcement in Arq-Net handlers (vote_ob/ping/pong)
- Local: 489 unit tests green; daemon builds

## Decision log

- Chose TCP connect probe over full HTTP RPC client for SS reachability: lower coupling, sufficient for operator status, avoids embedding SS protocol versioning in-daemon until a dedicated Storage Server binary lands.
- ping/pong ACL = Basic; vote_ob = ServiceNode (fail-closed for non-SN).

## Next exact work

1. File: `src/p2p/net_node.h` / related limits — enforce documented DoS aliases at runtime where still soft
2. Or restore `address_from_url.cpp` / fix `AUTO_VAL_INIT` in output_selection
3. Then wallet RPC validation parity with daemon rpc_validation helpers

## Next commit (after current)

`feat: tighten P2P connection intake against documented ceilings`
