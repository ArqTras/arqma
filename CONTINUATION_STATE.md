# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **516** passed (~2.1s)
- `ninja daemon` / `wallet_rpc_server` → OK
- Always local-first before push/CI

## Architecture now

- Mesh: `arqnet::SNNetwork`; facade `arqmq` (transport=`snnetwork`)
- ACL + framing: `authorize_request` on vote_ob / ping / pong
- Storage client: Remote TCP probe; InMemory for tests
- Messaging: sealed-box + identity + envelope/onion/swarm
- RPC: shared `rpc_base` validation; wallet soft caps + restricted auth
- Fee: unit coverage for `get_dynamic_base_fee` (v12 + HF19)

## Decision log

- ArqMQ framing caps match SN ZMQ 1 MiB maxmsgsize
- Legacy `get_dynamic_per_kb_fee` tests replaced by `get_dynamic_base_fee`
- Restricted wallet-rpc catalog + deny_if_restricted defense-in-depth

## Next exact implementation

1. **File:** `src/arq_messaging/onion_routing.hpp` (or extend envelope)
2. **Functions:** multi-hop onion peel helper with size bounds
3. **Tests:** peel roundtrip in `arq_messaging_envelope.cpp`
4. **Commit:** `feat: add bounded multi-hop onion peel helpers`
5. Then restore `ban.cpp` if API fits, or expand storage client HTTP stub

## Modified areas this stretch

wallet_rpc_*, arqmq message_limits, arqnet handlers, fee tests, docs
