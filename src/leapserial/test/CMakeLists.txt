set(LeapSerialTest_SRCS
  ArchiveJSONTest.cpp
  LeapArchiveTest.cpp
  PrettyPrintTest.cpp
  SerialFormatTest.cpp
  SerializationTest.cpp
  TestObject.h
)
add_pch(LeapSerialTest_SRCS "stdafx.h" "stdafx.cpp")

add_conditional_sources(
  LeapSerialTest_SRCS
  "FlatBuffers_FOUND"
  GROUP_NAME "FlatBuffers"
  FILES
  ArchiveFlatbufferTest.cpp
  TestObject.fbs
  TestObject_generated.h
)

find_package(FlatBuffers QUIET)
if(${FlatBuffers_FOUND})
  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject_generated.h
                    COMMAND FlatBuffers::flatc -c -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject.fbs
                    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject.fbs
                    COMMENT "Generating TestObject_generated.h"
                    VERBATIM )
else()
  # Create a default header, here, so cmake doesn't complain
  file(WRITE TestObject_generated.h)
endif()

find_package(Threads)
add_executable(LeapSerialTest ${LeapSerialTest_SRCS} ../../gtest-all-guard.cpp)
target_link_libraries(LeapSerialTest LeapSerial ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME LeapSerialTest COMMAND $<TARGET_FILE:LeapSerialTest>)
