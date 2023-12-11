##
## Project LibFileShareProtocol, 2023
##
## Author Francois Michaut
##
## Started on  Sun Dec 10 10:29:19 2023 Francois Michaut
## Last update Sun Dec 10 10:29:24 2023 Francois Michaut
##
## CMakeLists.txt : CMake to set build options for cereal dependency
##

set(JUST_INSTALL_CEREAL ON) # We don't want to build tests or benchmarks

FetchContent_MakeAvailable(cereal)
