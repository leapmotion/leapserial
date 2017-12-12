cmake_minimum_required(VERSION 3.8)
include(version.cmake)
include("standard/Standard.cmake")

standard_project_preinit()
project(LeapSerial VERSION ${LeapSerial_VERSION})
standard_project_postinit()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules")
include(AddPCH)
include(ConditionalSources)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


# We have unit test projects via googletest, they're added in the places where they are defined
enable_testing()

add_definitions(-DGTEST_HAS_TR1_TUPLE=0)

# Recurse through source directories
include_directories(
  contrib
)
add_subdirectory(contrib)
add_subdirectory(src)

# Now we can generate the version and install stuff
generate_version()
combined_installer(
  VENDOR "Leap Motion"
  CONTACT "cmercenary@gmail.com"
)
