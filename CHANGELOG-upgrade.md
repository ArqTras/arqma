# Changelog — upgrade branch

## Unreleased

### Security

- Fix HF19 amount-burn construction gate (`<` instead of `<=`) so burns are
  allowed from hard fork 19 onward, matching wallet policy.
- Deny unknown Curve25519 peers on Arq-Net (service nodes only).
- Add explicit HF19 burn gate helper coverage and document current P2P packet /
  connection ceilings with compile-time assertions and unit tests.
- Soft-cap batch RPC DoS amplification for `get_transactions`,
  `is_key_image_spent`, `get_blocks_by_height`, batch
  `get_block_header_by_hash`, and `get_block_headers_range`; fix missing early
  return on key-image size mismatch.
- Cap `get_service_nodes` pubkey filter lists (`max_service_node_pubkeys_per_request`).
- Soft-cap wallet RPC transfer destinations, bulk payment IDs, address-book
  index lists and subaddress index filters (`wallet_rpc_validation.h`); add
  `WALLET_RPC_ERROR_CODE_TOO_MANY_ENTRIES`.
- Centralize wallet-rpc `--restricted-rpc` denials (`wallet_rpc_auth.h`) and
  close gaps on `relay_tx`, `get_tx_key`, `export_key_images`, mining controls.
- Gate additional daemon operator RPCs (`set_bans`, `flush_txpool`, `save_bc`,
  `relay_tx`, log controls, `pop_blocks`, `prune_blockchain`,
  `get_service_node_key`, `get_service_node_privkey`) via `rpc_auth`
  with `CORE_RPC_ERROR_CODE_RESTRICTED`.
- Cap ArqMQ/Arq-Net request framing (1 MiB / 16 frames / 64-byte command names)
  via `authorize_request` on vote_ob/ping/pong.
- Cleartext storage reachability uses HTTP GET after TCP connect; TLS stays
  TCP-only until a TLS client is wired.
- Harden experimental `arq_router` lifecycle: require data_dir, validate
  listen `host:port`, require init before start.
- Add ArqMQ builtin command ACL registry (`authorize` denies unknown commands).
- Both `--arqnet-backend` values initialize; RPC exposes `transport=snnetwork`.
- In-memory StorageClient / SwarmMap / onion validation / router lifecycle tests.
- RPC access-level helpers (`rpc_auth.h`).

### Features

- Add `--arqnet-mesh-shadow` opt-in SocketStack dual-write (stagenet/testnet soak;
  mainnet needs `--arqnet-allow-experimental`) and expose `mesh_shadow*` counters
  on `get_arqnet_status`. Live peer mesh remains SNNetwork.
- Add live-vs-shadow parity telemetry (`mesh_live_relays`, `mesh_vote_ob_*`,
  `mesh_shadow_parity_sample_ok`) for stagenet soak decisions.
- Bind opt-in mesh-shadow CURVE listener on ANET+10000 and rewrite dual-write
  peer hints to that port (`mesh_shadow_endpoint`, `mesh_vote_ob_shadow_in`).
- Add `utils/arqnet-mesh-soak-monitor.py` to poll soak parity via `get_arqnet_status`.
- Scaffold HF20 cutover relay (`primary_mesh_send_to_peer`); inactive while
  `k_native_mesh_port_stage == 3` / `vote-ob-parity-unverified`.
- Expose cutover gates on `get_arqnet_status` (`native_mesh_ready`,
  `native_mesh_blocker`, `native_mesh_hf_permits`, `hard_fork_version`).
- Restore `arqnet_ping` RPC and `last_arqnet_ping` daemon info fields.
- Add testnet hard fork 19 at height 1200.
- Add `--storage-client-url` for outbound Storage Server TCP reachability probes.
- Enforce ArqMQ command ACL checks on Arq-Net `vote_ob` / `ping` / `pong` handlers.
- Add `src/rpc/rpc_validation.*` helpers for bounded limits, fixed-size hex
  validation and pagination scaffolding.
- Add `get_arqnet_status` JSON-RPC for backend name, initialization state and
  latest Arq-Net ping visibility.
- Add `src/arq_messaging/message_envelope.hpp` with minimal binary
  encode/decode roundtrip helpers.
- Add multi-hop onion wrap/peel helpers (`onion_layer.*`) using sealed-box
  with payload/ciphertext size caps.
- Bound in-memory swarm map membership (`max_service_nodes_per_swarm`).

### Tests

- Add CLSAG and HF19 unit coverage; curate a green C++20 unit suite for CI
  while legacy Monero-era fixtures are restored incrementally (**529** tests).
- Add hardfork version constants and basic serialization roundtrip tests.
- Add unit coverage for RPC validation helpers, P2P hardening constants,
  ArqMQ facade state and messaging envelope roundtrips.
- Add unit coverage for daemon `rpc_auth` access-level helpers.
- Restore `mul_div` and `get_xtype_from_string` legacy unit coverage.
- Restore `sha256` unit coverage after `tools::sha256sum_str` API migration.
- Restore `base58` / account-address unit coverage with Arqma mainnet (`ar`) vectors.
- Restore `uri` wallet URI coverage for `arqma:` scheme with generated testnet addresses.
- Add `arqmq::allows` ACL privilege helper coverage and SN hygiene audit notes.
- Add sealed-sender marker / TTL bound helpers for messaging envelopes.

### Build / CI

- Raise CMake minimum to 3.16; build as C++20 with `-fno-char8_t` compatibility
  for existing `u8""` string usage in epee/protocol code.
- Default `BUILD_INTEGRATION_TESTS=OFF` so CI builds curated `unit_tests` only.
- Add native GitHub Actions workflow (`ci.yml`): Linux Release/Debug unit,
  macOS-14 unit, ASan/UBSan, format-check, Windows depends gate.
- Modernize macOS depends cross-compile to clang-19 + lld; keep Windows/Linux/
  macOS release binaries in `depends.yml`.
- Fix `make coverage` to enable tests; add `release-test` and `debug-test`.
- Add `.clang-format` and `.clang-tidy` baselines.
- Add a non-blocking `clang-format --dry-run` CI job for upgrade modules.

### Documentation

- Add architecture, security, Arq-Net, migration and upgrade roadmap docs under
  `docs/`.
- Add `docs/PLATFORM.md` — Linux / Windows / macOS support and CI map.
- Add phase 3–12 status matrix, RPC/P2P plans, OpenAPI stub and PR completeness gate.
- Add `SECURITY_REVIEW_CHECKLIST.md` and `ERROR_SEMANTICS.md` for Phase 12 gating.
- Add `PROCESS_BOUNDARIES.md` and ArqMQ command registry notes.
- Scaffold `arqmq`, `arq_storage`, `arq_messaging`, `arq_router` modules with unit tests.
- Add daemon `--arqnet-backend` flag for messaging facade selection.
- Restore `parse_amount` unit coverage for Arqma 9-decimal amounts.
- Restore dynamic base-fee unit coverage for pre-per-byte and HF19 output fees.
- Add RPC pagination helpers, `get_arqnet_status`, `get_storage_status`, HF19 burn helper,
  messaging envelope codec, P2P limit aliases, operator/performance docs, and
  `--arq-router` experimental scaffold flag.
- Expand the OpenAPI stub with `get_service_nodes` pagination parameters and
  refresh the phase/roadmap status docs for the latest upgrade work.
