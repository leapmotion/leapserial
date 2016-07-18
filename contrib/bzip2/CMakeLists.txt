project(bzip2 C)

set(VERSION "1.0.6")

include(CheckTypeSize)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckCSourceCompiles)

#
# Check to see if we have large file support
#
set(CMAKE_REQUIRED_DEFINITIONS -D_LARGEFILE64_SOURCE=1)
# We add these other definitions here because CheckTypeSize.cmake
# in CMake 2.4.x does not automatically do so and we want
# compatibility with CMake 2.4.x.
check_type_size(off64_t OFF64_T)
if(HAVE_OFF64_T)
   add_definitions(-D_LARGEFILE64_SOURCE=1)
endif()
set(CMAKE_REQUIRED_DEFINITIONS) # clear variable
add_definitions(-D_FILE_OFFSET_BITS=64)

if(MSVC)
    set(CMAKE_DEBUG_POSTFIX "d")
    add_definitions(-DWIN32)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

#============================================================================
# bzip2
#============================================================================

set(BZIP2_PUBLIC_HDRS
    bzlib.h
)
set(BZIP2_PRIVATE_HDRS
    bzlib_private.h
)
set(BZIP2_SRCS
    blocksort.c
    huffman.c
    crctable.c
    randtable.c
    compress.c
    decompress.c
    bzlib.c
)

# parse the full version number from bzlib.h and include in BZIP2_FULL_VERSION
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/bzlib.h _bzlib_h_contents)
string(REGEX REPLACE ".*bzip2/libbzip2 version ([-0-9A-Za-z.]+) of .*"
    "\\1" BZIP2_FULL_VERSION ${_bzlib_h_contents})

add_library(bz2 STATIC ${BZIP2_SRCS} ${BZIP2_PUBLIC_HDRS} ${BZIP2_PRIVATE_HDRS})

install(TARGETS bz2 EXPORT LeapSerialTargets
  DESTINATION lib
  COMPONENT LeapSerial
  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)
target_include_directories(
  bz2
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)
