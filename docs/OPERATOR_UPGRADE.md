# Operator Upgrade Guide (`upgrade` branch)

## Mainnet policy (production)

This branch is prepared for **mainnet** operators. Compatibility locks:

1. **Default** `--arqnet-backend=legacy-arqnet` — do not change on mainnet SNs.
2. **Peer mesh** (`vote_ob` / quorum Curve+ZMQ) always stays on `arqnet::SNNetwork`
   until an explicit cutover after stagenet parity.
3. `--arqnet-backend=arqmq` is **refused on mainnet** unless you also pass
   `--arqnet-allow-experimental` (not recommended for production service nodes).
4. On testnet/stagenet, `arqmq` may be used to exercise the dedicated `SocketStack`.

`get_arqnet_status` reports both:

| Field | Meaning |
|-------|---------|
| `transport` | Facade / native stack (`snnetwork` or `arqmq`) |
| `mesh` | Live peer quorum transport (`snnetwork` while compatibility lock holds) |

## Before upgrading

1. Backup datadir (`~/.arqma` or custom `--data-dir`).
2. Confirm Storage Server is running and reachable (uptime proofs still require it).
3. Note current daemon version with `arqmad --version`.
4. Keep existing Arq-Net ports and SN keys; no mesh protocol flip in this release line.

## Build / install notes

- Toolchain: **C++20**, CMake ≥ **3.16**
- Platforms: **Linux, macOS, Windows** — see [`docs/PLATFORM.md`](PLATFORM.md)
- Recommended verify (native Linux/macOS):
  ```bash
  cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_INTEGRATION_TESTS=OFF
  cmake --build build --parallel --target unit_tests hash-target-tests
  ctest --test-dir build -R 'unit_tests|hash-target' --output-on-failure
  ```
- Windows / cross macOS release binaries: `make depends target=<triplet>`
  (CI matrix in `.github/workflows/depends.yml`).

## New / restored operator knobs

| Flag / RPC | Meaning |
|------------|---------|
| `--arqnet-backend=legacy-arqnet\|arqmq` | Messaging selection (default `legacy-arqnet`; mainnet refuses `arqmq` without override) |
| `--arqnet-allow-experimental` | Permit `arqmq` on mainnet (dev/soak only; peer mesh still SNNetwork) |
| `--arq-router` | Experimental privacy-router scaffold (lives for daemon lifetime) |
| `--storage-client-url=<url>` | Outbound Storage Server reachability probe (`http://` GET / `https://` TCP) |
| `arqnet_ping` | Records Arq-Net reachability (not yet a hard uptime gate) |
| `get_arqnet_status` | `backend`, `transport`, `mesh`, initialized, last ping |
| `get_storage_status` | Storage client scaffold status + last SS ping |
| `get_service_nodes` `offset`/`limit` | Optional pagination |

## Compatibility

- HF19 burn construction now allowed at HF ≥ 19 (core/wallet aligned).
- Arq-Net accepts only registered service-node Curve keys.
- Missing Arq-Net pings do **not** currently block uptime proofs.
- Mainnet quorum wire path is unchanged (SNNetwork).

## Rollback

Revert to the previous release binaries and restore datadir backup if required.
Prefer forward fixes once HF19 burn transactions exist on-network.
