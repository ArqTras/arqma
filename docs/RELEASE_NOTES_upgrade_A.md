# Release notes draft — upgrade milestone A

## Highlights

- Correct HF19 burn construction so burns work from hard fork 19 onward.
- Harden Arq-Net: only registered service nodes may authenticate to the mesh.
- Restore `arqnet_ping` / `last_arqnet_ping` for operator visibility.
- Raise the build baseline to C++20 (with `-fno-char8_t` compatibility).
- Add native GitHub Actions unit tests and sanitizer jobs.
- Add CLSAG and HF19 unit coverage (51 green unit tests in the curated suite).

## Operator impact

- Storage Server ping remains required for uptime proofs.
- Missing Arq-Net pings do **not** yet block uptime proofs.
- Unknown remote Curve keys are rejected on port 19996.

## Developer impact

- CMake ≥ 3.16, C++20 compiler required.
- Prefer `make release-test` / `make debug-test`.
- See `docs/MIGRATION.md` and `docs/UPGRADE_ROADMAP.md`.

## Known limitations

- Several legacy unit fixtures still assume Monero-era constants and are
  temporarily out of the default suite.
- ArqMQ dual-backend migration is designed but not yet implemented.
- Arq-Net uptime hard-gate remains deferred pending operator release notes.
