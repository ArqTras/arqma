#!/usr/bin/env python3
# Copyright (c) 2018 - 2026, The Arqma Network
#
# Poll arqmad get_arqnet_status for mesh-shadow soak progress.
# Stdlib only (no third-party deps).
#
# Usage:
#   utils/arqnet-mesh-soak-monitor.py [host:rpc_port] [--interval SEC] [--once]
#
# Exit codes:
#   0  --once and mesh_shadow_parity_sample_ok is true
#   1  usage / RPC error
#   2  --once and parity sample not yet ok

from __future__ import annotations

import argparse
import json
import sys
import time
import urllib.error
import urllib.request


def rpc_call(url: str, method: str) -> dict:
    body = json.dumps({"jsonrpc": "2.0", "id": "0", "method": method, "params": {}}).encode()
    req = urllib.request.Request(
        url,
        data=body,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=15) as resp:
        payload = json.loads(resp.read().decode())
    if "error" in payload:
        raise RuntimeError(payload["error"])
    return payload.get("result") or {}


def fmt_row(result: dict) -> str:
    return (
        f"shadow={result.get('mesh_shadow')} "
        f"ep={result.get('mesh_shadow_endpoint') or '-'} "
        f"live_vote={result.get('mesh_vote_ob_live', 0)} "
        f"sh_ok={result.get('mesh_vote_ob_shadow_ok', 0)} "
        f"sh_fail={result.get('mesh_vote_ob_shadow_fail', 0)} "
        f"sh_in={result.get('mesh_vote_ob_shadow_in', 0)} "
        f"ok_bps={result.get('mesh_shadow_ok_rate_bps', 0)} "
        f"sample_ok={result.get('mesh_shadow_parity_sample_ok')}"
    )


def main() -> int:
    ap = argparse.ArgumentParser(description="Monitor Arq-Net mesh-shadow soak parity via JSON-RPC")
    ap.add_argument(
        "rpc",
        nargs="?",
        default="127.0.0.1:39994",
        help="arqmad RPC host:port (default stagenet 39994)",
    )
    ap.add_argument("--interval", type=float, default=30.0, help="seconds between polls (default 30)")
    ap.add_argument("--once", action="store_true", help="single poll; exit 0 only if parity sample ok")
    args = ap.parse_args()

    if ":" not in args.rpc:
        print("error: rpc must look like host:port", file=sys.stderr)
        return 1
    url = f"http://{args.rpc}/json_rpc"

    while True:
        try:
            result = rpc_call(url, "get_arqnet_status")
        except (urllib.error.URLError, TimeoutError, RuntimeError, json.JSONDecodeError) as exc:
            print(f"RPC error @ {args.rpc}: {exc}", file=sys.stderr)
            if args.once:
                return 1
            time.sleep(args.interval)
            continue

        line = fmt_row(result)
        print(line, flush=True)

        if args.once:
            return 0 if result.get("mesh_shadow_parity_sample_ok") else 2

        if result.get("mesh_shadow_parity_sample_ok"):
            print("parity sample OK — keep soaking; do not flip cutover without multi-hour window", flush=True)

        time.sleep(args.interval)


if __name__ == "__main__":
    sys.exit(main())
