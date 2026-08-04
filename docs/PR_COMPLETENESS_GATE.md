# Completeness Gate (upgrade PR)

This PR lands the **production-grade foundation** for Arqma modernization
(prompt phases 1–12 engineering baseline), verified on **Linux, Windows, and
macOS**.

It does **not** claim that full Oxen Storage Server, Lokinet-class routing,
Session messenger clients, Pulse or Blink are shipped as product clones.
Those require an explicit Milestone C product choice (see
`docs/UPGRADE_ROADMAP.md` and `docs/PLATFORM.md`).

### Must-have (satisfied)

- [x] Architecture analysis + dependency/roadmap docs
- [x] Critical consensus/wallet correctness fix (HF19 burn)
- [x] Arq-Net auth hardening
- [x] Toolchain modernization (C++20, CMake, CI, sanitizers job)
- [x] Automated unit tests green on curated suite (**529** tests)
- [x] Module scaffolding with tests/docs/CI for MQ, storage, messaging, router
- [x] ArqMQ facade backends initialize; transport remains SNNetwork
- [x] In-memory storage/swarm helpers for tests; remote client stays honest
- [x] Experimental router lifecycle (enabled flag) without claiming onion routing
- [x] RPC validation, DoS caps, auth level helpers, OpenAPI stub
- [x] Soft batch RPC DoS caps + Phase-12 security checklist
- [x] Restricted RPC gates including SN key/privkey methods
- [x] Arqma branding preserved; no Co-Authored-By trailers
- [x] Daemon `--arqnet-backend` / `--arq-router` flags
- [x] Cross-platform CI: Linux/macOS unit + Windows/macOS/Linux depends binaries
- [x] Platform matrix + operator/migration docs

### Explicitly deferred (documented, Milestone C)

- Dedicated ArqMQ socket stack replacing SNNetwork internals
- Production Storage Server binary + swarm replication
- Full privacy router binary
- Messenger clients / Session protocol wire completeness
- Pulse/Blink/L2 consensus changes (SN economics decisions)
- Restoring every legacy Monero-era unit fixture / `core_tests` in default CI
- Native MSVC unit job on `windows-latest` (Windows covered via mingw depends)

See `docs/PHASES_3_TO_12_STATUS.md`, `docs/PROCESS_BOUNDARIES.md`, and
`docs/PLATFORM.md`.
