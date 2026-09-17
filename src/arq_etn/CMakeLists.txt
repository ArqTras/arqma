# Copyright (c) 2018 - 2026, The Arqma Network
#
# Optional ETN custody / wrap sketch (models A+C). Off by default.

set(arq_etn_sources etn_types.cpp)
set(arq_etn_headers etn_types.h)

arqma_add_library(arq_etn
  ${arq_etn_sources}
  ${arq_etn_headers})

target_include_directories(arq_etn
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..)
