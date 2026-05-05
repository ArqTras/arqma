# Copyright (c) 2026, The Arqma Network
# Shared JSON-RPC helper for pulse-rehearsal scripts (stdlib only).

import json
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
