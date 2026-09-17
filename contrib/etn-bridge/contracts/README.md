# Solidity stubs (NOT audited)

Deploy order for local anvil:

1. Deploy `WARQ` with `bridge = address(0)` then call a future `setBridge`, **or**
   use a create2/salt workflow so `EtNBridge` address is known first.
2. Deploy `EtNBridge(warq, operator)`.
3. If needed, transfer bridge role on `WARQ` to `EtNBridge`.

This scaffold keeps `WARQ` mint/burn gated by `bridge` for clarity. Production
must use audited OpenZeppelin ERC-20 + multisig/timelock operator.
