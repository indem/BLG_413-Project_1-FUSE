cmake_minimum_required (VERSION 2.6)
set (CMAKE_C_STANDARD 99)
project (filesystem C)

# Ansilove library
find_path(ANSILOVE_INCLUDE_DIRS ansilove.h)
find_library(ANSILOVE_LIBRARIES NAMES ansilove REQUIRED)
include_directories(${ANSILOVE_INCLUDE_DIRS})

find_path(MAGIC_INCLUDE_DIRS magic.h)
find_library(MAGIC_LIBRARIES NAMES magic REQUIRED)
include_directories(${MAGIC_INCLUDE_DIRS})

find_path(FUSE_INCLUDE_DIRS fuse.h)
find_library(FUSE_LIBRARIES NAMES fuse REQUIRED)
include_directories(${FUSE_INCLUDE_DIRS})

set(SRC filesystem.c)

add_definitions(-Wall -ansi -std=c99 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -ansi -g -ggdb)

add_executable(filesystem ${SRC})
target_link_libraries(filesystem ${ANSILOVE_LIBRARIES})
target_link_libraries(filesystem ${MAGIC_LIBRARIES})
target_link_libraries(filesystem ${FUSE_LIBRARIES})