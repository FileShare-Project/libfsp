##
## Project LibFileShareProtocol, 2022
##
## Author Francois Michaut
##
## Started on  Thu May 26 23:23:59 2022 Francois Michaut
## Last update Mon Aug 18 17:26:23 2025 Francois Michaut
##
## CMakeLists.txt : CMake to build the FileShareProtocol library
##

cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

if(!WIN32)
  set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O3 -DDEBUG -g3")
  set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")
  add_compile_definitions(_GNU_SOURCE _FILE_OFFSET_BITS=64 _TIME_BITS=64)
endif()

project(LibFileShareProtocol VERSION 0.1.0 LANGUAGES C CXX)
configure_file(include/FileShare/Version.hpp.in FileShare/Version.hpp)

add_subdirectory(dependencies)

add_library(fsp
  source/Config/Config.cpp
  source/Config/FileMapping.cpp
  source/Config/KnownPeerStore.cpp
  source/Config/ServerConfig.cpp

  source/Errors/TransferErrors.cpp

  source/Peer/Peer.cpp
  source/Peer/PeerBase.cpp
  source/Peer/PreAuthPeer.cpp
  source/Peer/Peer_private.cpp

  source/MessageQueue.cpp

  source/Protocol/Handler/v0.0.0/ProtocolHandler.cpp
  source/Protocol/Handler/IProtocolHandler.cpp

  source/Protocol/Protocol.cpp
  source/Protocol/RequestData.cpp
  source/Protocol/Version.cpp

  source/Server.cpp
  source/TransferHandler.cpp

  source/Utils/DebugPerf.cpp
  source/Utils/FileDescriptor.cpp
  source/Utils/FileHash.cpp
  source/Utils/Path.cpp
  source/Utils/Poll.cpp
  source/Utils/Serialize.cpp
  source/Utils/VarInt.cpp
)

target_include_directories(fsp PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_compile_definitions(fsp PRIVATE _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64)
target_compile_options(fsp PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-Wall -Wextra>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(fsp cppsockets cereal frozen)
if(WIN32)
  target_link_libraries(fsp userenv)
endif()

option(LIBFSP_BUILD_TESTS "TRUE to build the libfsp tests" FALSE)
if(LIBFSP_BUILD_TESTS)
  add_subdirectory(tests)
endif()

