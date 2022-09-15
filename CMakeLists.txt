##
## Project LibFileShareProtocol, 2022
##
## Author Francois Michaut
##
## Started on  Thu May 26 23:23:59 2022 Francois Michaut
## Last update Thu Sep 15 14:10:59 2022 Francois Michaut
##
## CMakeLists.txt : CMake to build the FileShareProtocol library
##

cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# TODO: This currently breaks Apple/Windows builds: find another solution
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

project(LibFileShareProtocol VERSION 0.1.0 LANGUAGES C CXX)
configure_file(include/FileShareProtocol/Version.hpp.in FileShareProtocol/Version.hpp)

add_subdirectory(lib/libcppsockets) # Lib C++ Sockets

file(GLOB LIB_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/lib/*/include")
file(GLOB LIB_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/lib/*")

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${LIB_INCLUDES}"
  "${LIB_OUTPUTS}"
  )

add_library(fsp
  source/Protocol.cpp
  source/Client.cpp
  source/ClientConfig.cpp
  )

target_link_libraries(fsp cppsockets)

add_custom_target(test
  COMMAND ${CMAKE_COMMAND}
  -B "${CMAKE_CURRENT_BINARY_DIR}/tests"
  -S "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  -G ${CMAKE_GENERATOR}
  COMMAND ${CMAKE_COMMAND} -E cmake_echo_color
  --switch=$(COLOR) --cyan "Building tests..."
  COMMAND ${CMAKE_COMMAND}
  --build "${CMAKE_CURRENT_BINARY_DIR}/tests"
  COMMAND ${CMAKE_MAKE_PROGRAM}
  -C "${CMAKE_CURRENT_BINARY_DIR}/tests" test
  )
