// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

interface IWARQ {
    function mint(address to, uint256 amount) external;
    function burnFrom(address from, uint256 amount) external;
}

/// @title EtNBridge — stub controller linking ARQ burn attestations to wARQ mint/redeem
/// @dev Issuer/multisig operated. NOT audited.
contract EtNBridge {
    IWARQ public immutable warq;
    address public operator;
    mapping(bytes32 => bool) public usedArqBurn;
    mapping(bytes32 => bool) public usedEthRedeem;

    event Minted(bytes32 indexed arqBurnTx, address indexed to, uint256 amount);
    event Redeemed(bytes32 indexed redeemId, address indexed from, uint256 amount, string arqDest);

    modifier onlyOperator() {
        require(msg.sender == operator, "not operator");
        _;
    }

    constructor(address warq_, address operator_) {
        warq = IWARQ(warq_);
        operator = operator_;
    }

    function setOperator(address next) external onlyOperator {
        operator = next;
    }

    /// Mint after off-chain verification of an Arqma HF19 burn (+ attestation).
    function mintWithBurnAttestation(bytes32 arqBurnTx, address to, uint256 amount) external onlyOperator {
        require(!usedArqBurn[arqBurnTx], "used");
        usedArqBurn[arqBurnTx] = true;
        warq.mint(to, amount);
        emit Minted(arqBurnTx, to, amount);
    }

    /// User burns wARQ; operator later pays ARQ on L1 and records payout attestation.
    function requestRedeem(uint256 amount, string calldata arqDest) external {
        bytes32 redeemId = keccak256(abi.encode(msg.sender, amount, arqDest, block.number));
        require(!usedEthRedeem[redeemId], "used");
        usedEthRedeem[redeemId] = true;
        warq.burnFrom(msg.sender, amount);
        emit Redeemed(redeemId, msg.sender, amount, arqDest);
    }
}
