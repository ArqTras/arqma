#!/usr/bin/env python3
# Copyright (c) 2026, The Arqma Network
#
# Submit a mined / assembled block blob via JSON-RPC submit_block (same as pool "submitblock").
# Arqnet quorum (pulse_proposal / pulse_vote) is NOT implemented here — use SN tooling or extend
# your orchestrator to speak the ZMQ/bt-dict wire documented in src/arqnet/pulse_wire.h.

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import urllib.error

from pulse_tools_common import json_rpc


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
