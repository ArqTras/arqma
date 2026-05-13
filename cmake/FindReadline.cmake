# - Try to find readline include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(Readline)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  Readline_ROOT_DIR         Set this variable to the root installation of
#                            readline if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  READLINE_FOUND            System has readline, include and lib dirs found
#  GNU_READLINE_FOUND        Version of readline found is GNU readline, not libedit!
#  LIBEDIT_FOUND             Version of readline found is libedit, not GNU readline!
#  Readline_INCLUDE_DIR      The readline include directories.
#  Readline_LIBRARY          The readline library.
#  GNU_READLINE_LIBRARY      The GNU readline library or empty string.
#  LIBEDIT_LIBRARY           The libedit library or empty string.

find_path(Readline_ROOT_DIR
    NAMES include/readline/readline.h
    PATHS /usr/local/opt/readline/ /opt/homebrew/opt/readline/ /opt/local/ /usr/local/ /usr/
    NO_DEFAULT_PATH
)

# Do not `set(... "")` here: CMake treats "" as an already-set value and
# find_path/find_library will skip searching (see CMake docs: only NOTFOUND is overwritten).

if(Readline_ROOT_DIR)
  find_path(Readline_INCLUDE_DIR
      NAMES readline/readline.h
      PATHS "${Readline_ROOT_DIR}/include"
      NO_DEFAULT_PATH
  )
  find_library(Readline_LIBRARY
      NAMES readline
      PATHS "${Readline_ROOT_DIR}/lib"
      NO_DEFAULT_PATH
  )
endif()

# MSYS2 / MinGW and other layouts: Homebrew-style root is empty, so search defaults
# (and optional MINGW_PREFIX from the MINGW64 login environment).
if(NOT Readline_INCLUDE_DIR OR NOT Readline_LIBRARY)
  # HINTS must be the parent of readline/ and lib/, not the toolchain prefix alone:
  # NAMES readline/readline.h is resolved as <hint>/readline/readline.h
  set(_readline_include_hints "")
  set(_readline_lib_hints "")
  if(DEFINED ENV{MINGW_PREFIX})
    list(APPEND _readline_include_hints "$ENV{MINGW_PREFIX}/include")
    list(APPEND _readline_lib_hints "$ENV{MINGW_PREFIX}/lib")
  endif()
  find_path(Readline_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS ${_readline_include_hints}
  )
  find_library(Readline_LIBRARY
    NAMES readline
    HINTS ${_readline_lib_hints}
  )
endif()

find_library(Termcap_LIBRARY
  NAMES ncurses tinfo termcap ncursesw cursesw curses
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Readline DEFAULT_MSG Readline_INCLUDE_DIR Readline_LIBRARY)
mark_as_advanced(
    Readline_ROOT_DIR
    Readline_INCLUDE_DIR
    Readline_LIBRARY
)

if(Readline_FOUND)
  set(READLINE_FOUND TRUE)
else()
  set(READLINE_FOUND FALSE)
endif()

set(LIBEDIT_LIBRARY "")
set(GNU_READLINE_LIBRARY "")
set(GNU_READLINE_FOUND FALSE)
set(LIBEDIT_FOUND FALSE)

if(Readline_INCLUDE_DIR AND Readline_LIBRARY)
  set(CMAKE_REQUIRED_INCLUDES ${Readline_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY})

  include(CheckFunctionExists)
  check_function_exists(rl_copy_text HAVE_COPY_TEXT)
  check_function_exists(rl_filename_completion_function HAVE_COMPLETION_FUNCTION)

  if(NOT HAVE_COMPLETION_FUNCTION)
    set(CMAKE_REQUIRED_LIBRARIES ${Readline_LIBRARY} ${Termcap_LIBRARY})
    check_function_exists(rl_copy_text HAVE_COPY_TEXT_TC)
    check_function_exists(rl_filename_completion_function HAVE_COMPLETION_FUNCTION_TC)
    set(HAVE_COMPLETION_FUNCTION ${HAVE_COMPLETION_FUNCTION_TC})
    set(HAVE_COPY_TEXT ${HAVE_COPY_TEXT_TC})
    if(HAVE_COMPLETION_FUNCTION)
      set(Readline_LIBRARY ${Readline_LIBRARY} ${Termcap_LIBRARY})
    endif()
  endif()

  if(HAVE_COMPLETION_FUNCTION AND HAVE_COPY_TEXT)
    set(GNU_READLINE_FOUND TRUE)
    set(GNU_READLINE_LIBRARY ${Readline_LIBRARY})
  elseif(READLINE_FOUND AND NOT HAVE_COPY_TEXT)
    set(LIBEDIT_FOUND TRUE)
    set(LIBEDIT_LIBRARY ${Readline_LIBRARY})
  endif()
endif()
