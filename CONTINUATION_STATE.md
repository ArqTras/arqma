# CONTINUATION_STATE

Branch: `upgrade` · Remote: `origin/upgrade` · Authorship: ArqTras only

## Local quality gate (latest)

- `ninja unit_tests` → **522** passed (~2.0s)

## Next exact implementation

1. **File:** `docs/PROCESS_BOUNDARIES.md` — document storage HTTP probe + onion peel + wallet auth
2. **File:** `src/p2p/net_node.inl` — assert/enforce `P2P_PREAUTH_PACKET_MAX_SIZE` on handshake path if not already
3. **Commit:** docs then `feat: enforce P2P preauth packet size on handshake`
4. Then restore additional legacy fixtures where API permits

## Architecture snapshot

See prior sections; HEAD includes onion_layer, storage HTTP GET probe, rpc_auth tests,
wallet_rpc_auth/validation, ArqMQ framing, fee/tx_utils restores.
