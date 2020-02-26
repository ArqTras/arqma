#ifndef BLOCKCHAIN_OBJECTS_H
#define BLOCKCHAIN_OBJECTS_H

#include "cryptonote_core/blockchain.h"
#include "cryptonote_core/tx_pool.h"
#include "cryptonote_core/service_node_list.h"
#include "cryptonote_core/service_node_deregister.h"

struct blockchain_objects_t
{
  cryptonote::Blockchain m_blockchain;
  cryptonote::tx_memory_pool m_mempool;
  service_nodes::service_node_list m_service_node_list;
  arqma_sn::deregister_vote_pool m_deregister_vote_pool;
  blockchain_objects_t() :
    m_blockchain(m_mempool, m_service_node_list, m_deregister_vote_pool),
    m_service_node_list(m_blockchain),
    m_mempool(m_blockchain) { }
};

#endif // BLOCKCHAIN_OBJECTS_H
