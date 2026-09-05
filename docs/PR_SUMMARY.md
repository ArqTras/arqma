# PR summary — `upgrade` branch (Milestone A/B + C start)

Bilingual operator/reviewer notes for this release line. Code identifiers stay
English.

---

## English

### What this release is

A production-grade modernization of Arqma (C++20, 3-OS CI, ArqMQ/`SocketStack`)
that **keeps current mainnet consensus until height 4 000 000**, then runs a
**hybrid service-node window (HF20)**, then **new-style exclusive SN operation
(HF21 at 5 000 000)**. Milestone C (Pulse POSPOW) is started in-tree: leader
election and hard-fork gates are live; Pulse is **not** yet allowed to replace
RandomARQ, so the chain cannot stall.

### Benefits of the new design

- **No surprise fork today.** v19 wire and miner PoW remain until 4 000 000.
- **Hybrid SN (HF20).** Miners keep producing RandomARQ blocks while service
  nodes begin Pulse rounds and native Arq-Net mesh (opt-in `--arqnet-backend=arqmq`).
- **Exclusive SN (HF21).** One target style: Pulse + native mesh. PoW stays
  mandatory until the Pulse producer is wired (`pulse-block-producer-unwired`).
- **Votes cannot be dropped** if a node is still on legacy Arq-Net: SNNetwork is
  the fallback whenever CURVE SocketStack is missing.
- **Observable.** Operators can query mesh parity and Pulse quorum without
  reading logs.

### Hard-fork schedule

| Network | HF20 (hybrid mesh + POSPOW permit) | HF21 (exclusive new style) |
|---------|-------------------------------------|----------------------------|
| Stagenet | 240 | 260 |
| Testnet | 1300 | 1400 |
| Mainnet | **4 000 000** | **5 000 000** |

### Changes, features, and RPC (by area)

**Consensus / SN**

- `network_version_21` / `HF_VERSION_PULSE_EXCLUSIVE` / `MAINNET_HARD_FORK_21_HEIGHT`.
- Pulse module (`src/cryptonote_core/pulse.*`): `legacy` → `hybrid` (HF20) →
  `exclusive` (HF21). Deterministic leader + 11-wide quorum from height and SN
  count.
- Block validation still requires RandomARQ while `pow_replacement_ready==false`.

**Arq-Net mesh**

- Stage 4 cutover gate on (`native_mesh_blocker=none`).
- HF20: native mesh live only with `arqmq` + CURVE; otherwise SNNetwork.
- HF21: exclusive native mesh **intended**; missing CURVE stack still falls back
  to SNNetwork and logs a warning.
- Soak inbound `vote_ob` parse counters; CURVE `ping`→`pong`.

**RPC queries (unrestricted daemon JSON-RPC)**

| Method | New / changed fields | Purpose |
|--------|----------------------|---------|
| `get_arqnet_status` | `mesh`, `mesh_shadow_*`, `mesh_vote_ob_shadow_parse_*`, `native_mesh_ready`, `native_mesh_hf_permits`, `native_mesh_blocker`, `native_mesh_exclusive`, `sn_operating_mode`, `pulse_pow_required`, `pulse_blocker`, `hard_fork_version` | Live mesh vs capability; Pulse/PoW gate |
| `get_pulse_status` | **new** — `sn_operating_mode`, `hybrid_permitted`, `exclusive_required`, `pow_required`, `pow_replacement_ready`, `pulse_blocker`, `leader_index`, `quorum_indices`, `service_node_count`, `height` | Hybrid/exclusive SN status and this height’s Pulse quorum |
| `get_storage_status` | existing scaffold | Storage Server reachability |
| `arqnet_ping` | existing | Arq-Net reachability (not a hard uptime gate) |

**Operator flags**

- `--arqnet-backend=legacy-arqnet\|arqmq` (default legacy; mainnet `arqmq` needs `--arqnet-allow-experimental` until operators opt in).
- `--arqnet-mesh-shadow` — dual-write soak on ANET+10000.
- Default backend is **not** flipped in this release.

