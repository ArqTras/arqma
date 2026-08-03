set(arq_storage_sources
  storage_client.cpp)

set(arq_storage_headers
  storage_client.h)

arqma_find_all_headers(arq_storage_private_headers "${CMAKE_CURRENT_SOURCE_DIR}")

arqma_private_headers(arq_storage
  ${arq_storage_private_headers})
arqma_add_library(arq_storage
  ${arq_storage_sources}
  ${arq_storage_headers}
  ${arq_storage_private_headers})

target_include_directories(arq_storage
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
