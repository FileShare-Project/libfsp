##
## Project LibFileShareProtocol-Tests, 2022
##
## Author Francois Michaut
##
## Started on  Mon Feb 14 19:35:41 2022 Francois Michaut
## Last update Mon Aug 29 20:23:03 2022 Francois Michaut
##
## CMakeLists.txt : CMake building and running tests for FileShare
##

cmake_minimum_required (VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(LibFileShareProtocol-Tests VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

# create_test_sourcelist(TestFiles test_driver.cpp
#   )

# add_executable(unit_tests
#   ${TestFiles}
#   )

foreach (test ${TestFiles})
  if (NOT ${test} STREQUAL test_driver.cpp)
    get_filename_component (DName ${test} DIRECTORY)
    get_filename_component (TName ${test} NAME_WE)
    if (DName STREQUAL "")
      add_test (NAME ${TName} COMMAND unit_tests ${TName})
    else()
      add_test (NAME ${DName}/${TName} COMMAND unit_tests ${DName}/${TName})
    endif()
  endif()
endforeach ()

file(GLOB LIB_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/lib/*")
foreach (LIB_DIRECTORY IN LISTS LIB_DIRECTORIES)
  cmake_path(GET LIB_DIRECTORY FILENAME LIB_NAME)
  add_test (NAME ${LIB_NAME}_tests COMMAND ${CMAKE_MAKE_PROGRAM} -C "${LIB_DIRECTORY}" test)
endforeach()

include_directories(
  "${CMAKE_CURRENT_BINARY_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/../include"
)
