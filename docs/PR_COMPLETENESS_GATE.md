# Completeness Gate (upgrade PR)

This PR lands the **production-grade foundation** for Arqma modernization
(prompt phases 1–12 engineering baseline), verified on **Linux, Windows, and
macOS**.

It does **not** claim Session-class mobile clients or Storage swarm
replication. Pulse SN **hybrid (HF20) / exclusive (HF21)** gates, Pulse
`pulse_rnd` collector, wait-windows, payload-bound majority extra, and
`get_pulse_status` are started; RandomARQ stays required. Companion binaries
`arqma-storage`, `arqma-router`, and `arqma-msg` plus in-daemon Blink
(`get_blink_status`) ship from this repository. See
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

- Native mesh cutover: stage 4 is on; live `vote_ob` uses SocketStack only with HF20+ `arqmq`+CURVE (SNNetwork fallback otherwise)
- Storage swarm replication / durable volume (HTTP KV binary is in-tree)
- Session-class client UX beyond `arqma-msg`
- Pulse/Blink/L2: Pulse **hybrid producer started**; Blink collector/RPC in-tree (does not replace Pulse or PoW)
- Restoring every legacy Monero-era unit fixture / `core_tests` in default CI
- Native MSVC unit job on `windows-latest` (Windows covered via mingw depends)

See `docs/PHASES_3_TO_12_STATUS.md`, `docs/PROCESS_BOUNDARIES.md`, and
`docs/PLATFORM.md`.
