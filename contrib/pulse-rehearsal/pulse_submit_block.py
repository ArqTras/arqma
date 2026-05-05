#!/usr/bin/env python3
# Copyright (c) 2026, The Arqma Network
#
# Submit a mined / assembled block blob via JSON-RPC submit_block (same as pool "submitblock").
# Arqnet quorum (pulse_proposal / pulse_vote) is NOT implemented here — use SN tooling or extend
# your orchestrator to speak the ZMQ/bt-dict wire documented in src/arqnet/pulse_wire.h.

import argparse
import json
import sys
import urllib.error
import urllib.request


def json_rpc(url, method, params=None, timeout=60.0):
    body = {"jsonrpc": "2.0", "id": "0", "method": method}
    if params is not None:
        body["params"] = params
    data = json.dumps(body).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        out = json.loads(resp.read().decode("utf-8"))
    if "error" in out and out["error"]:
        raise RuntimeError(out["error"])
    return out.get("result", {})


def load_hex_blob(path):
    if path == "-":
        raw = sys.stdin.read()
    else:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            raw = f.read()
    s = "".join(c for c in raw.strip() if c not in " \t\r\n")
    if len(s) % 2 != 0:
        raise ValueError("hex blob must have even length")
    return s


def main():
    p = argparse.ArgumentParser(description="Submit block hex via JSON-RPC submit_block.")
    p.add_argument(
        "--url",
        default="http://127.0.0.1:19994/json_rpc",
        help="Daemon JSON-RPC URL",
    )
    p.add_argument(
        "blob_hex_file",
        nargs="?",
        default="-",
        help="File with one continuous hex string (default: stdin)",
    )
    args = p.parse_args()

    try:
        blob_hex = load_hex_blob(args.blob_hex_file)
        r = json_rpc(args.url, "submit_block", [blob_hex])
        print(json.dumps(r, indent=2))
    except (OSError, ValueError) as e:
        print(str(e), file=sys.stderr)
        return 2
    except urllib.error.URLError as e:
        print("HTTP error: %s" % e, file=sys.stderr)
        return 1
    except RuntimeError as e:
        print("RPC error: %s" % e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
