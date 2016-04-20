set(LeapSerialTest_SRCS
  AESStreamTest.cpp
  ArchiveJSONTest.cpp
  BoundedStreamTest.cpp
  BufferedStreamTest.cpp
  ChronoTypesTest.cpp
  CompressionStreamTest.cpp
  InheritanceTest.cpp
  ArchiveLeapSerialTest.cpp
  MapTest.cpp
  MemoryStreamTest.cpp
  OptionalTest.cpp
  PrettyPrintTest.cpp
  SerialCallbackTest.cpp
  SerialFormatTest.cpp
  SerializationTest.cpp
  SerializerEnumerationTest.cpp
  TestObject.h
  TestProtobufLS.hpp
)
add_pch(LeapSerialTest_SRCS "stdafx.h" "stdafx.cpp")

find_package(FlatBuffers QUIET)
if(FlatBuffers)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/TestObject_generated.h
    COMMAND FlatBuffers::flatc -c -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/TestObject.fbs
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/TestObject.fbs
    COMMENT "Generating TestObject_generated.h"
    VERBATIM
  )
else()
  file(WRITE TestObject_generated.h)
endif()

add_conditional_sources(
  LeapSerialTest_SRCS
  "FlatBuffers_FOUND"
  GROUP_NAME "FlatBuffers"
  FILES
  ArchiveFlatbufferTest.cpp
  TestObject.fbs
  TestObject_generated.h
)

option(LS_ENABLE_PROTOBUF_TESTS "Enable protobuf tests" ON)
option(LS_PROTOBUF_REQUIRED "Require protobuf tests" OFF)
if(LS_ENABLE_PROTOBUF_TESTS OR LS_PROTOBUF_REQUIRED)
  if(LS_PROTOBUF_REQUIRED)
    find_package(Protobuf 2.5.0 REQUIRED)
  else()
    find_package(Protobuf 2.5.0 QUIET)
  endif()
endif()

if(Protobuf)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/TestProtobuf.pb.cc ${CMAKE_CURRENT_SOURCE_DIR}/TestProtobuf.pb.h
    COMMAND Protobuf::protoc -I ${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/TestProtobuf.proto
    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/TestProtobuf.proto
    COMMENT "Generating TestProtobuf.pb.cc and TestProtobuf.pb.h"
    VERBATIM
  )
else()
  file(WRITE TestProtobuf.pb.h)
  file(WRITE TestProtobuf.pb.cc)
endif()

add_conditional_sources(
  LeapSerialTest_SRCS
  "Protobuf"
  GROUP_NAME "Protobuf"
  FILES
  ArchiveProtobufTest.cpp
  SchemaTest.cpp
  TestProtobuf.proto
  TestProtobuf.pb.h
  TestProtobuf.pb.cc
)

find_package(Threads)
add_executable(LeapSerialTest ${LeapSerialTest_SRCS} ../../gtest-all-guard.cpp)
target_link_libraries(LeapSerialTest LeapSerial ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME LeapSerialTest COMMAND $<TARGET_FILE:LeapSerialTest>)

if(FlatBuffers)
  target_link_libraries(LeapSerialTest FlatBuffers::FlatBuffers)
endif()

if(Protobuf)
  target_link_libraries(LeapSerialTest Protobuf::Protobuf)
endif()
