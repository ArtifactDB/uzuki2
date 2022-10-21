cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  byteme
  GIT_REPOSITORY https://github.com/LTLA/byteme
  GIT_TAG 76521683c544ae61e502b07fc9f124e11c48681e
)

FetchContent_MakeAvailable(byteme)

FetchContent_Declare(
  millijson 
  GIT_REPOSITORY https://github.com/LTLA/millijson
  GIT_TAG ac5698b30f87f61902becb4acd5ba4b771ba511b
)

FetchContent_MakeAvailable(millijson)
