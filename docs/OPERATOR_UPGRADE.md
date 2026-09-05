# Operator Upgrade Guide (`upgrade` branch)

## Mainnet policy (production)

This branch is prepared for **mainnet** operators. Compatibility locks:

1. **Default** `--arqnet-backend=legacy-arqnet` — do not change on mainnet SNs.
2. **Peer mesh** (`vote_ob` / quorum Curve+ZMQ) stays on `arqnet::SNNetwork` unless
   **HF20+** and this node started `--arqnet-backend=arqmq` with CURVE identity
   (`native_mesh_live_at`). Mainnet HF20 is at height **4 000 000**, so until then
   the chain stays v19 and live mesh stays SNNetwork.
3. `--arqnet-backend=arqmq` is **refused on mainnet** unless you also pass
   `--arqnet-allow-experimental` (not recommended for production service nodes).
4. On testnet/stagenet, `arqmq` may be used to exercise the dedicated `SocketStack`.
5. Stagenet soak (shadow dual-write; live mesh SNNetwork until HF20 on `arqmq` nodes):

```text
arqmad --stagenet --arqnet-backend=arqmq --arqnet-mesh-shadow
```

   Watch `get_arqnet_status`:

   | Field | Meaning |
   |-------|---------|
   | `mesh_live_relays` / `mesh_vote_ob_live` | SNNetwork relays observed |
   | `mesh_vote_ob_shadow_ok` / `*_fail` | SocketStack dual-write results |
   | `mesh_vote_ob_shadow_in` | inbound shadow `vote_ob` frames (receivers) |
   | `mesh_vote_ob_shadow_parse_ok` / `*_parse_fail` | inbound payloads that decode as obligation-vote wire |
   | `mesh_pulse_rnd_live` / `mesh_pulse_rnd_shadow_*` | Pulse `pulse_rnd` dual-write + inbound parse (observability; not in `sample_ok`) |
   | `mesh_shadow_ok_rate_bps` | overall shadow success (0–10000) |
   | `mesh_shadow_parity_sample_ok` | ≥32 live `vote_ob` + ≥95% shadow send ok **and** inbound parse ok |
   | `native_mesh_ready` / `native_mesh_blocker` | implementation gate (stage 4 → `none`) |
   | `native_mesh_hf_permits` / `hard_fork_version` | HF20+ permit on this chain |
   | `mesh` | **live** quorum carrier (`arqmq` only at HF20+ with CURVE `arqmq` stack) |

   Soak re-check (implementation gate is already 4; confirm on a live SN quorum):

   1. ≥2 stagenet SNs run `arqmq` + mesh-shadow (legacy-only peers inflate fails).
   2. Open firewall for **ANET port + 10000** (CURVE listener). Before HF20, live
      mesh stays on ANET; after HF20, `arqmq` nodes send quorum on ANET+10000.
   3. `mesh_shadow_endpoint` populated; watch `mesh_vote_ob_shadow_in` **and**
      `mesh_vote_ob_shadow_parse_ok` on receivers (`parse_fail` must stay near 0).
      After HF20, also watch `mesh_pulse_rnd_shadow_in` / `mesh_pulse_rnd_shadow_parse_ok`
      (`pulse_parse_fail` near 0). `mesh_shadow_parity_sample_ok` still keys off `vote_ob`.
   4. `mesh_shadow_parity_sample_ok == true` across a multi-hour quorum window
      (requires both outbound send success and inbound vote-wire parse).
   5. No consensus / uptime regressions vs SNNetwork-only control nodes.

   Implementation cutover is already flipped:

```text
# src/arqmq/mesh_bridge.cpp
constexpr int k_native_mesh_port_stage = 4;
```

   Live `primary_mesh_send_to_peer` still requires HF20+ **and** `--arqnet-backend=arqmq`
   with CURVE keys. Do **not** change mainnet default `--arqnet-backend` in this release.

   Do **not** enable shadow on mainnet production SNs.

   Monitor soak progress (stdlib Python):

```text
utils/arqnet-mesh-soak-monitor.py 127.0.0.1:39994
utils/arqnet-mesh-soak-monitor.py 127.0.0.1:39994 --once   # exit 0 when sample_ok
# Prints mesh-shadow parity plus Pulse round / signature_count (unrestricted RPC).
```

### HF20 / HF21 (native mesh + hybrid then exclusive SN)

| Network | HF20 | HF21 | Role |
|---------|------|------|------|
| Stagenet | 240 | 260 | Hybrid then exclusive (short window) |
| Testnet | 1300 | 1400 | Intermediate |
| Mainnet | **4 000 000** | **5 000 000** | v19 until 4M; hybrid POSPOW 4M–5M; exclusive after 5M |

