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
2. **Command registry** — map existing quorum/ping commands to ACL categories.
3. **Native transport** — real ArqMQ socket/worker path under Arqma copyright.
4. **Dual-run** — both backends can be built; integration tests cover deny/auth.
5. **Cutover** — default flips to `arqmq`; legacy retained one release.
6. **Optional hard gate** — Arq-Net ping for uptime proofs after operator notice.

## Operator flags

| Flag | Meaning |
|------|---------|
| `--arqnet-backend=legacy-arqnet` | Current production path (default) |
| `--arqnet-backend=arqmq` | Experimental; falls back if unsupported |

## Exit criteria before default flip

- [ ] Native backend initializes on Linux CI
- [ ] Quorum vote relay parity verified on stagenet
- [ ] Deny path for unknown Curve peers covered by integration test
- [ ] Release notes + `docs/OPERATOR_UPGRADE.md` updated
