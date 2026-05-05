#!/usr/bin/env python3
# Copyright (c) 2026, The Arqma Network
#
# HTTP JSON-RPC rehearsal for Pulse tooling: get_info, get_pulse_block_template,
# get_pulse_arqnet_votes, optional template→submit_block (single shot or --watch-submit loop).
# Requires a daemon with PoS telemetry / Pulse RPC enabled (see summary-pos.md).
# Arqnet quorum (ZMQ SN transport) is out of scope here — use SN peers so pulse_vote
# fills the daemon buffer, then --merge-arqnet-votes + --watch-submit for an E2E rehearsal.

import argparse
import sys
import time
from pathlib import Path

# Allow `from pulse_tools_common import …` when run as a script.
sys.path.insert(0, str(Path(__file__).resolve().parent))

import urllib.error

from pulse_tools_common import json_rpc


def call_pulse_template(url, pulse_round, validator_bitset, pulse_random_hex, merge_arqnet):
    params = {
        "pulse_round": int(pulse_round),
        "validator_bitset": int(validator_bitset),
    }
    if pulse_random_hex:
        params["pulse_random_value"] = pulse_random_hex
    if merge_arqnet:
        params["merge_arqnet_votes"] = True
    return json_rpc(url, "get_pulse_block_template", params)


def print_template_subset(tpl_r):
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


def submit_template_blob(url, tpl_r):
    blob = tpl_r.get("blocktemplate_blob", "")
    if not blob:
        raise RuntimeError("no blocktemplate_blob in template response")
    return json_rpc(url, "submit_block", [blob])


def main():
    p = argparse.ArgumentParser(
        description="Pulse HTTP JSON-RPC rehearsal (stdlib only). "
        "Use --watch-submit for E2E: poll template with merged arqnet votes until quorum, then submit_block."
    )
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
        "--submit-if-ready",
        action="store_true",
        help="With --pulse-template: after template, submit_block if merged_pulse_signatures_meet_threshold",
    )
    p.add_argument(
        "--submit-anyway",
        action="store_true",
        help="With --submit-if-ready or --watch-submit: submit even if quorum threshold not met (stagenet/dev only)",
    )
    p.add_argument(
        "--watch-submit",
        action="store_true",
        help="Poll get_pulse_block_template (--merge-arqnet-votes required); submit when ready or with --submit-anyway",
    )
    p.add_argument(
        "--watch-interval",
        type=float,
        default=2.0,
        help="Seconds between template polls (default 2)",
    )
    p.add_argument(
        "--watch-max",
        type=int,
        default=300,
        help="Max poll iterations (default 300)",
    )
    p.add_argument(
        "--votes-for-hash",
        default="",
        metavar="HEX64",
        help="After other steps, call get_pulse_arqnet_votes with this 64-char block hash",
    )
    args = p.parse_args()

    if args.watch_submit and not args.merge_arqnet_votes:
        print("--watch-submit requires --merge-arqnet-votes", file=sys.stderr)
        return 2
    if args.submit_if_ready and not args.pulse_template and not args.watch_submit:
        print("--submit-if-ready requires --pulse-template (or use --watch-submit alone)", file=sys.stderr)
        return 2
    if args.submit_anyway and not (args.submit_if_ready or args.watch_submit):
        print("--submit-anyway requires --submit-if-ready or --watch-submit", file=sys.stderr)
        return 2

    try:
        if args.info:
            r = json_rpc(args.url, "get_info")
            keys = (
                "pos_fork_active",
                "pos_planned_hf_name",
                "pos_planned_fork_height",
                "pos_target_block_time_sec",
                "pos_quorum_validators_min",
                "pos_signature_threshold",
                "pos_expects_sn_storage_server",
                "pos_pulse_blocks_since_fork",
                "pos_pulse_next_round_wire_hint",
                "pos_pulse_cum_diff_uses_60s_lwma",
                "pos_pulse_arqnet_vote_buffer_blocks",
            )
            print("get_info (subset):")
            for k in keys:
                if k in r:
                    print(f"  {k}: {r[k]}")

        tpl_r = None

        if args.watch_submit:
            for i in range(args.watch_max):
                tpl_r = call_pulse_template(
                    args.url,
                    args.pulse_round,
                    args.validator_bitset,
                    args.pulse_random_hex,
                    True,
                )
                if i == 0 or (i + 1) % 30 == 0:
                    print_template_subset(tpl_r)
                ready = bool(tpl_r.get("merged_pulse_signatures_meet_threshold"))
                if ready or args.submit_anyway:
                    if not ready:
                        print(
                            "WARNING: submitting without merged_pulse_signatures_meet_threshold (--submit-anyway)",
                            file=sys.stderr,
                        )
                    sub = submit_template_blob(args.url, tpl_r)
                    print("submit_block:", sub.get("status", sub))
                    return 0
                time.sleep(max(0.1, float(args.watch_interval)))
            print("watch-submit: max iterations reached without submitting", file=sys.stderr)
            return 3

        if args.pulse_template:
            tpl_r = call_pulse_template(
                args.url,
                args.pulse_round,
                args.validator_bitset,
                args.pulse_random_hex,
                args.merge_arqnet_votes,
            )
            print_template_subset(tpl_r)

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

            if args.submit_if_ready:
                ready = bool(tpl_r.get("merged_pulse_signatures_meet_threshold"))
                if not ready and not args.submit_anyway:
                    print(
                        "Not submitting: merged_pulse_signatures_meet_threshold is false "
                        "(use --submit-anyway for forced stagenet submit)",
                        file=sys.stderr,
                    )
                    return 3
                if not ready:
                    print(
                        "WARNING: --submit-anyway: submitting without quorum threshold",
                        file=sys.stderr,
                    )
                sub = submit_template_blob(args.url, tpl_r)
                print("submit_block:", sub.get("status", sub))

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
