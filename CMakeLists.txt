set(LeapSerial_SRCS
  Allocation.h
  Allocation.cpp
  Archive.h
  Archive.cpp
  Descriptor.h
  Descriptor.cpp
  field_descriptor.h
  field_serializer.h
  field_serializer_t.h
  IArchiveImpl.h
  IArchiveImpl.cpp
  OArchiveImpl.h
  OArchiveImpl.cpp
  Serializer.h
  serial_traits.h
)

set(LeapSerialTest_SRCS
  test/SerializationTest.cpp
)

add_pch(LeapSerial_SRCS "stdafx.h" "stdafx.cpp")

add_library(LeapSerial ${LeapSerial_SRCS})
target_link_libraries(LeapSerial Autowiring::Autowiring)
set_property(TARGET LeapSerial PROPERTY FOLDER "Utility")

if(BUILD_TESTING)
  add_pch(LeapSerialTest_SRCS "stdafx.h" "stdafx.cpp")
  add_executable(LeapSerialTest ${LeapSerialTest_SRCS} "../gtest-all-guard.cpp")
  target_strip(LeapSerialTest)
  target_link_libraries(LeapSerialTest LeapSerial Autowiring::AutoTesting)
  set_property(TARGET LeapSerialTest PROPERTY FOLDER "Utility")
endif()
