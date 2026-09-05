# Completeness Gate (upgrade PR)

This PR lands the **production-grade foundation** for Arqma modernization
(prompt phases 1–12 engineering baseline), verified on **Linux, Windows, and
macOS**.

It does **not** claim that full Oxen Storage Server, Lokinet-class routing,
Session messenger clients, or Blink are shipped. Pulse SN **hybrid (HF20) /
exclusive (HF21)** gates, Pulse `pulse_rnd` collector, wait-windows, payload-bound
majority extra, and `get_pulse_status` are
started; RandomARQ stays required (hybrid PoW — Pulse does not replace mining). See
https://github.com/ArqTras/arqma/pull/3.

### Must-have (satisfied)

- [x] Architecture analysis + dependency/roadmap docs
- [x] Critical consensus/wallet correctness fix (HF19 burn)
- [x] Arq-Net auth hardening
- [x] Toolchain modernization (C++20, CMake, CI, sanitizers job)
- [x] Automated unit tests green on curated suite (**548** tests)
- [x] Module scaffolding with tests/docs/CI for MQ, storage, messaging, router
- [x] ArqMQ facade + dedicated `SocketStack` behind `--arqnet-backend=arqmq`
- [x] In-memory storage/swarm helpers for tests; remote client stays honest
- [x] Experimental router lifecycle held for daemon lifetime (scaffold only)
- [x] RPC validation, DoS caps, auth level helpers, OpenAPI stub (JSON-RPC aligned)
- [x] Soft batch RPC DoS caps + Phase-12 security checklist
- [x] Restricted RPC gates including SN key/privkey + wallet lifecycle/proofs/CSV
- [x] Checkpoints empty/pre-init unit coverage restored
- [x] Arqma branding preserved; no Co-Authored-By trailers
- [x] Daemon `--arqnet-backend` / `--arq-router` flags
- [x] Cross-platform CI: Linux/macOS unit + Windows/macOS/Linux depends binaries
- [x] Platform matrix + operator/migration docs

### Explicitly deferred (documented)

- Native mesh cutover: `vote_ob` peer relay on `SocketStack` (dual-run / Milestone B cutover)
- Production Storage Server binary + swarm replication (Milestone C)
- Full privacy router binary (Milestone C)
- Messenger clients / Session protocol wire completeness (Milestone C)
- Pulse/Blink/L2: Pulse **hybrid producer started** (signatures, `pulse_rnd`, wait-windows, idle SN votes); Blink still later
- Restoring every legacy Monero-era unit fixture / `core_tests` in default CI
- Native MSVC unit job on `windows-latest` (Windows covered via mingw depends)

See `docs/PHASES_3_TO_12_STATUS.md`, `docs/PROCESS_BOUNDARIES.md`, and
`docs/PLATFORM.md`.
