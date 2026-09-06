## Summary

Production-grade modernization of Arqma (C++20, 3-OS CI, ArqMQ/`SocketStack`) that **keeps current mainnet consensus until height 4 000 000**, then runs a **hybrid service-node window (HF20)**, then **exclusive native mesh (HF21 at 5 000 000)**. Milestone C Pulse is **wired as a hybrid producer**: service nodes sign, gossip, and retransmit `pulse_rnd` rounds (15s wait-windows, leader rotation). **RandomARQ stays required at HF20 and HF21** — Pulse does not replace PoW (`pow_replacement_ready` stays false; no stage-3 / PoW-off).

Operator notes stay in this PR description (EN + PL). Existing docs (`docs/OPERATOR_UPGRADE.md`, `docs/UPGRADE_ROADMAP.md`, `docs/ARQNET_DUAL_STACK.md`) are updated in place.

Companion stack binaries and messenger CLI exist in-tree for local operator probes; they are **secondary** to the consensus / mesh work and are **not** a Session-class UX deliverable in this PR. See `docs/PRODUCT.md` for the short inventory.

## Benefits

- **No surprise fork today.** v19 wire and miner PoW remain until 4 000 000.
- **Hybrid SN (HF20).** Miners keep producing RandomARQ blocks while service nodes run Pulse rounds and native Arq-Net mesh (opt-in `--arqnet-backend=arqmq`).
- **Exclusive mesh (HF21).** Intended native mesh for operators. Block production stays **hybrid**: RandomARQ **and** Pulse. Pulse does not replace mining.
- **Votes cannot be dropped** if a node is still on legacy Arq-Net: SNNetwork is the fallback whenever CURVE SocketStack is missing.
- **Observable.** Operators can query mesh parity (`vote_ob` + `pulse_rnd`), Pulse quorum, Storage reachability, and Blink collector without reading logs.

## Hard-fork schedule

| Network | HF20 (hybrid mesh + Pulse permit) | HF21 (exclusive mesh; hybrid PoW) |
|---------|-------------------------------------|-----------------------------------|
| Stagenet | 240 | 260 |
| Testnet | 1300 | 1400 |
| Mainnet | **4 000 000** | **5 000 000** |

## Mainnet compatibility locks (until 4 000 000)

- Default `--arqnet-backend=legacy-arqnet`
- Mainnet `arqmq` needs `--arqnet-allow-experimental`
- Quorum/`vote_ob` stays SNNetwork unless HF20+ **and** CURVE `arqmq`
- Default backend is **not** flipped in this PR
- Curated unit suite **628** tests on tip

## Changes, features, and RPC

**Consensus / SN**

- `network_version_21` / `HF_VERSION_PULSE_EXCLUSIVE` / `MAINNET_HARD_FORK_21_HEIGHT`
- Pulse module (`src/cryptonote_core/pulse.*`): `legacy` → `hybrid` (HF20) → `exclusive` (HF21, **mesh**). Deterministic leader + 11-wide quorum
- Miner-tx extra (`TX_EXTRA_TAG_PULSE`) only after a **majority certificate**: exactly 7 of 11, lowest `validator_index` slots, strictly increasing order. `extra.round` must match the parent→block 15s wait-window. Signatures bind to a **miner payload hash** (timestamp + included tx hashes + miner vout[0]); idle `pulse_rnd` may sign the round with a null payload, but miner extra requires a non-null payload. The certificate is attached **before** coinbase weight padding so extra stays parseable and weight-locked. Quorum members co-sign inbound payload votes. `get_pulse_status.signature_count` is the real collector total (no synthetic +1); `local_signature_ready` is true only when this node’s vote is collected
- 15s wait-windows (`round` 0–7, covering the 120s block target); idle-loop `pulse_rnd` every 5s with retransmit
- `pulse_rnd` wire **v2** (146 bytes, includes payload hash); v1 114-byte blobs are rejected
- Block validation still requires RandomARQ (`pow_replacement_ready==false`; blocker `pulse-pow-replacement-not-ready`)

**Arq-Net mesh**

