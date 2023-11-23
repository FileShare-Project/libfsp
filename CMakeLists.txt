##
## Project LibFileShareProtocol, 2022
##
## Author Francois Michaut
##
## Started on  Thu May 26 23:23:59 2022 Francois Michaut
## Last update Sun Nov 19 13:11:34 2023 Francois Michaut
##
## CMakeLists.txt : CMake to build the FileShareProtocol library
##

cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O3 -DDEBUG -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")
add_compile_definitions(_GNU_SOURCE)

# TODO: This currently breaks Apple/Windows builds: find another solution
set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

project(LibFileShareProtocol VERSION 0.1.0 LANGUAGES C CXX)
configure_file(include/FileShare/Version.hpp.in FileShare/Version.hpp)

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
  source/Errors/TransferErrors.cpp

  source/Protocol/Handler/v0.0.0/ProtocolHandler.cpp

  source/Protocol/Protocol.cpp
  source/Protocol/RequestData.cpp
  source/Protocol/Version.cpp

  source/Utils/DebugPerf.cpp
  source/Utils/FileDescriptor.cpp
  source/Utils/FileHash.cpp
  source/Utils/Path.cpp
  source/Utils/Serialize.cpp
  source/Utils/VarInt.cpp

  source/Config/FileMapping.cpp

  source/Client.cpp
  source/Client_private.cpp
  source/Config.cpp
  source/MessageQueue.cpp
  source/Server.cpp
  source/TransferHandler.cpp
)

target_compile_definitions(fsp PRIVATE _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64)

target_link_libraries(fsp cppsockets ssl)

add_custom_target(test
  COMMAND ${CMAKE_COMMAND} --log-level=WARNING
  -B "${CMAKE_CURRENT_BINARY_DIR}/tests"
  -S "${CMAKE_CURRENT_SOURCE_DIR}/tests"
  -G ${CMAKE_GENERATOR}
  COMMAND ${CMAKE_COMMAND} -E cmake_echo_color
  --switch=$(COLOR) --cyan "Building tests..."
  COMMAND ${CMAKE_COMMAND}
  --build "${CMAKE_CURRENT_BINARY_DIR}/tests" -- --quiet
  COMMAND ${CMAKE_MAKE_PROGRAM}
  -C "${CMAKE_CURRENT_BINARY_DIR}/tests" test ARGS=--output-on-failure
)
