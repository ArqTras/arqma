# Phases 3 to 12 Status

Branch: `upgrade`

This document tracks what is implemented as production-ready scaffolding versus
what still requires deeper protocol, service-node economics or separate-binary
work before it can be shipped as a complete feature.

## Status Matrix

| Phase | Status | Implemented now | Still required |
|-------|--------|-----------------|----------------|
| 3 MQ feature parity | Scaffolded | `src/arqmq` facade, backend enum, ACL enum, legacy path documentation | Native backend port, daemon flagging, integration with live command routing |
| 4 Arq-Net evolution | In progress | `arqmq::Backend::LegacyArqNet` documents current `SNNetwork` path | Transport hardening, migration planning, dual-stack rollout |
| 5 Storage Server | Scaffolded | `src/arq_storage` daemon-side client API with explicit not-implemented stubs | Separate production storage binary, replication, swarm sync, SN incentive decisions |
| 6 Messaging modules | Scaffolded | `src/arq_messaging` identity, onion request and swarm map placeholders | Wire format, encryption, request lifecycle, interoperability tests |
| 7 RPC modernization | Planned with docs | modernization plan and OpenAPI stub under `docs/openapi` | auth middleware, request validation, pagination semantics, generated API docs |
| 8 P2P improvements | Planned with docs | hardening recommendations tied to current constants | implementation, measurement, rollout tuning, compatibility testing |
| 9 Performance | Planned | no runtime changes in this scaffold | profiling, benchmarks, targeted optimizations |
| 10 Testing | In progress | lightweight unit coverage for `arqmq` and `arq_storage` scaffolds | integration tests, daemon tests, network-path regression coverage |
| 11 CI | In progress | current unit target remains buildable with new scaffold targets | dedicated matrix coverage for feature flags and future separate binaries |
| 12 Final review | Ongoing | documentation and compile-time scaffolding reduce later integration risk | security review, soak testing, operator validation, release gating |

## Notes by Area

### Messaging facade

- Public API remains Arqma-named.
- `LegacyArqNet` is the only backend that initializes successfully today.
- The native `ArqMq` backend intentionally reports not-supported until a real
  port exists.

### Storage integration

- The daemon-side `StorageClient` is intentionally thin.
- A full storage deployment remains a separate binary concern, not an
  in-daemon shortcut.

### Privacy routing

- `src/arq_router` is naming and lifecycle scaffolding only.
- No claim is made that production routing exists yet.

## Recommended next milestone outputs

1. Add daemon config and RPC exposure for backend selection and health reporting.
2. Define shared error semantics for cross-module upgrade components.
3. Decide which features remain in-process versus separate-binary integrations.
