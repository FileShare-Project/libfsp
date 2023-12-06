##
## Project LibFileShareProtocol, 2022
##
## Author Francois Michaut
##
## Started on  Thu May 26 23:23:59 2022 Francois Michaut
## Last update Thu Dec  7 10:39:45 2023 Francois Michaut
##
## CMakeLists.txt : CMake to build the FileShareProtocol library
##

cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-O3 -DDEBUG -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O3")
add_compile_definitions(_GNU_SOURCE)

project(LibFileShareProtocol VERSION 0.1.0 LANGUAGES C CXX)
configure_file(include/FileShare/Version.hpp.in FileShare/Version.hpp)

FetchContent_Declare(
  cppsockets
  GIT_REPOSITORY  https://github.com/FileShare-Project/libcppsockets.git
  GIT_TAG         94ac5087f8eca9dd25a1c87e347e16bb9a2168d2
)
FetchContent_MakeAvailable(cppsockets)

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

target_include_directories(fsp PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)
target_compile_definitions(fsp PRIVATE _LARGEFILE_SOURCE _FILE_OFFSET_BITS=64)

target_link_libraries(fsp cppsockets)

option(LIBFSP_BUILD_TESTS "TRUE to build the libfsp tests" FALSE)
if(LIBFSP_BUILD_TESTS)
  add_subdirectory(tests)
endif()
