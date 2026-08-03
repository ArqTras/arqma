# ArqMQ Migration Plan

## Goal

Evolve Arq-Net messaging toward a maintainable command/ACL/worker model inspired
by OxenMQ, while preserving Arqma naming, ports and operator UX.

## Current state

`src/arqnet` already provides:

- ZMQ transport + Curve25519 auth
- Worker pool / proxy thread
- bt-style serialization
- Quorum command dispatch (`vote_ob`, `ping`/`pong`)

This is a viable base. Blind replacement with upstream OxenMQ would create
unnecessary branding and history noise.

## Target architecture

```
arqmad
  └── arqnet facade (stable C++ API for cryptonote_core / protocol)
        └── transport backend
              ├── legacy SNNetwork (default during migration)
              └── arqmq backend (feature-flagged)
```

Public names remain **Arq-Net** / **arqnet**. Internal library may be called
`arqmq` in code comments and CMake targets if split out.

## ACL categories (planned)

| Category | Who | Examples |
|----------|-----|----------|
| `None` | rejected | — |
| `Basic` | local admin / loopback helpers | diagnostics |
| `ServiceNode` | registered SN keys | quorum votes, pings |
| `Admin` | local control socket | privileged ops |

Unknown remote Curve keys stay denied (already enforced).

## Migration steps

1. Extract stable facade headers from `SNNetwork` usage sites.
2. Add feature flag `--arqnet-backend=legacy|arqmq` (daemon arg).
3. Vendor OxenMQ **or** port selected subsystems under Arqma copyright/license
   notices without Co-Authored-By trailers.
4. Dual-run on testnet/stagenet.
5. Make `arqmq` default; keep legacy until one stable release cycle.
6. Optionally require Arq-Net ping for uptime proofs after operator notice.

## Non-goals

- Renaming the product mesh to Lokinet/Oxennet
- Breaking Levin P2P
- Shipping Session branding

## Success criteria

- Existing quorum vote relay works unchanged for operators
- Unit/integration tests cover auth deny path and ping RPC
- CI builds both backends when flag is present
- Docs updated in `docs/ARQNET.md` and release notes
