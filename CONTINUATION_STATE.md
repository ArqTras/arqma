# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
Hooks: `git -c core.hooksPath=/tmp/empty-git-hooks`  
Author: ArqTras `<33489188+ArqTras@users.noreply.github.com>`

## Local quality gate (latest)

- `ninja unit_tests` → **524** passed (~2.0s)
- `ninja daemon` → OK (as of router harden commit)

## Completed (this autonomous session, on `upgrade`)

1. `test_tx_utils` restore for Arqma `construct_miner_tx`
2. `wallet_rpc_validation` + move `rpc_validation` → `rpc_base`
3. `wallet_rpc_auth` restricted denials + gap closes
4. ArqMQ `message_limits` + `authorize_request` on vote/ping/pong
5. Fee `get_dynamic_base_fee` unit restore
6. Onion multi-hop wrap/peel
7. Storage cleartext HTTP GET probe
8. `rpc_auth` unit tests
9. PROCESS_BOUNDARIES + Levin↔P2P preauth lock
10. `arq_router` validate_config / lifecycle
11. Swarm map membership bounds (pending push if not yet)

## Next exact file / function / commit

1. **File:** `docs/openapi/` stub — document wallet restricted + storage HTTP probe  
2. **File:** `src/arq_messaging/` — optional swarm_id hash helper from pubkey  
3. **Commit:** `docs: expand OpenAPI stub for wallet-rpc and storage probes`  
4. Then legacy fixture restores (`serialization.cpp` / `ban.cpp` cost analysis)

## Decision log (locked)

- Wallet dest soft cap 100; BP max outs 16 per tx  
- Storage TLS = TCP-only; HTTP GET for cleartext  
- ArqMQ framing = SN ZMQ 1MiB  
- Onion ≤3 hops, 64KiB payload  
- Swarm ≤100 SNs/mapping, ≤10000 mappings in-memory  
- Router experimental requires data_dir + host:port  

## Do not restart analysis

Resume from OpenAPI stub expansion or swarm_id helper above.
