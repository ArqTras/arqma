# Arqma product (one repository)

This tree is the **unified Arqma product**. Consensus, storage, privacy routing,
and messenger helpers ship together. They stay **separate processes** (see
[`PROCESS_BOUNDARIES.md`](PROCESS_BOUNDARIES.md)); they are no longer split
across other repositories.

End users get **one simple path**. Storage, router, hops, and swarm are started
by the stack — they are not steps the messenger user has to assemble.

## Binaries

| Binary | Role |
|--------|------|
| `arqmad` | Consensus daemon (RandomARQ + hybrid Pulse, Arq-Net, RPC) |
| `arqma-wallet-rpc` / `arqma-wallet-cli` | Wallet processes |
| `arqma-storage` | HTTP Storage Server (KV + TTL + `--data-dir` + `--peer` / swarm replicas) |
| `arqma-router` | Privacy-router (`POST /v1/peel`, multi-hop `POST /v1/store`) |
| `arqma-msg` | CLI messenger: `gen` / `send` / `inbox` / `open` |

Blink lives **in-process** as `src/arq_blink` (quorum sign/verify + collector).
`get_blink_status` reports that collector. There is **no** Arq-Net Blink wire yet,
so it does **not** pre-confirm transactions on the network. It does **not** replace
Pulse or RandomARQ.

## Local stack

```text
utils/arqma-stack.sh                 # Linux / macOS
utils/arqma-stack.cmd                # Windows
arqma-msg gen
arqma-msg send <hex> hello
arqma-msg inbox
arqma-msg open
```

The stack starts storage + router and writes `ARQMA_STORAGE_URL` /
`ARQMA_ROUTER_URL` into `$ARQMA_STACK_DIR/env` (default `/tmp/arqma-stack/env`,
Windows `%TEMP%\arqma-stack\env`). `arqma-msg` reads that file, so send/inbox/open
need no `--url` / `--router`. `gen` writes `~/.arqma/msg/identity` so `inbox` and
`open` need no `--to` / `--secret` / `--key`. `send bob=<hex> hello` remembers
`bob` in `~/.arqma/msg/contacts`; later `send bob hello`. `gen` prints only the
public key. Inbox URLs use an opaque id, not the raw pubkey. The stack writes
`ARQMA_STACK_TOKEN`; `arqma-msg` sends it without a new flag. If the router is down,
send stores directly.

Operators still have `--url`, `--router` (repeat, max 3), `swarm --snode`,
`--peer`, and `--storage-url`. Those stay out of the everyday path.

`arqmad --storage-client-url=http://127.0.0.1:22021 --arq-router` is the daemon
probe; consensus stays a separate process.

`PUT /v1/kv?ttl=` expires values (cap 14 days; `0` means keep). Inbox namespaces
`inbox-<pubkey>` also fan out to URLs in `PUT /v1/snodes`. `GET /v1/swarm?pubkey=`
returns the FNV swarm id plus those member URLs. `arqma-msg` get / inbox / open
read from those members when the local node has no copy. HTTP bodies are capped
at 1 MiB; listen addresses accept IPv6 (`[::1]:22021`). `PUT /v1/snodes` merges
unique HTTP member URLs (cap 32) and pushes the list to those members. This is
not a full epidemic gossip protocol.

`get_storage_status` / `storage_server_ping` talk to `arqma-storage`.
`get_blink_status` / `print_blink` report the in-daemon Blink collector.

How to test: [`docs/OPERATOR_UPGRADE.md`](OPERATOR_UPGRADE.md) (unit suite, messenger stack, daemon probes).

## Locks that stay

1. Hybrid PoW + Pulse. RandomARQ remains required. Pulse stage stays **2**.
2. Default `--arqnet-backend=legacy-arqnet`.
3. Storage and router stay out of the `arqmad` process.
4. No Oxen / Session / Lokinet branding or tokenomics copied in.
5. End-user messenger path stays simple (`gen` / `send` / `inbox` / `open`).
