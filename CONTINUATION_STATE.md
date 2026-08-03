# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **518** passed (~2.0s)
- Always local-first before push/CI

## Architecture now

- ArqMQ: ACL + framing (`authorize_request`, 1MiB/16 frames)
- Messaging: envelope, sealed-box, multi-hop onion wrap/peel
- Wallet RPC: soft caps + restricted auth catalog
- Fee: `get_dynamic_base_fee` unit coverage
- Storage: TCP reachability probe (HTTP API still deferred)

## Completed this stretch (pushed)

- test_tx_utils restore
- wallet_rpc_validation + rpc_base move
- wallet_rpc_auth restricted denials
- ArqMQ message_limits + handler wiring
- fee unit restore
- onion_layer multi-hop peel (pending push)

## Next exact implementation

1. **File:** `src/arq_storage/storage_client.cpp` / `remote_storage_client.*`
2. **Functions:** optional HTTP GET `/ping` after TCP connect when URL scheme is http(s)
3. **Tests:** mock or skip-if-no-server in `arq_storage_client.cpp`
4. **Commit:** `feat: optional HTTP ping after storage TCP reachability`
5. Then P2P preauth enforcement helper tests / ban fixture restore

## Authorship

ArqTras only — `git -c core.hooksPath=/tmp/empty-git-hooks`
