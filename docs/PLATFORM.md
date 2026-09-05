# Platform Support Matrix (`upgrade`)

Arqma on the `upgrade` branch is built and verified for **Linux**, **Windows**,
and **macOS**. This page is the operator/developer map of how each platform is
covered in CI and how to reproduce builds locally.

## Supported platforms

| Platform | Architecture | How validated | What you get |
|----------|--------------|---------------|--------------|
| **Linux** | x86_64 | Native unit CI (`ubuntu-24.04`) + depends cross | Daemon, wallet, unit suite, ASan |
| **Linux** | aarch64 / RPi | Depends (`aarch64-linux-gnu`, `NO_AES` RPi) | Release binaries via contrib/depends |
| **macOS** | arm64 (Apple Silicon) | Native unit CI (`macos-14`) + depends `arm64-apple-darwin` | Unit suite + release binaries |
| **macOS** | x86_64 | Depends `x86_64-apple-darwin` (clang-19 + lld) | Release binaries |
| **Windows** | x86_64 | Depends mingw (`x86_64-w64-mingw32`) | Release binaries (`.exe`) |

Native MSVC unit CI on `windows-latest` is deferred until Boost/OpenSSL packages
are pinned; Windows product binaries are the depends/mingw artifacts.

## CI workflows

| Workflow | File | Jobs |
|----------|------|------|
| Unit / format / sanitize | `.github/workflows/ci.yml` | Linux Release+Debug unit, macOS-14 unit, ASan (`-DSANITIZE=ON` = address), clang-format, Windows depends gate |
| Cross binaries | `.github/workflows/depends.yml` | Windows x64, Linux x86_64, Linux armv8 (+ RPi), macOS x64, macOS arm64 |

Default test CMake flag: `-DBUILD_INTEGRATION_TESTS=OFF` (curated `unit_tests`
only). Legacy `core_tests` remain available with `-DBUILD_INTEGRATION_TESTS=ON`
after API migration.

## Local verification (all platforms)

### Native (Linux / macOS)

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_INTEGRATION_TESTS=OFF
cmake --build build --parallel --target unit_tests hash-target-tests
ctest --test-dir build -R 'unit_tests|hash-target' --output-on-failure
```

On macOS Homebrew builds, point CMake at OpenSSL/Boost:

```bash
-DOPENSSL_ROOT_DIR="$(brew --prefix openssl@3)" \
-DCMAKE_PREFIX_PATH="$(brew --prefix);$(brew --prefix openssl@3)"
```

### Cross binaries (Linux host → Windows / macOS / Linux targets)

```bash
make depends target=x86_64-w64-mingw32          # Windows
make depends target=x86_64-apple-darwin         # macOS Intel (needs clang+lld)
make depends target=arm64-apple-darwin          # macOS Apple Silicon
make depends target=x86_64-unknown-linux-gnu    # Linux x64
make depends target=aarch64-linux-gnu           # Linux ARM
```

macOS cross from Linux requires LLVM 19 tools (`clang-19`, `lld-19`) as in
`.github/workflows/depends.yml`.

## Functional surface (this PR)

Working end-to-end on the platforms above:

- Consensus / wallet HF19 (CLSAG, burns, fees)
- Daemon + wallet RPC with validation, DoS soft-caps, restricted-RPC gates
- Arq-Net SN-only auth, ping observability; ArqMQ facade + optional SocketStack
- **Mainnet:** keep `--arqnet-backend=legacy-arqnet` (default). Peer mesh stays
  `snnetwork`. Experimental `arqmq` is refused on mainnet without
  `--arqnet-allow-experimental`.
- Storage client TCP/HTTP GET probe; messaging envelope / onion / swarm helpers
- In-repo companions: `arqma-storage`, `arqma-router`, `arqma-msg` (IPv6 listen,
  1 MiB HTTP body cap, socket timeouts on Windows and POSIX; router multi-hop
  onion forward, max 3; storage swarm membership merge/push, cap 32)
- Experimental `arq_router` lifecycle (scaffold; not a Lokinet product)

See `docs/UPGRADE_ROADMAP.md` and `docs/OPERATOR_UPGRADE.md` for mainnet locks.

## Documentation index

| Doc | Purpose |
|-----|---------|
| `docs/UPGRADE_ROADMAP.md` | Phase / milestone map |
| `docs/PR_COMPLETENESS_GATE.md` | What this PR claims vs defers |
| `docs/OPERATOR_UPGRADE.md` | Operator knobs and upgrade steps |
| `docs/MIGRATION.md` | Developer and SN migration notes |
| `docs/ARCHITECTURE.md` | Module layout |
| `docs/PROCESS_BOUNDARIES.md` | Where MQ / storage / router live |
| `docs/openapi/arqmad-rpc.openapi.yaml` | RPC OpenAPI stub |
| `CHANGELOG-upgrade.md` | User-visible change log |
| `CONTINUATION_STATE.md` | Agent / resume handoff |

## Known gaps (documented, not blockers for foundation merge)

1. Legacy Monero-era `core_tests` / some unit fixtures need API migration.
2. Native Windows MSVC unit job not yet in CI (Windows product binaries are mingw depends).
3. Full Storage swarm gossip protocol and Session-class clients remain follow-ups.
   Companion binaries, KV TTL, and inbox swarm fan-out are already in-tree.
