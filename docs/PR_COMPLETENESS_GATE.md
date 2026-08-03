# Completeness Gate (upgrade PR)

This PR lands the **production-grade foundation** for Arqma modernization
(prompt phases 1–12 engineering baseline).

It does **not** claim that full Oxen Storage Server, Lokinet-class routing,
Session messenger clients, Pulse or Blink are shipped as product clones.
Those require an explicit Milestone C product choice (see
`docs/UPGRADE_ROADMAP.md`).

### Must-have (satisfied)

- [x] Architecture analysis + dependency/roadmap docs
- [x] Critical consensus/wallet correctness fix (HF19 burn)
- [x] Arq-Net auth hardening
- [x] Toolchain modernization (C++20, CMake, CI, sanitizers job)
- [x] Automated unit tests green on curated suite (local gate before CI)
- [x] Module scaffolding with tests/docs/CI for MQ, storage, messaging, router
- [x] ArqMQ facade backends initialize; transport remains SNNetwork
- [x] In-memory storage/swarm helpers for tests; remote client stays honest
- [x] Experimental router lifecycle (enabled flag) without claiming onion routing
- [x] RPC validation, DoS caps, auth level helpers, OpenAPI stub
- [x] Soft batch RPC DoS caps + Phase-12 security checklist
- [x] Arqma branding preserved; no Co-Authored-By trailers
- [x] Daemon `--arqnet-backend` / `--arq-router` flags

### Explicitly deferred (documented, Milestone C)

- Dedicated ArqMQ socket stack replacing SNNetwork internals
- Production Storage Server binary + swarm replication
- Full privacy router binary
- Messenger clients / Session protocol wire completeness
- Pulse/Blink/L2 consensus changes (SN economics decisions)
- Restoring every legacy Monero-era unit fixture (`checkpoints`, `fee`, …)

See `docs/PHASES_3_TO_12_STATUS.md` and `docs/PROCESS_BOUNDARIES.md`.