- Stage 4 cutover gate on (`native_mesh_blocker=none`)
- HF20: native mesh live only with `arqmq` + CURVE; otherwise SNNetwork
- HF21: exclusive native mesh **intended**; missing CURVE stack still falls back to SNNetwork
- Soak inbound `vote_ob` **and** `pulse_rnd` parse counters; CURVE `ping`→`pong`
- `mesh_shadow_parity_sample_ok` still keys off `vote_ob` (Pulse gossip starts at HF20)
- Blink mesh `blink_tx` shadow parse counters are observability-only (do not replace Pulse/PoW)

**RPC (unrestricted daemon JSON-RPC)**

| Method | New / changed fields | Purpose |
|--------|----------------------|---------|
| `get_arqnet_status` | `mesh`, `mesh_shadow_*`, `mesh_vote_ob_*`, `mesh_pulse_rnd_*`, `mesh_blink_tx_*`, `native_mesh_*`, `sn_operating_mode`, `pulse_pow_required`, `pulse_blocker`, `hard_fork_version` | Live mesh vs capability; Pulse/PoW gate; soak telemetry |
| `get_pulse_status` | **new** — mode, wait-window `round`, leader, quorum, `signature_count`, `majority_ok`, `certificate_ready`, hex `payload_hash`, honest `local_signature_ready`, PoW gates | Hybrid Pulse round status |
| `get_blink_status` / `print_blink` | quorum 10, majority 7, `wire_connected`, `mesh_blink_tx_*`; does not replace Pulse/PoW | Local collector + mesh soak counters |
| `get_block_header_*` | `pulse_certificate`, `pulse_round`, `pulse_signature_count`, `pulse_payload_hash` | Pulse extra on a produced block |
| `get_storage_status` / `print_storage` | reachability + gossip/catalog counters from companion storage `GET /status` | Optional operator probe |
| `arqnet_ping` | existing | Arq-Net reachability (not a hard uptime gate) |

**Operator flags / console**

- `--arqnet-backend=legacy-arqnet|arqmq` (default legacy; mainnet `arqmq` needs `--arqnet-allow-experimental`)
- `--arqnet-mesh-shadow` — dual-write soak on ANET+10000
- `--storage-client-url` — optional probe of companion storage
- Daemon `print_pulse` / `print_blink` / `print_storage`

### Companion probes (secondary)

In-tree helpers for local soak/dev only — **not** the focus of this PR and **not** Session-class messenger UX:

| Binary | Role |
|--------|------|
| `arqma-storage` | HTTP KV + TTL + anti-entropy gossip / membership |
| `arqma-router` | Privacy-router peel/store (max 3 hops) |
| `arqma-msg` | Minimal CLI probe (`gen` / `send` / `inbox` / `open`) |

```text
utils/arqma-stack.sh   # or utils/arqma-stack.cmd
arqmad --storage-client-url=http://127.0.0.1:22021
```

## Test plan

Copy-paste steps also live in `docs/OPERATOR_UPGRADE.md`. Tip **628** unit tests.

### Unit suite (Linux / macOS)

```bash
cmake -S . -B build/upgrade-test -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTS=ON \
  -DBUILD_INTEGRATION_TESTS=OFF
cmake --build build/upgrade-test --parallel \
  --target unit_tests daemon
build/upgrade-test/tests/unit_tests/unit_tests
```

Expect `[  PASSED  ] 628 tests.` Optional: `ctest --test-dir build/upgrade-test -R 'unit_tests|hash-target' --output-on-failure`.

### Daemon probes

```bash
build/upgrade-test/bin/arqmad --storage-client-url=http://127.0.0.1:22021
# get_pulse_status  — hybrid, pow_replacement_ready=false
# get_blink_status  — observability only
# get_arqnet_status — mesh=snnetwork on default backend
# print_pulse / print_blink / print_storage
```

Keep `--arqnet-backend=legacy-arqnet` on mainnet SNs.

### Checklist

- [x] Local `unit_tests` (**628**)
- [x] Local loopback mesh / `vote_ob` / `pulse_rnd` soak (inbound parse included)
- [x] Storage status telemetry + Blink `blink_tx` mesh unit tests
- [ ] CI unit + depends on current tip
- [ ] Operator: mainnet SN keeps default flags until 4 000 000; CLSAG wallets ship with the daemon
- [ ] Optional live stagenet soak (when a quorum exists)

## Deferred (not this PR)

- Pulse PoW-off / stage 3 (`k_pulse_pow_stage` stays 2; hybrid policy)
- Full Storage swarm gossip protocol
- Session-class client UX / polished messenger product
- Default `--arqnet-backend` flip after a later operator notice
- Live stagenet soak re-run; `core_tests` under `BUILD_INTEGRATION_TESTS`

