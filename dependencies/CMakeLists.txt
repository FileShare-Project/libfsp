##
## Project LibFileShareProtocol, 2023
##
## Author Francois Michaut
##
## Started on  Sun Dec 10 10:27:39 2023 Francois Michaut
## Last update Sat Jun 20 10:47:41 2026 Francois Michaut
##
## CMakeLists.txt : CMake to fetch and build the dependecies of the FileShareProtocol library
##

include(FetchContent)

FetchContent_Declare(
  cppsockets
  GIT_REPOSITORY  https://github.com/FileShare-Project/libcppsockets.git
  GIT_TAG         99cb1d1ff00345ed040b11ef645a3977a30512db
)
FetchContent_MakeAvailable(cppsockets)

FetchContent_Declare(
  frozen
  GIT_REPOSITORY  https://github.com/serge-sans-paille/frozen.git
  GIT_TAG         61dce5ae18ca59931e27675c468e64118aba8744
)
FetchContent_MakeAvailable(frozen)

FetchContent_Declare(
  cereal
  GIT_REPOSITORY  https://github.com/f-michaut/cereal.git
  GIT_TAG         83cb386d0146bd5fe77f9bc5974d85b4da3cda90
)
add_subdirectory(cereal)
