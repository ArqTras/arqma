set(arq_storage_sources
  storage_client.cpp
  storage_endpoint.cpp)

set(arq_storage_headers
  storage_client.h
  storage_endpoint.h)

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

target_link_libraries(arq_storage
  PUBLIC
    ${Boost_SYSTEM_LIBRARY}
  PRIVATE
    ${EXTRA_LIBRARIES})
