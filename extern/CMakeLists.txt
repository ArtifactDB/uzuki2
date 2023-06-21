cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  byteme
  GIT_REPOSITORY https://github.com/LTLA/byteme
  GIT_TAG a270a9cc0431d3f22e378becdd0ed1a32b7a4812
)

FetchContent_MakeAvailable(byteme)

FetchContent_Declare(
  millijson 
  GIT_REPOSITORY https://github.com/LTLA/millijson
  GIT_TAG dbd19272ff84d9b7f65d84472870f27a5b426a2b
)

FetchContent_MakeAvailable(millijson)
