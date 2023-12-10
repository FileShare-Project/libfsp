##
## Project LibFileShareProtocol, 2023
##
## Author Francois Michaut
##
## Started on  Sun Dec 10 10:27:39 2023 Francois Michaut
## Last update Sun Dec 10 18:56:37 2023 Francois Michaut
##
## CMakeLists.txt : CMake to fetch and build the dependecies of the FileShareProtocol library
##

include(FetchContent)

FetchContent_Declare(
  cppsockets
  GIT_REPOSITORY  https://github.com/FileShare-Project/libcppsockets.git
  GIT_TAG         7a0bd91b16f6c3aa808e26684916e0dcc935717a
)
FetchContent_MakeAvailable(cppsockets)

FetchContent_Declare(
  cereal
  GIT_REPOSITORY  https://github.com/f-michaut/cereal.git
  GIT_TAG         203ba6f75047dd90e961596d6ad742b80d942014
)
add_subdirectory(cereal)
