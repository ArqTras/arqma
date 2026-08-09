# Arq-Net Dual-Stack Migration

Plan for evolving from `LegacyArqNet` (`SNNetwork`) to a native `ArqMq`
backend without breaking operators.

## Goals

- Keep `--arqnet-backend=legacy-arqnet` as the default until one stable release cycle
  after the native backend is production-ready.
- Allow `--arqnet-backend=arqmq` on testnet/stagenet first.
- Preserve Arqma naming, ports and Curve25519 SN identity.

## Phases

1. **Facade only (landed)** — `arqmq::init`, ACL helpers, RPC status, fallback.
2. **Command registry (landed)** — map existing quorum/ping commands to ACL categories.
3. **Native transport (landed)** — `arqmq::SocketStack` ZMQ worker path under Arqma naming;
   selected by `--arqnet-backend=arqmq` (`transport=arqmq`). Peer mesh still SNNetwork.
4. **Dual-run coexistence (landed, compatibility mode)** — native stack + SNNetwork mesh
   run together; RPC reports `transport` vs `mesh`; peer wire protocol unchanged.
5. **Cutover** — default flips to `arqmq` mesh only after stagenet parity; legacy retained one release.
6. **Optional hard gate** — Arq-Net ping for uptime proofs after operator notice.

## Operator flags

| Flag | Meaning |
|------|---------|
| `--arqnet-backend=legacy-arqnet` | Current production path (default; **required for mainnet**) |
| `--arqnet-backend=arqmq` | Experimental SocketStack; allowed on testnet/stagenet; mainnet needs `--arqnet-allow-experimental` |
| `--arqnet-allow-experimental` | Explicit mainnet override for `arqmq` (not for production SNs) |

## Hard fork gate (HF20)

Following Monero/Oxen practice, native mesh cutover is tied to a **network
version**, not only an operator flag:

| Net | HF20 height | Notes |
|-----|-------------|-------|
| Stagenet | **240** | Soak / parity first |
| Testnet | **1300** | Intermediate |
| Mainnet | **unscheduled** | Add only after stagenet mesh parity |

Runtime helpers (`arqmq`):

- `hf_permits_native_mesh(hf)` — true at `network_version_20+`
- `native_mesh_implementation_ready()` — false until Curve/ZAP peer relay ported
- `native_mesh_ready_at(hf)` — both conditions (daemon should use this)

Alias: `HF_VERSION_NATIVE_ARQNET_MESH` → `network_version_20`.

## Exit criteria before default flip

- [x] Native backend initializes on Linux CI (unit coverage for `SocketStack`)
- [x] Dual-run coexistence with peer mesh locked on SNNetwork (full wire compatibility)
- [x] Deny path for unknown Curve peers covered by unit tests (`arqnet_auth` /
      `decide_incoming_curve_peer*`; full ZAP two-process integration still later)
- [x] HF20 scaffold (stagenet/testnet heights; mainnet height deferred)
- [ ] Quorum vote relay parity verified on stagenet (native mesh path;
      `native_mesh_implementation_ready()` remains false until Curve/ZAP ported)
- [ ] Schedule mainnet HF20 height + release notes / `OPERATOR_UPGRADE.md` for cutover
