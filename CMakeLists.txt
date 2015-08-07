set(LeapSerial_SRCS
  Allocation.h
  Allocation.cpp
  Archive.h
  ArchiveFlatbuffer.h
  ArchiveFlatbuffer.cpp
  ArchiveJSON.h
  ArchiveJSON.cpp
  base.h
  Descriptor.h
  Descriptor.cpp
  field_descriptor.h
  field_serializer.h
  field_serializer_t.h
  ProtobufType.h
  IArchiveImpl.h
  IArchiveImpl.cpp
  OArchiveImpl.h
  OArchiveImpl.cpp
  Serializer.h
  serial_traits.h
)

set(LeapSerialTest_SRCS
  test/ArchiveFlatbufferTest.cpp
  test/ArchiveJSONTest.cpp
  test/LeapArchiveTest.cpp
  test/PrettyPrintTest.cpp
  test/SerialFormatTest.cpp
  test/SerializationTest.cpp
  test/TestObject.h
  test/TestObject.fbs
  test/TestObject_generated.h
)

add_pch(LeapSerial_SRCS "stdafx.h" "stdafx.cpp")

add_library(LeapSerial ${LeapSerial_SRCS})
target_link_libraries(LeapSerial Autowiring::Autowiring)
set_property(TARGET LeapSerial PROPERTY FOLDER "Utility")

if(BUILD_TESTING)
  add_pch(LeapSerialTest_SRCS "stdafx.h" "stdafx.cpp")


  add_custom_command( OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject_generated.h
                    COMMAND FlatBuffers::flatc -c -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject.fbs
                    MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/test/TestObject.fbs
                    COMMENT "Generating TestObject_generated.h"
                    VERBATIM )

  add_executable(LeapSerialTest ${LeapSerialTest_SRCS} "../gtest-all-guard.cpp")
  target_link_libraries(LeapSerialTest LeapSerial Autowiring::AutoTesting Protobuf::Protobuf FlatBuffers::FlatBuffers DataStructures)
  set_property(TARGET LeapSerialTest PROPERTY FOLDER "Utility")
endif()
