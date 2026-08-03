# Phases 3 to 12 Status

Branch: `upgrade`

This document tracks what is implemented as production-ready scaffolding versus
what still requires deeper protocol, service-node economics or separate-binary
work before it can be shipped as a complete feature.

## Status Matrix

| Phase | Status | Implemented now | Still required |
|-------|--------|-----------------|----------------|
| 3 MQ feature parity | In progress | `src/arqmq` facade, backend enum, ACL enum + `allows()`, legacy path docs, backend status introspection | Native backend port, live command routing, richer health/error reporting |
| 4 Arq-Net evolution | In progress | `arqmq::Backend::LegacyArqNet` documents current `SNNetwork` path; `get_arqnet_status` exposes backend/init status and latest ping | Transport hardening, migration planning, dual-stack rollout |
| 5 Storage Server | Scaffolded + status RPC | `src/arq_storage` + `get_storage_status` | Separate production storage binary, replication, swarm sync, SN incentive decisions |
| 6 Messaging modules | In progress | `src/arq_messaging` identity, onion request, swarm map and message-envelope roundtrip helpers | Encryption, request lifecycle, interoperability tests |
| 7 RPC modernization | In progress | validation helpers, SN hex checks, SN list pagination, batch DoS caps (txs/KIs/blocks/headers), OpenAPI stub, shared error semantics doc | auth middleware, generated API docs, more privileged-RPC hardening |
| 8 P2P improvements | In progress | documented limit aliases, packet-budget compile-time checks, unit coverage for current limits | implementation, measurement, rollout tuning, compatibility testing |
| 9 Performance | Baseline docs | `docs/PERFORMANCE.md` | profiling, benchmarks, targeted optimizations |
| 10 Testing | In progress | curated suite expanded (incl. restored mul_div / get_xtype); coverage for arqmq ACL, RPC caps, P2P limits, HF19 burn, messaging envelopes | integration tests, daemon tests, network-path regression coverage |
| 11 CI | In progress | current unit target remains buildable with new scaffold targets; non-blocking format-check job added for upgrade modules | dedicated matrix coverage for feature flags and future separate binaries |
| 12 Final review | Ongoing | `docs/SECURITY_REVIEW_CHECKLIST.md` Phase-12 merge gate + docs scaffolding | soak testing, operator validation, release gating, CI sanitizer green |

## Notes by Area

### Messaging facade

- Public API remains Arqma-named.
- `LegacyArqNet` is the only backend that initializes successfully today.
- The native `ArqMq` backend intentionally reports not-supported until a real
  port exists.
- The daemon now exposes a lightweight `get_arqnet_status` JSON-RPC method for
  backend selection, initialization state and the last Arq-Net ping timestamp.

### Storage integration

- The daemon-side `StorageClient` is intentionally thin.
- A full storage deployment remains a separate binary concern, not an
  in-daemon shortcut.

### Privacy routing

- `src/arq_router` is naming and lifecycle scaffolding only.
- No claim is made that production routing exists yet.

### RPC and validation

- `src/rpc/rpc_validation.*` centralizes small helper logic for limit clamping,
  pagination defaults and fixed-size hex validation.
- `get_service_nodes` now accepts optional `offset` and `limit` parameters
  without breaking existing full-list callers.

## Recommended next milestone outputs

1. ~~Define shared error semantics for cross-module upgrade components.~~ (`docs/ERROR_SEMANTICS.md`)
2. Decide which features remain in-process versus separate-binary integrations.
3. Wire native ArqMQ transport behind `--arqnet-backend=arqmq` without breaking LegacyArqNet.
4. Restore additional legacy unit fixtures that are safe under Arqma 9-decimal / C++20.
