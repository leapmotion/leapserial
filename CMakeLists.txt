cmake_minimum_required(VERSION 3.0)
include(version.cmake)
project(LeapSerial VERSION ${LeapSerial_VERSION})

include(CTest)
include(CheckTypeSize)

if(APPLE)
  option(LeapSerial_BUILD_FAT "Build fat binaries for LeapSerial" ON)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;i386" CACHE STRING "Mac OS X build architectures" FORCE)
endif()

# Need to classify the architecture before we run anything else
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
  set(LeapSerial_BUILD_ARM ON)
  set(LeapSerial_BUILD_ARCHITECTURES "arm")
  set(LeapSerial_BUILD_64 OFF)
elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64;i386")
  set(LeapSerial_BUILD_ARCHITECTURES x64 x86)
  set(LeapSerial_BUILD_64 ON)
elseif(CMAKE_SIZEOF_VOID_P STREQUAL 4)
  set(LeapSerial_BUILD_ARCHITECTURES "x86")
  set(LeapSerial_BUILD_64 OFF)
else()
  set(LeapSerial_BUILD_ARCHITECTURES "x64")
  set(LeapSerial_BUILD_64 ON)
endif()
message(STATUS "Using architecture: ${LeapSerial_BUILD_ARCHITECTURES}")

if(CMAKE_COMPILER_IS_GNUCC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.8")
    message(FATAL_ERROR "GCC version 4.8 minimum is required to build LeapSerial")
  endif()
elseif (MSVC)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "18.0")
    message(FATAL_ERROR "MSVC 2013 minimum is required to build LeapSerial")
  endif()
endif()

message(STATUS "Compiler version ${CMAKE_CXX_COMPILER_VERSION}")

# Always use c++11 compiler with hidden visibility
if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fvisibility=hidden")
endif()

# Clang needs special additional flags to build with C++11
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  message(STATUS "AppleClang C++11")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(STATUS "Clang C++11")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lstdc++")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message(STATUS "GCC C++11")
endif()

# Also need position-independent code to make things work correctly on certain 64-bit machines
if(NOT WIN32 AND LeapSerial_BUILD_64)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
include(AddPCH)
include(ConditionalSources)

get_filename_component(LEAPSERIAL_ROOT_DIR . ABSOLUTE)

# All of our binaries go to one place:  The binaries output directory.  We only want to tinker
# with this if we're building by ourselves, otherwise we just do whatever the enclosing project
# wants us to do.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Libraries not only all wind up in the libraries directory, but we also keep them all together
# here by putting them in the same place, regardless of whether they are debug or release.  This
# makes globbing them together much easier.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)

# ARM installations should have "arm" as the suffix for the generated libraries and should be
# position-independent
if(LeapSerial_BUILD_ARM)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

  foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${config} config)
    string(CONCAT CMAKE_${config}_POSTFIX "${CMAKE_${config}_POSTFIX}" "64")
  endforeach()
endif()

# 64-bit installations should suffix with 64 regardless of the CPU type (arm or intel)
if(LeapSerial_BUILD_64)
  foreach(config IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${config} config)
    string(CONCAT CMAKE_${config}_POSTFIX "${CMAKE_${config}_POSTFIX}" "64")
  endforeach()
endif()

# Postfix on all debug libraries should be "d"
set(CMAKE_DEBUG_POSTFIX d${CMAKE_DEBUG_POSTFIX})

# We have unit test projects via googletest, they're added in the places where they are defined
enable_testing()

# CMake configurations
configure_file(leapserial-config.cmake.in leapserial-config.cmake @ONLY)
configure_file(leapserial-configVersion.cmake.in leapserial-configVersion.cmake @ONLY)
configure_file(LeapSerialConfig.h.in ${PROJECT_SOURCE_DIR}/LeapSerial/LeapSerialConfig.h @ONLY)

# Recurse through source directories
include_directories(contrib)
add_subdirectory(contrib)
add_subdirectory(src)

# Export library
export(EXPORT LeapSerialTargets FILE LeapSerialTargets.cmake NAMESPACE LeapSerial::)

# Install leapserial-config.cmake and leapserial-configVersion.cmake
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/leapserial-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/leapserial-configVersion.cmake"
  DESTINATION "."
  COMPONENT LeapSerial
)

# Targets file is needed in order to describe how to link LeapSerial to the rest of the system
install(EXPORT LeapSerialTargets FILE LeapSerialTargets.cmake COMPONENT LeapSerial NAMESPACE LeapSerial:: DESTINATION cmake CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES})

# 64-bit installations get a different upgrade GUID
if(LeapSerial_BUILD_64)
  set(LeapSerial_GUID_LAST_CHAR E)
else()
  set(LeapSerial_GUID_LAST_CHAR D)
endif()

# This is the upgrade GUID.  Part of the GUID is derived from the major version number.  Any time
# the major version number is adjusted, the upgrade GUID changes.  This allows multiple versions
# of the same product to be installed on a user's system at the same time, but also means that
# manual uninstallation of older versions is required.
#
# For more information on the rationale for this process, see the discussion on semantic versioning
# found at http://semver.org/
SET(CPACK_WIX_UPGRADE_GUID "{060E5EDD-229${LeapSerial_VERSION_MAJOR}-4AD8-BAFA-A303D5696A2${LeapSerial_GUID_LAST_CHAR}}")

# Need a custom wix installation template so that we update the CMake package registry correctly
# Only really needed on Windows; Mac and Linux have pretty good default search behavior, so we
# leave those alone.
SET(CPACK_WIX_TEMPLATE "${CMAKE_SOURCE_DIR}/LeapSerial.wxs")
SET(CPACK_MONOLITHIC_INSTALL ON)

# Run the script that will grab the debug and release configurations and install them during packaging
set(CPACK_INSTALL_COMMANDS
  "${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Debug"
  "${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --config Release"
  "${CMAKE_COMMAND} -DBUILD_TYPE=Debug -P \\\"${CMAKE_SOURCE_DIR}/cmake_package.cmake\\\""
  "${CMAKE_COMMAND} -DBUILD_TYPE=Release -P \\\"${CMAKE_SOURCE_DIR}/cmake_package.cmake\\\""
)

# Pick the generator in an appropriate way
if(WIN32)
  set(CPACK_GENERATOR WIX ZIP)
elseif(APPLE)
  # TODO: Add Bundle as a generator here
  set(CPACK_GENERATOR TGZ)
else()
  set(CPACK_GENERATOR TGZ DEB)
endif()

# Packaging stuff, if an installer is being made instead
SET(CPACK_PACKAGE_VENDOR "Leap Motion")
SET(CPACK_PACKAGE_CONTACT "cmercenary@gmail.com")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LeapSerial")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR "${LeapSerial_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${LeapSerial_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${LeapSerial_VERSION_PATCH}")
SET(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "LeapSerial")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "LeapSerial")
INCLUDE(CPack)
