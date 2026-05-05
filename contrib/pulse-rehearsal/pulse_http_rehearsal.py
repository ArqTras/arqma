#!/usr/bin/env python3
# Copyright (c) 2026, The Arqma Network
#
# HTTP JSON-RPC rehearsal for Pulse tooling: get_info, get_pulse_block_template,
# get_pulse_arqnet_votes. Requires a daemon with PoS telemetry / Pulse RPC enabled
# (see summary-pos.md). For submit_block use pulse_submit_block.py in this directory.
# Arqnet quorum (ZMQ SN transport) is out of scope here.

import argparse
import sys
from pathlib import Path

# Allow `from pulse_tools_common import …` when run as a script.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import urllib.error

from pulse_tools_common import json_rpc


def main():
    p = argparse.ArgumentParser(description="Pulse HTTP JSON-RPC rehearsal (stdlib only).")
    p.add_argument(
        "--url",
        default="http://127.0.0.1:19994/json_rpc",
        help="Daemon JSON-RPC URL (default mainnet RPC port)",
    )
    p.add_argument("--info", action="store_true", help="Call get_info and print PoS/Pulse fields")
    p.add_argument(
        "--pulse-template",
        action="store_true",
        help="Call get_pulse_block_template (needs FORK_ACTIVE + v20 ideal HF on daemon)",
    )
    p.add_argument("--pulse-round", type=int, default=0, help="pulse_round param (0..255)")
    p.add_argument("--validator-bitset", type=int, default=1, help="validator_bitset param")
    p.add_argument(
        "--pulse-random-hex",
        default="",
        help="Optional 32 hex chars (16 bytes) pulse_random_value",
    )
    p.add_argument(
        "--merge-arqnet-votes",
        action="store_true",
        help="Set merge_arqnet_votes true",
    )
    p.add_argument(
        "--dump-template-hex",
        metavar="PATH",
        default="",
        help="With --pulse-template, write blocktemplate_blob hex to file (- = stdout)",
    )
    p.add_argument(
        "--votes-for-hash",
        default="",
        metavar="HEX64",
        help="After other steps, call get_pulse_arqnet_votes with this 64-char block hash",
    )
    args = p.parse_args()

    try:
        if args.info:
            r = json_rpc(args.url, "get_info")
            keys = (
                "pos_fork_active",
                "pos_planned_hf_name",
                "pos_planned_fork_height",
                "pos_pulse_blocks_since_fork",
                "pos_pulse_next_round_wire_hint",
                "pos_pulse_cum_diff_uses_60s_lwma",
                "pos_pulse_arqnet_vote_buffer_blocks",
            )
            print("get_info (subset):")
            for k in keys:
                if k in r:
                    print(f"  {k}: {r[k]}")

        if args.pulse_template:
            params = {
                "pulse_round": int(args.pulse_round),
                "validator_bitset": int(args.validator_bitset),
            }
            if args.pulse_random_hex:
                params["pulse_random_value"] = args.pulse_random_hex
            if args.merge_arqnet_votes:
                params["merge_arqnet_votes"] = True
            tpl_r = json_rpc(args.url, "get_pulse_block_template", params)
            print("get_pulse_block_template (subset):")
            for k in (
                "height",
                "pulse_template_block_hash",
                "merged_arqnet_vote_count",
                "pulse_signature_threshold",
                "merged_pulse_signatures_meet_threshold",
                "status",
            ):
                if k in tpl_r:
                    print(f"  {k}: {tpl_r[k]}")

            if args.dump_template_hex:
                blob = tpl_r.get("blocktemplate_blob", "")
                if not blob:
                    print("No blocktemplate_blob in response", file=sys.stderr)
                    return 2
                out_path = args.dump_template_hex
                if out_path == "-":
                    sys.stdout.write(blob)
                    if not blob.endswith("\n"):
                        sys.stdout.write("\n")
                else:
                    with open(out_path, "w", encoding="ascii", errors="strict") as f:
                        f.write(blob)
                    print("Wrote blocktemplate_blob hex to", out_path, file=sys.stderr)

        if args.votes_for_hash:
            h = args.votes_for_hash.strip().lower()
            if len(h) != 64:
                print("votes-for-hash must be 64 hex characters", file=sys.stderr)
                return 2
            r = json_rpc(args.url, "get_pulse_arqnet_votes", {"block_hash": h})
            print("get_pulse_arqnet_votes:")
            print(f"  found: {r.get('found')}")
            print(f"  chain_height: {r.get('chain_height')}")
            print(f"  votes: {len(r.get('votes', []))} row(s)")
    except urllib.error.URLError as e:
        print(f"HTTP error: {e}", file=sys.stderr)
        return 1
    except RuntimeError as e:
        print(f"RPC error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
