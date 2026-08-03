## Completeness Gate (upgrade PR)

This PR does **not** claim that Oxen/Lokinet/Session product clones are
fully shipped. It claims the upgrade branch meets the engineering gate for
a production-grade foundation PR:

### Must-have (satisfied)

- [x] Architecture analysis + dependency/roadmap docs
- [x] Critical consensus/wallet correctness fix (HF19 burn)
- [x] Arq-Net auth hardening
- [x] Toolchain modernization (C++20, CMake, CI, sanitizers job)
- [x] Automated unit tests green on curated suite
- [x] Module scaffolding with tests/docs/CI for MQ, storage, messaging, router
- [x] RPC OpenAPI stub + modernization/P2P docs
- [x] Arqma branding preserved; no Co-Authored-By trailers
- [x] Daemon `--arqnet-backend` feature flag for MQ facade

### Explicitly deferred (documented, not blocking this PR)

- Native ArqMQ transport replacing SNNetwork
- Production Storage Server binary + swarm replication
- Full privacy router (Lokinet-class) binary
- Messenger clients / Session protocol wire completeness
- Pulse/Blink/L2 consensus changes (SN economics decisions)
- Restoring every legacy Monero-era unit fixture

See `docs/PHASES_3_TO_12_STATUS.md`.
