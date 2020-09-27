cmake_minimum_required(VERSION 3.10)
project(DwarfIdeaBuilder)

# Third-party dependencies.

include(FetchContent)

find_package(OpenMP REQUIRED)

FetchContent_Declare(
    libarchive
    URL "https://www.libarchive.org/downloads/libarchive-3.4.3.tar.gz"
    URL_HASH "SHA256=ee1e749213c108cb60d53147f18c31a73d6717d7e3d2481c157e1b34c881ea39"
)
FetchContent_MakeAvailable(libarchive)

FetchContent_Declare(
    gflags
    URL "https://github.com/gflags/gflags/archive/v2.2.2.zip"
    URL_HASH "SHA256=19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5"
)
FetchContent_MakeAvailable(gflags)

FetchContent_Declare(
    glog
    URL "https://github.com/google/glog/archive/v0.4.0.zip"
    URL_HASH "SHA256=9e1b54eb2782f53cd8af107ecf08d2ab64b8d0dc2b7f5594472f3bd63ca85cdc"
)
# `glogs` doesn't see our `gflags` dependency declared above, so disable it to avoid warning messages.
set(WITH_GFLAGS OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(glog)

FetchContent_Declare(
    sqlite3
    URL "https://www.sqlite.org/2020/sqlite-amalgamation-3330000.zip"
    URL_HASH "SHA256=b34f4c0c0eefad9a7e515c030c18702e477f4ef7d8ade6142bdab8011b487ac6"
)
FetchContent_GetProperties(sqlite3)
if(NOT sqlite3_POPULATED)
  FetchContent_Populate(sqlite3)
  add_library(sqlite3 ${sqlite3_SOURCE_DIR}/sqlite3.c)
  target_include_directories(sqlite3 INTERFACE $<BUILD_INTERFACE:${sqlite3_SOURCE_DIR}>)
  target_link_libraries(sqlite3 PRIVATE ${CMAKE_DL_LIBS})
endif()

FetchContent_Declare(
    fse
    GIT_REPOSITORY "https://github.com/Cyan4973/FiniteStateEntropy.git"
    GIT_TAG "12a533a9bf4d7bdcc507bf9d11302a7a1be454f5"
)
FetchContent_GetProperties(fse)
if(NOT fse_POPULATED)
  FetchContent_Populate(fse)
  file(GLOB_RECURSE fse_SOURCES ${fse_SOURCE_DIR}/lib/*.c)
  # This file doesn't compile. Given we're not using any functionality provided
  # there - just exclude it from the compilation.
  list (REMOVE_ITEM fse_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/_deps/fse-src/lib/Archives/hufx6_decompress.c")
  add_library(fse ${fse_SOURCES})
  target_include_directories(fse INTERFACE $<BUILD_INTERFACE:${fse_SOURCE_DIR}/lib>)
endif()

FetchContent_Declare(
    kanzi
    GIT_REPOSITORY "https://github.com/flanglet/kanzi-cpp.git"
    GIT_TAG "913b580c7945f3be26cd6cdf055df371a1717fe8"
)
FetchContent_GetProperties(kanzi)
if(NOT kanzi_POPULATED)
  FetchContent_Populate(kanzi)
  file(GLOB_RECURSE kanzi_SOURCES ${kanzi_SOURCE_DIR}/src/*.cpp)
  add_library(kanzi ${kanzi_SOURCES})
  target_include_directories(kanzi INTERFACE $<BUILD_INTERFACE:${kanzi_SOURCE_DIR}/src>)
endif()

# Main target.

file(GLOB dwarf-idea-builder_SOURCES src/*.cpp)
add_executable(dwarf-idea-builder ${dwarf-idea-builder_SOURCES})

target_include_directories(
    dwarf-idea-builder
    PRIVATE
    src
)

target_link_libraries(
    dwarf-idea-builder
    archive
    fse
    kanzi
    sqlite3
    gflags
    glog
    OpenMP::OpenMP_CXX
)
