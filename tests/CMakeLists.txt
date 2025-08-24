##
## Project LibFileShareProtocol-Tests, 2022
##
## Author Francois Michaut
##
## Started on  Mon Feb 14 19:35:41 2022 Francois Michaut
## Last update Sun Aug 24 01:20:19 2025 Francois Michaut
##
## CMakeLists.txt : CMake building and running tests for FileShare
##

include(CTest)

create_test_sourcelist(TestFiles test_driver.cpp
  Config/TestFileMapping.cpp

  Protocol/TestVersion.cpp

  Utils/TestFileHash.cpp
  Utils/TestSerialize.cpp
  Utils/TestVarInt.cpp
)

add_executable(unit_tests
  ${TestFiles}
)

target_compile_definitions(unit_tests PRIVATE DEBUG)
target_link_libraries(unit_tests fsp)

target_compile_options(unit_tests PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:-UNDEBUG>
  $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>
)

foreach (test ${TestFiles})
  if (NOT ${test} MATCHES "test_driver.cpp$")
    get_filename_component (DName ${test} DIRECTORY)
    get_filename_component (TName ${test} NAME_WE)
    if (DName STREQUAL "")
      add_test (NAME ${TName} COMMAND unit_tests ${TName})
    else()
      add_test (NAME ${DName}/${TName} COMMAND unit_tests ${DName}/${TName})
    endif()
  endif()
endforeach ()
