cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(
  byteme
  GIT_REPOSITORY https://github.com/LTLA/byteme
  GIT_TAG master #^2.0.0
)

FetchContent_Declare(
  millijson 
  GIT_REPOSITORY https://github.com/ArtifactDB/millijson
  GIT_TAG master #^2.0.0
)

FetchContent_Declare(
  ritsuko 
  GIT_REPOSITORY https://github.com/ArtifactDB/ritsuko
  GIT_TAG master #^0.6.0
)

FetchContent_MakeAvailable(byteme)
FetchContent_MakeAvailable(millijson)
FetchContent_MakeAvailable(ritsuko)
