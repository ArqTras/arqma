# Arqma Upgrade Roadmap

Branch: `upgrade`  
Goal: modernize Arqma to production-grade quality using mature Monero/Oxen
patterns while preserving Arqma identity — **fully functional on Linux,
Windows, and macOS**.

Platform matrix: [`docs/PLATFORM.md`](PLATFORM.md).

## Phase status

| Phase | Status | Notes |
|-------|--------|-------|
| 1 Analysis | **Done** | Architecture, deps, risks, Oxen gap analysis |
| 2 Core modernization | **Done** | C++20, CI, CLSAG tests, HF/Arq-Net fixes |
| 3 Oxen feature parity (selective) | **Foundation + native transport** | ArqMQ facade + `SocketStack` behind `--arqnet-backend=arqmq` |
| 4 Arq-Net evolution | **Foundation done** | Auth harden, ping, dual-stack plan |
| 5 Storage Server | **Client foundation** | HTTP GET cleartext probe; TLS TCP-only; separate binary = Milestone C |
| 6 Session-like modules | **Foundation done** | Envelope, onion peel, swarm bounds + hash, sealed-box |
| 7 RPC modernization | **Foundation done** | Validation, wallet caps/auth, OpenAPI 0.2.0 |
| 8 P2P improvements | **Foundation done** | Limits + Levin/preauth lock |
| 9 Performance | **Baseline** | Local measurement doc |
| 10 Testing | **Done (curated)** | **543** unit tests green; integration suites opt-in |
| 11 CI | **Done (3 OS)** | Linux/macOS native unit + Windows/macOS/Linux depends |
| 12 Final review | **Gate ready** | Checklist + completeness gate + platform doc |

## Cross-platform delivery map

```text
                    ┌─────────────────────────────────────┐
                    │         upgrade foundation PR         │
                    └─────────────────────────────────────┘
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          ▼                           ▼                           ▼
   ┌─────────────┐            ┌─────────────┐            ┌─────────────┐
   │   Linux     │            │   macOS     │            │  Windows    │
   │ unit+ASan   │            │ macos-14    │            │ mingw x64   │
   │ depends x64 │            │ unit native │            │ depends     │
   │ depends arm │            │ depends x64 │            │ binaries    │
   │             │            │ depends arm │            │             │
   └─────────────┘            └─────────────┘            └─────────────┘
```

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
6. Darwin depends via clang + lld (no legacy cctools for CI)

## Requires SN economics / product decision (Milestone C)

- Full Storage Server + swarm replication for messaging
- Pulse-like PoS block production
- Blink-like instant tx quorums
- Lokinet-class onion routing network
- Session-class messenger clients

Do not implement those as drive-by copies; schedule behind an explicit product
milestone.

## Milestone A — platform (**complete**)

- [x] Architectural analysis + docs
- [x] Fix HF19 burn gate off-by-one
- [x] Deny non-SN Arq-Net connections
- [x] Restore Arq-Net ping RPC (non-blocking for proofs)
- [x] CLSAG unit tests
- [x] Testnet HF19 schedule
- [x] Native GitHub Actions CI + sanitizer job
- [x] `.clang-format` / `.clang-tidy`
- [x] CMake ≥ 3.16, C++20 (`-fno-char8_t` bridge)
- [x] Green curated unit suite (**543** tests)
- [x] Restore legacy fixtures (base58/uri/parse_amount/sha256/mul_div/fee/…)
- [x] Hardfork version + serialization basic unit coverage
- [x] ArqMQ dual-backend facade (transport remains SNNetwork)
- [x] RPC validation / pagination / DoS caps / restricted SN-key RPCs
- [x] P2P limit aliases + packet-budget unit coverage
- [x] Messaging envelope + onion/swarm helpers
- [x] Linux + macOS native unit CI; Windows/macOS/Linux depends binaries
- [x] Platform matrix documentation (`docs/PLATFORM.md`)

## Milestone B — MQ / SN hygiene

- [x] `arqmq` facade under Arqma naming
- [x] Command ACL registry + authorize semantics
- [x] Dedicated transport internals under Arqma naming (`arqmq::SocketStack`)
- [x] Feature-flag dual-stack (`--arqnet-backend`)
- [x] SN hygiene audit notes
- [x] Operator upgrade path docs

Remaining before default flip (not blocking Milestone B; preserves compatibility):

- [x] Dual-run coexistence: SocketStack + SNNetwork mesh (`mesh=snnetwork` always today)
- [ ] Native mesh path carries `vote_ob` peer relay (today still SNNetwork by design)
- [ ] Stagenet dual-run parity + integration deny-path coverage

## Milestone C — product fork-in-the-road

**Requires human product decision.** Choose one primary value-add:

1. Storage + onion requests (production Storage Server binary)
2. Blink-like fast confirmation
3. Pulse-like consensus change
4. Privacy routing daemon (Lokinet-inspired)

Foundation scaffolding for (1) and (4) exists; shipping them as products is
out of scope for the foundation PR (`docs/PR_COMPLETENESS_GATE.md`).

## Explicit non-goals (near term)

- Rebranding to Oxen/Session/Lokinet
- SESH / L2 / BLS tokenomics copy
- Wallet3 wholesale import without API plan
- Co-authored commit trailers / unnecessary contributor metadata
- Enabling broken legacy `core_tests` in default CI before API migration
