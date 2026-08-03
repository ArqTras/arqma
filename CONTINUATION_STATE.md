# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only  
`git -c core.hooksPath=/tmp/empty-git-hooks` · `33489188+ArqTras@users.noreply.github.com`

## Local quality gate (latest)

- `ninja unit_tests` → **523** passed (~2.0s)
- `ninja daemon` → OK

## Architecture snapshot

- ArqMQ: ACL + `authorize_request` framing (1MiB / 16 frames / 64 cmd)
- Messaging: envelope, sealed-box, multi-hop onion wrap/peel
- Storage: cleartext HTTP GET probe; TLS = TCP-only
- Wallet-rpc: soft caps + restricted auth catalog
- Router: experimental lifecycle with validate_config / state_name
- RPC: rpc_base validation + rpc_auth tests
- Fee / tx_utils / p2p Levin↔preauth lock

## Next exact implementation

1. **File:** `docs/openapi/` — add wallet-rpc restricted + storage probe notes
2. **File:** `src/arq_messaging/swarm_map.hpp` — add bound helpers + unit tests
3. **Commit:** `feat: bound swarm map membership lookups`
4. Then serialize fixture restore / OpenAPI expansion / CI green check

## Modified this long stretch (pushed)

tx_utils, wallet_rpc_*, rpc_base move, ArqMQ limits, fee, onion_layer,
storage HTTP GET, rpc_auth tests, PROCESS_BOUNDARIES, p2p Levin assert,
arq_router validate_config
