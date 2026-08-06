# Release notes — upgrade foundation (Milestone A)

## Highlights

- Correct HF19 burn construction so burns work from hard fork 19 onward.
- Harden Arq-Net: only registered service nodes may authenticate to the mesh.
- Restore `arqnet_ping` / `last_arqnet_ping` for operator visibility.
- Raise the build baseline to C++20 (with `-fno-char8_t` compatibility).
- Curated unit suite (**529** tests) green on Linux and macOS; ASan/UBSan on Linux.
- Cross-compile release binaries for **Windows x64**, **macOS x64/arm64**, and
  **Linux x64/arm** via `contrib/depends`.
- Arqma-named modules: ArqMQ facade, storage client, messaging helpers,
  experimental router lifecycle.
- RPC validation, DoS soft-caps, restricted-RPC gates (including SN key methods),
  OpenAPI stub.

## Platforms

| OS | Unit tests | Release binaries |
|----|------------|------------------|
| Linux | Native CI + ASan | depends x86_64 / armv8 |
| macOS | Native `macos-14` | depends x64 + arm64 (clang+lld) |
| Windows | Gate → mingw depends | depends `x86_64-w64-mingw32` |

Details: `docs/PLATFORM.md`.

## Operator impact

- Storage Server ping remains required for uptime proofs.
- Missing Arq-Net pings do **not** yet block uptime proofs.
- Unknown remote Curve keys are rejected on the Arq-Net port.
- Restricted RPC denials return `CORE_RPC_ERROR_CODE_RESTRICTED` for operator
  and SN-key methods.
- New flags: `--arqnet-backend`, `--arq-router`, `--storage-client-url`.

## Developer impact

- CMake ≥ 3.16, C++20 compiler required.
- Default: `-DBUILD_INTEGRATION_TESTS=OFF` (curated `unit_tests`).
- Prefer `ctest -R 'unit_tests|hash-target'`.
- See `docs/MIGRATION.md`, `docs/UPGRADE_ROADMAP.md`, `docs/OPERATOR_UPGRADE.md`.

## Known limitations

- Legacy `core_tests` / some Monero-era fixtures remain behind
  `BUILD_INTEGRATION_TESTS=ON` until API migration.
- ArqMQ dedicated `SocketStack` lands behind `--arqnet-backend=arqmq` (`transport=arqmq`);
  peer quorum mesh remains SNNetwork until dual-run cutover. On **mainnet**, `arqmq` is
  refused unless `--arqnet-allow-experimental` is set.
- Full Storage Server / Lokinet-class router / Pulse / Blink are Milestone C.
