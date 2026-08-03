set(arq_router_sources
  router_service.cpp)

set(arq_router_headers
  router_config.h
  router_service.h)

arqma_find_all_headers(arq_router_private_headers "${CMAKE_CURRENT_SOURCE_DIR}")

arqma_private_headers(arq_router
  ${arq_router_private_headers})
arqma_add_library(arq_router
  ${arq_router_sources}
  ${arq_router_headers}
  ${arq_router_private_headers})

target_include_directories(arq_router
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
