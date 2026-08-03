set(arq_messaging_sources
  identity.cpp)

set(arq_messaging_headers
  identity.hpp
  message_envelope.hpp
  onion_request.hpp
  sealed_sender.hpp
  swarm_map.hpp)

arqma_find_all_headers(arq_messaging_private_headers "${CMAKE_CURRENT_SOURCE_DIR}")

arqma_private_headers(arq_messaging
  ${arq_messaging_private_headers})
arqma_add_library(arq_messaging
  ${arq_messaging_sources}
  ${arq_messaging_headers}
  ${arq_messaging_private_headers})

target_include_directories(arq_messaging
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(arq_messaging
  PUBLIC
    sodium
  PRIVATE
    ${EXTRA_LIBRARIES})
