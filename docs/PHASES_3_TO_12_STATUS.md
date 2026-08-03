# Phases 3 to 12 Status

Branch: `upgrade`

Foundation engineering for phases 3–12 is landed. Product-scale binaries
(Storage Server, Lokinet-class router, Session clients, Pulse/Blink) remain
Milestone C decisions.

## Status Matrix

| Phase | Status | Implemented now | Still required (Milestone C / later) |
|-------|--------|-----------------|--------------------------------------|
| 3 MQ feature parity | Foundation done | facade, ACL, command registry, both backends init, `transport=snnetwork` | Dedicated ArqMQ socket stack |
| 4 Arq-Net evolution | Foundation done | SN-only auth, ping RPC, dual-stack docs, status+transport RPC | Dual-run CI for future socket stack |
| 5 Storage Server | Client foundation | Remote vs InMemory client; status RPC; boundaries doc | Production storage binary + replication |
| 6 Messaging modules | Foundation done | identity, onion validation, swarm in-memory, envelope, sealed-sender | Full crypto + wire interoperability |
| 7 RPC modernization | Foundation done | validation, pagination, DoS caps, auth helpers, OpenAPI | Generated OpenAPI + full auth middleware wiring |
| 8 P2P improvements | Foundation done | limit aliases, compile-time budget checks, unit coverage | Measured rollout tuning |
| 9 Performance | Baseline done | `docs/PERFORMANCE.md` local timings | CI hardware baselines / IBD profiles |
| 10 Testing | Foundation done | curated suite **485** green locally | integration/daemon network tests; remaining API-drift fixtures |
| 11 CI | Foundation done | unit Release/Debug, ASan/UBSan, format-check | TSan optional; coverage artifacts |
| 12 Final review | Gate ready | security checklist + completeness gate | soak / signed release / stagenet |

## Recommended next milestone outputs

1. Choose Milestone C primary value-add (Storage / Blink / Pulse / Router).
2. Port dedicated ArqMQ transport behind feature flag without breaking SNNetwork.
3. Restore remaining safe legacy fixtures (`fee` API, `hardfork` mocks, …).
