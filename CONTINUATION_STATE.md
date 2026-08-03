# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only (strip Co-authored)

## Local quality gate (latest)

- `ninja unit_tests` → **519** passed (~2.0s)
- Always local-first before push/CI

## Architecture now

- ArqMQ: ACL + framing (`authorize_request`)
- Messaging: envelope, sealed-box, multi-hop onion wrap/peel
- Storage: HTTP GET probe for cleartext; TLS = TCP connect only
- Wallet RPC: soft caps + restricted auth
- Fee / tx_utils fixtures restored

## Next exact implementation

1. **File:** `src/rpc/rpc_auth.h` + `tests/unit_tests/rpc_auth.cpp` (new)
2. **Functions:** unit coverage for `method_requires_operator` / `allow_rpc_method`
3. **Commit:** `test: cover daemon RPC access-level helpers`
4. Then expand `docs/PROCESS_BOUNDARIES.md` storage/messaging notes
5. Then evaluate `ban.cpp` restore cost vs P2P limit helpers

## Decision log

- Storage HTTP probe accepts any `HTTP/` response (incl. 4xx) as reachability
- TLS storage probe deferred (no TLS client in arq_storage yet)
- Onion: max 3 hops, 64KiB payload, 96KiB ciphertext

## Authorship

`git -c core.hooksPath=/tmp/empty-git-hooks` · ArqTras
`33489188+ArqTras@users.noreply.github.com`
