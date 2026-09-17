let currentSwapId = null;

async function refreshStatus() {
  const r = await fetch("/v1/status");
  document.getElementById("status-json").textContent = JSON.stringify(await r.json(), null, 2);
}

document.getElementById("direction").addEventListener("change", (e) => {
  const mint = e.target.value === "mint";
  document.getElementById("dest-label").firstChild.textContent = mint
    ? "Destination ETH address"
    : "Destination ARQ address";
  document.getElementById("dest").placeholder = mint ? "0x…" : "ar…";
});

document.getElementById("swap-form").addEventListener("submit", async (ev) => {
  ev.preventDefault();
  const direction = document.getElementById("direction").value;
  const amount_atomic = document.getElementById("amount").value.trim();
  const dest = document.getElementById("dest").value.trim();
  const body = { direction, amount_atomic };
  if (direction === "mint") body.dest_eth_address = dest;
  else body.dest_arq_address = dest;
  const r = await fetch("/v1/swap", {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(body),
  });
  const data = await r.json();
  if (!r.ok) {
    alert(data.detail || JSON.stringify(data));
    return;
  }
  currentSwapId = data.id;
  document.getElementById("result").hidden = false;
  document.getElementById("swap-json").textContent = JSON.stringify(data, null, 2);
});

document.getElementById("finalize").addEventListener("click", async () => {
  if (!currentSwapId) return;
  const txid = document.getElementById("txid").value.trim();
  const direction = document.getElementById("direction").value;
  const body = direction === "mint" ? { arq_txid: txid || undefined } : { eth_txid: txid || undefined };
  const r = await fetch(`/v1/swap/${currentSwapId}/finalize`, {
    method: "POST",
    headers: { "content-type": "application/json" },
    body: JSON.stringify(body),
  });
  const data = await r.json();
  document.getElementById("final-json").textContent = JSON.stringify(data, null, 2);
});

refreshStatus();
