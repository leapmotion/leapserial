include_directories(
  # Need to make sure all of our tests can find gtest
  ${PROJECT_SOURCE_DIR}/contrib/gtest-1.7.0/fused-src
  ${PROJECT_SOURCE_DIR}/LeapSerial
  ${PROJECT_SOURCE_DIR}/src/LeapSerial
)

add_subdirectory(leapserial)