- **Until 4 000 000:** major version 19, SNNetwork, miner PoW. Full compatibility with current nodes.
- **HF20 (hybrid):** miner RandomARQ **and** Pulse SN rounds are both permitted. Native mesh is live on `--arqnet-backend=arqmq` nodes; others stay SNNetwork.
- **HF21 (exclusive mesh):** intended native mesh for operators. Block production stays **hybrid**: miner RandomARQ **and** Pulse SN rounds. `get_pulse_status.pow_replacement_ready` stays false — Pulse does not replace PoW. From HF20, active SNs in the Pulse quorum sign and gossip `pulse_rnd` (idle loop + miner template). Miner extra is a **majority certificate** (exactly 7 of 11, lowest validator slots, increasing index) bound to the miner payload hash (timestamp, included txs, miner vout[0]); idle `pulse_rnd` votes may omit the payload. Extra `round` must match the parent→block 15s wait-window. If a round has no majority after **15 seconds**, the leader rotates (`round` 0–7, covering one 120s block target).

Until mainnet height 4 000 000, upgraded daemons still produce **major version 19**
blocks (they only vote 20 in `minor_version`). Pre-HF20 peers accept those blocks.
At 4 000 000, upgraded nodes require major version 20 — operators must upgrade
before that height. At 5 000 000 they must be ready for exclusive SN operation.

Default `--arqnet-backend=legacy-arqnet` stays in this release. Pass `--arqnet-backend=arqmq`
before HF21 so exclusive mesh has a CURVE stack.

`get_arqnet_status` reports both:

| Field | Meaning |
|-------|---------|
| `transport` | Facade / native stack (`snnetwork` or `arqmq`) |
| `mesh` | Live peer quorum transport (`snnetwork` until HF20+ with a CURVE `arqmq` stack) |

## Before upgrading

1. Backup datadir (`~/.arqma` or custom `--data-dir`).
2. Confirm Storage Server is running and reachable (uptime proofs still require it).
3. Note current daemon version with `arqmad --version`.
4. Keep existing Arq-Net ports and SN keys; no live mesh protocol flip on mainnet
   until height **4 000 000**, and only on nodes that opt into `--arqnet-backend=arqmq`.

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
| `--arqnet-allow-experimental` | Permit `arqmq` / mesh-shadow on mainnet (dev/soak only; peer mesh still SNNetwork) |
| `--arqnet-mesh-shadow` | Dual-write peer commands onto SocketStack (needs `arqmq`; default off) |
| `--arq-router` | Hold experimental router lifecycle in the daemon (companion `arqma-router` is the HTTP process) |
| `--storage-client-url=<url>` | Outbound Storage Server reachability probe (`http://` GET / `https://` TCP) — point at `arqma-storage` |
| `arqnet_ping` | Records Arq-Net reachability (not yet a hard uptime gate) |
| `get_arqnet_status` | `backend`, `transport`, `mesh`, shadow counters (`vote_ob` + `pulse_rnd`), `native_mesh_*`, `sn_operating_mode`, `pulse_*` |
| `get_pulse_status` | Hybrid/exclusive SN mode, PoW gate, Pulse `round` / leader / quorum / collector `signature_count` / `majority_ok` / `certificate_ready` / `payload_hash` / `local_signature_ready` |
| `print_pulse` | Daemon console dump of `get_pulse_status` |
| `get_blink_status` / `print_blink` | In-daemon Blink quorum (7 of 10). Does not replace Pulse or RandomARQ |
| `get_block_header_*` | Optional `pulse_certificate`, `pulse_round`, `pulse_signature_count`, `pulse_payload_hash` when miner extra has Pulse |
| `get_storage_status` | Storage client status + last SS ping (`arqma-storage`) |
| `get_service_nodes` `offset`/`limit` | Optional pagination |

Companion processes (same repo, separate PIDs — [`docs/PRODUCT.md`](PRODUCT.md)):

```text
utils/arqma-stack.sh
utils/arqma-stack.cmd
arqmad --storage-client-url=http://127.0.0.1:22021 --arq-router
arqma-msg gen
arqma-storage --listen 127.0.0.1:22021 --data-dir ~/.arqma/storage --peer http://127.0.0.1:22022
arqma-router --listen 127.0.0.1:1090 --data-dir ~/.arqma/arq-router --storage-url http://127.0.0.1:22021
arqma-msg send --router http://127.0.0.1:1090 --to <64-hex> --text hello
arqma-msg inbox --url http://127.0.0.1:22021 --to <64-hex>
```

`arqma-storage` honors `PUT /v1/kv?ttl=` (max 14 days). Inbox keys also copy to
URLs listed in `PUT /v1/snodes?pubkey=`. `GET /v1/swarm?pubkey=` returns swarm id
plus those members. `arqma-msg` get / inbox / open also read those members when
the node in `--url` has no local copy. Listen on IPv6 as `[::1]:22021`. HTTP
bodies are capped at 1 MiB on Linux, macOS, and Windows.

## Compatibility

- HF19 burn construction now allowed at HF ≥ 19 (core/wallet aligned).
- Arq-Net accepts only registered service-node Curve keys.
- Missing Arq-Net pings do **not** currently block uptime proofs.
- Mainnet quorum wire path stays SNNetwork until HF20 on `arqmq` nodes; HF21 is exclusive native mesh when CURVE is configured.

## Rollback

Revert to the previous release binaries and restore datadir backup if required.
Prefer forward fixes once HF19 burn transactions exist on-network.