---

## Polski

### Czym jest ten release

Modernizacja Arqmy (C++20, CI na 3 OS, ArqMQ/`SocketStack`) **bez zmiany konsensusu mainnetu do bloku 4 000 000**. Potem **hybrydowe SN (HF20)**, a od **HF21 (5 000 000)** docelowo **wyłącznie native mesh**. Milestone C (Pulse) jest **podpięty jako producent hybrydowy**: service node’y podpisują, plotkują i retransmitują rundy `pulse_rnd` (okna 15 s, rotacja lidera). **RandomARQ zostaje obowiązkowe na HF20 i HF21** — Pulse **nie** zastępuje PoW (`pow_replacement_ready` zostaje false; bez stage 3 / wyłączenia kopania).

Notatki dla operatorów są **w tym opisie PR** (EN + PL). Binarki towarzyszące i CLI messengera są w drzewie jako **pomocnicze sondy operatorskie** — **nie** są głównym celem tego PR i **nie** stanowią Session-class UX. Zobacz `docs/PRODUCT.md`.

### Korzyści

- **Brak niespodziewanego forka dziś.** Do 4 000 000 zostaje v19 i miner PoW.
- **Hybryda SN (HF20).** Minery dalej robią bloki RandomARQ, a service node’y prowadzą rundy Pulse i native mesh (opt-in `--arqnet-backend=arqmq`).
- **Wyłącznie mesh (HF21).** Native mesh jako docelowy styl operatora. Produkcja bloków zostaje **hybrydowa**: RandomARQ **i** Pulse. Pulse nie zastępuje kopania.
- **Głosy nie giną** na węźle legacy: SNNetwork jest fallbackiem bez CURVE.
- **Obserwowalność.** Mesh (`vote_ob` + `pulse_rnd`), quorum Pulse, Storage i Blink widać przez RPC, bez logów.

### Harmonogram hardforków

| Sieć | HF20 (hybryda mesh + Pulse) | HF21 (wyłącznie mesh; hybryda PoW) |
|------|------------------------------|-------------------------------------|
| Stagenet | 240 | 260 |
| Testnet | 1300 | 1400 |
| Mainnet | **4 000 000** | **5 000 000** |

### Zmiany, funkcje i RPC

- Konsensus: `network_version_21`, Pulse `legacy` → `hybrid` → `exclusive` (**mesh**), lider + quorum 11, extra `TX_EXTRA_TAG_PULSE` dopiero po **certyfikacie większości 7/11**. RandomARQ obowiązkowe.
- Mesh: stage 4 włączony; HF20 native tylko przy `arqmq`+CURVE; HF21 exclusive z fallbackiem SNNetwork. Soak: parse `vote_ob` **i** `pulse_rnd` (+ `blink_tx` tylko observability).
- RPC: `get_arqnet_status`, `get_pulse_status`, `get_blink_status` / `print_blink`, `get_storage_status` / `print_storage`, pola Pulse na nagłówku bloku, `print_pulse`.
- Default `--arqnet-backend` **bez zmian**. Suite **628** testów.
- Pomocniczo (drugorzędne): `arqma-storage` / `arqma-router` / `arqma-msg` do lokalnych probe’ów — bez obietnicy Session UX w tym PR.

### Testy

Instrukcje: `docs/OPERATOR_UPGRADE.md`. Tip **628** testów.

```bash
cmake -S . -B build/upgrade-test -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_INTEGRATION_TESTS=OFF
cmake --build build/upgrade-test --parallel --target unit_tests daemon
build/upgrade-test/tests/unit_tests/unit_tests
```

- [x] Lokalne `unit_tests` (**628**)
- [x] Lokalny loopback soak mesh / Pulse
- [x] Telemetria storage + testy mesh `blink_tx`
- [ ] CI unit + depends na bieżącym tipie
- [ ] Operator: default flagi do 4 000 000; portfele CLSAG razem z daemonem
- [ ] Opcjonalny soak stagenet na żywym quorum

### Poza tym PR

Wyłączenie PoW (stage 3), pełny protokół gossip swarma Storage, **Session-class UX / dopieszczony messenger**, flip default `--arqnet-backend`.
