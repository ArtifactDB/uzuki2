cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  byteme
  GIT_REPOSITORY https://github.com/LTLA/byteme
  GIT_TAG master
)

FetchContent_Declare(
  millijson 
  GIT_REPOSITORY https://github.com/ArtifactDB/millijson
  GIT_TAG master
)

FetchContent_Declare(
  ritsuko 
  GIT_REPOSITORY https://github.com/ArtifactDB/ritsuko
  GIT_TAG master
)

FetchContent_MakeAvailable(byteme)
FetchContent_MakeAvailable(millijson)
FetchContent_MakeAvailable(ritsuko)
