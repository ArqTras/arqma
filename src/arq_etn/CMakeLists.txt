# Copyright (c) 2018 - 2026, The Arqma Network
#
# ETN custody / wrap companion (models A+C). Not consensus.

set(arq_etn_sources
  etn_types.cpp
  etn_store.cpp
  etn_server.cpp)

set(arq_etn_headers
  etn_types.h
  etn_store.h
  etn_server.h)

arqma_add_library(arq_etn
  ${arq_etn_sources}
  ${arq_etn_headers})

target_include_directories(arq_etn
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_link_libraries(arq_etn
  PUBLIC
    ${Boost_SYSTEM_LIBRARY}
  PRIVATE
    ${CMAKE_THREAD_LIBS_INIT}
    ${EXTRA_LIBRARIES})

if(NOT IOS)
  arqma_add_executable(arqma_etn_audit main.cpp)
  target_link_libraries(arqma_etn_audit
    PRIVATE
      arq_etn
      epee
      ${Boost_PROGRAM_OPTIONS_LIBRARY}
      ${Boost_SYSTEM_LIBRARY}
      ${CMAKE_THREAD_LIBS_INIT}
      ${EXTRA_LIBRARIES})
endif()