**Not in this release (still Milestone C follow-ups)**

- Full Pulse block producer / signatures (would turn off PoW at HF21).
- Production Storage Server binary, Blink, Lokinet-class router, Session clients.

### Soak / ops

```text
arqmad --stagenet --arqnet-backend=arqmq --arqnet-mesh-shadow
utils/arqnet-mesh-soak-monitor.py 127.0.0.1:39994
```

Watch `get_pulse_status` after stagenet 240 (`hybrid`) and 260 (`exclusive`).

---

## Polski

### Czym jest ten release

Modernizacja Arqmy (C++20, CI na 3 OS, ArqMQ/`SocketStack`) **bez zmiany
konsensusu mainnetu do bloku 4 000 000**. Potem **hybrydowe SN (HF20)**, a od
**HF21 (5 000 000)** docelowo **wyłącznie nowy styl**. Milestone C (Pulse /
POSPOW) jest **rozpoczęty**: wybór lidera i bramki forka działają; Pulse **nie**
zastępuje jeszcze RandomARQ, więc łańcuch nie zatrzyma się na wysokości forka.

### Korzyści

- **Brak niespodziewanego forka dziś.** Do 4 000 000 zostaje v19 i miner PoW.
- **Hybryda SN (HF20).** Minery dalej robią bloki RandomARQ, a service node’y
  startują rundy Pulse i native mesh (opt-in `--arqnet-backend=arqmq`).
- **Wyłącznie nowy styl (HF21).** Pulse + native mesh. PoW zostaje obowiązkowe,
  dopóki producent Pulse nie jest podpięty (`pulse-block-producer-unwired`).
- **Głosy nie giną** na węźle legacy: SNNetwork jest fallbackiem bez CURVE.
- **Obserwowalność.** Mesh i quorum Pulse widać przez RPC, bez logów.

### Harmonogram hardforków

| Sieć | HF20 (hybryda mesh + POSPOW) | HF21 (wyłącznie nowy styl) |
|------|------------------------------|----------------------------|
| Stagenet | 240 | 260 |
| Testnet | 1300 | 1400 |
| Mainnet | **4 000 000** | **5 000 000** |

### Zmiany, funkcje i zapytania RPC

**Konsensus / SN**

- `network_version_21`, wysokość mainnet **5 000 000**.
- Moduł Pulse: tryby `legacy` → `hybrid` (HF20) → `exclusive` (HF21). Lider i
  quorum 11 z wysokości i liczby SN.
- Walidacja bloku nadal wymaga RandomARQ, dopóki `pow_replacement_ready==false`.

**Mesh Arq-Net**

- Bramka stage 4 włączona.
- HF20: native mesh tylko przy `arqmq` + CURVE; inaczej SNNetwork.
- HF21: exclusive native mesh; bez CURVE — fallback SNNetwork + ostrzeżenie.
- Soak: parse `vote_ob`, `ping`→`pong`.

**Zapytania RPC (nieskrępowany JSON-RPC daemona)**

| Metoda | Pola | Po co |
|--------|------|--------|
| `get_arqnet_status` | mesh, cienie soak, `native_mesh_*`, `sn_operating_mode`, `pulse_*` | Nośnik quorum i bramka PoW |
| `get_pulse_status` | **nowe** — tryb SN, czy hybryda/exclusive, czy PoW, lider, indeksy quorum | Status POSPOW na bieżącej wysokości |
| `get_storage_status` | bez zmian szkieletu | Ping Storage Server |
| `arqnet_ping` | bez zmian | Zasięg Arq-Net (nie twarda bramka uptime) |

**Flagi operatora** — jak wyżej; default backendu **bez zmian**.

**Poza tym release’em** — pełny producent bloków Pulse, Storage Server, Blink,
router, klienty Session.

### Soak

Jak w części angielskiej. Po stagenet 240 tryb `hybrid`, po 260 `exclusive`.
