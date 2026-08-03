# Arqma Upgrade Roadmap

Branch: `upgrade`  
Goal: modernize Arqma to production-grade quality using mature Monero/Oxen
patterns while preserving Arqma identity.

## Phase status

| Phase | Status | Notes |
|-------|--------|-------|
| 1 Analysis | **Done** | Architecture, deps, risks, Oxen gap analysis |
| 2 Core modernization | **In progress** | C++20, CI, CLSAG tests, HF/Arq-Net fixes |
| 3 Oxen feature parity (selective) | Planned | MQ migration, SN hygiene — no blind copy |
| 4 Arq-Net evolution | Planned | Harden mesh; Lokinet-inspired routing later |
| 5 Storage Server | Planned | Separate repo / integration; swarm sync |
| 6 Session-like modules | Planned | Protocol patterns only, Arqma branding |
| 7 RPC modernization | Planned | Auth, validation, docs/OpenAPI |
| 8 P2P improvements | Planned | DoS, sync, bandwidth |
| 9 Performance | Planned | Measure first |
| 10 Testing | **Started** | CLSAG + native CI unit tests |
| 11 CI | **Started** | `ci.yml` + existing depends/docker |
| 12 Final review | Ongoing | Per-milestone |

## Dependency map (summary)

| Dependency | Arqma today | Target |
|------------|-------------|--------|
| C++ standard | C++20 (upgrade) | Keep C++20 |
| CMake | ≥ 3.16 | Align README + CI |
| Boost | ≥ 1.66 | Track LTS |
| OpenSSL | ≥ 1.1.1 | Prefer 3.x in CI |
| libsodium | ≥ 1.0.18 | Keep |
| libzmq + cppzmq | ≥ 4.3.2 | Keep; evolve toward OxenMQ-style API |
| LMDB | vendored | Keep; audit wrappers |
| RandomARQ | submodule | Keep |
| easylogging++ | vendored | Evaluate structured logging later |

## Safe adaptations from Oxen (no new tokenomics)

1. C++20 / Ninja / ccache / LTO hygiene
2. Native CI with unit tests + sanitizers
3. Messaging library patterns (OxenMQ concepts → Arqma naming)
4. Operator RPC clarity and ping/reachability reporting
5. clang-format / clang-tidy baselines

## Requires SN economics / product decision

- Full Storage Server + swarm replication for messaging
- Pulse-like PoS block production
- Blink-like instant tx quorums
- Lokinet-class onion routing network
- Session-class messenger clients

Do not implement those as drive-by copies; schedule behind an explicit product
milestone.

## Milestone A — platform (current)

- [x] Architectural analysis + docs
- [x] Fix HF19 burn gate off-by-one
- [x] Deny non-SN Arq-Net connections
- [x] Restore Arq-Net ping RPC (non-blocking for proofs)
- [x] CLSAG unit tests
- [x] Testnet HF19 schedule
- [x] Native GitHub Actions CI + sanitizer job
- [x] `.clang-format` / `.clang-tidy`
- [x] CMake ≥ 3.16, C++20 (`-fno-char8_t` bridge)
- [x] Green local unit suite (51 tests incl. CLSAG/HF19)
- [ ] Restore legacy unit fixtures (base58/uri/parse_amount/multisig)
- [ ] Fix checkpoints unit crash
- [ ] Expand HF19 burn/per-output-fee core tests
- [ ] ArqMQ dual-backend prototype

## Milestone B — MQ / SN hygiene

- Vendor or adapt OxenMQ under Arqma naming (`arqmq` / keep `arqnet` facade)
- Feature-flag dual-stack during migration
- Audit service node proofs/ports vs current Oxen-core
- Document operator upgrade path

## Milestone C — product fork-in-the-road

Choose **one** primary value-add:

1. Storage + onion requests
2. Blink-like fast confirmation
3. Pulse-like consensus change
4. Privacy routing daemon (Lokinet-inspired)

## Explicit non-goals (near term)

- Rebranding to Oxen/Session/Lokinet
- SESH / L2 / BLS tokenomics copy
- Wallet3 wholesale import without API plan
- Co-authored commit trailers / unnecessary contributor metadata

## Reporting template (per milestone)

### Completed
### Security
### Performance
### Compatibility
### Remaining Work
### Risks
### Recommended Next Steps
