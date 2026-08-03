set(arqmq_sources
  arqmq.cpp)

set(arqmq_headers
  arqmq.h)

arqma_find_all_headers(arqmq_private_headers "${CMAKE_CURRENT_SOURCE_DIR}")

arqma_private_headers(arqmq
  ${arqmq_private_headers})
arqma_add_library(arqmq
  ${arqmq_sources}
  ${arqmq_headers}
  ${arqmq_private_headers})

target_include_directories(arqmq
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
