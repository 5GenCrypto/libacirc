cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(acirc VERSION 1.0.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(GNUInstallDirs)

option(HAVE_GMP "Define whether gmp is enabled" ON)
message(STATUS "GMP: ${HAVE_GMP}")

set(acirc_SOURCES
  src/acirc.c
  src/build.c
  src/parse.c
  src/scan.c
  src/topo.c
  src/utils.c
  src/commands/outputs.c
  src/commands/secrets.c
  src/commands/test.c
  )
set(acirc_HEADERS
  src/acirc.h
  )
if(HAVE_GMP)
  list(APPEND acirc_SOURCES src/gmp.c)
endif(HAVE_GMP)

add_library(acirc SHARED ${acirc_SOURCES})
target_include_directories(acirc INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)
if(HAVE_GMP)
  target_link_libraries(acirc PUBLIC gmp)
endif(HAVE_GMP)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -std=gnu11 -march=native")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg -ggdb -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")

install(TARGETS acirc LIBRARY DESTINATION lib)
install(FILES ${acirc_HEADERS} DESTINATION include)

# Test files

enable_testing()
macro(add_test_ _name)
  add_executable("${_name}" "test/${_name}.c")
  target_include_directories("${_name}" PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries("${_name}" PRIVATE acirc)
  if(HAVE_GMP)
    target_link_libraries("${_name}" PRIVATE gmp)
  endif(HAVE_GMP)
  add_test(NAME "${_name}" COMMAND "${_name}")
endmacro()

add_test_(test_builder)
add_test_(test_circ)
if(HAVE_GMP)
  add_test_(test_mpz)
endif(HAVE_GMP)
add_test_(test_total_degree)
