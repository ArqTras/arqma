# Process vs Separate Binary Boundaries

Decision record for upgrade-module placement.

## Keep in-process (`arqmad`)

| Component | Why |
|-----------|-----|
| `arqmq` facade + ACL registry + framing limits | Thin control plane over existing SNNetwork |
| `arq_storage::StorageClient` | Client stubs / RPC status / reachability probes only |
| RPC validation / pagination / DoS caps / `rpc_auth` | Daemon request path |
| Messaging envelope / sealed-box / onion peel helpers | Shared crypto primitives for later wire format |
| Wallet-rpc validation + restricted auth | Operator-facing wallet process (`arqma-wallet-rpc`) |

## Separate binary / service

| Component | Why |
|-----------|-----|
| Storage Server | Replication, swarm sync, disk/IO isolation, independent release |
| Privacy router (`arq_router` production) | Network stack lifecycle separate from consensus daemon |
| Messenger clients | UX / mobile / desktop product surfaces |

## Reachability vs full APIs

| Probe | In-process meaning |
|-------|--------------------|
| Storage TCP / cleartext HTTP GET | Daemon can report endpoint liveness without embedding Storage Server |
| Storage TLS | TCP connect only until a TLS client is deliberately linked |
| Arq-Net ping/pong | Mesh liveness; not a substitute for Storage Server |

## Rule of thumb

If a feature needs its own process supervision, storage volume, or upgrade cadence
independent of consensus, it stays out of `arqmad`. Facades and status RPCs may
live in-process as long as they do not pretend to be the full service.
