# Changelog — upgrade branch

## Unreleased

### Security

- Fix HF19 amount-burn construction gate (`<` instead of `<=`) so burns are
  allowed from hard fork 19 onward, matching wallet policy.
- Deny unknown Curve25519 peers on Arq-Net (service nodes only).

### Features

- Restore `arqnet_ping` RPC and `last_arqnet_ping` daemon info fields.
- Add testnet hard fork 19 at height 1200.

### Tests

- Add CLSAG and HF19 unit coverage; curate a green C++20 unit suite for CI
  while legacy Monero-era fixtures are restored incrementally.

### Build / CI

- Raise CMake minimum to 3.16; build as C++20 with `-fno-char8_t` compatibility
  for existing `u8""` string usage in epee/protocol code.
- Add native GitHub Actions workflow (`ci.yml`) with unit tests and ASan/UBSan.
- Fix `make coverage` to enable tests; add `release-test` and `debug-test`.
- Add `.clang-format` and `.clang-tidy` baselines.

### Documentation

- Add architecture, security, Arq-Net, migration and upgrade roadmap docs under
  `docs/`.
- Add phase 3–12 status matrix, RPC/P2P plans, OpenAPI stub and PR completeness gate.
- Scaffold `arqmq`, `arq_storage`, `arq_messaging`, `arq_router` modules with unit tests.
- Add daemon `--arqnet-backend` flag for messaging facade selection.
- Restore `parse_amount` unit coverage for Arqma 9-decimal amounts.
