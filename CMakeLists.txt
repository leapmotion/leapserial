set(LeapResource_SRCS
  Allocation.h
  Allocation.cpp
  Archive.h
  Archive.cpp
  Descriptor.h
  Descriptor.cpp
  field_serializer.h
  IArchiveImpl.h
  IArchiveImpl.cpp
  OArchiveImpl.h
  OArchiveImpl.cpp
  Serializer.h
  serial_traits.h
)

set(LeapResourceTest_SRCS
  test/DescriptorTest.cpp
)

add_pch(LeapResource_SRCS "stdafx.h" "stdafx.cpp")

add_library(LeapResource ${LeapResource_SRCS})
target_link_libraries(LeapResource Autowiring::Autowiring)
set_property(TARGET LeapResource PROPERTY FOLDER "Utility")

if(BUILD_TESTING)
  add_pch(LeapResourceTest_SRCS "stdafx.h" "stdafx.cpp")
  add_executable(LeapResourceTest ${LeapResourceTest_SRCS} "../gtest-all-guard.cpp")
  target_strip(LeapResourceTest)
  target_link_libraries(LeapResourceTest LeapResource Autowiring::AutoTesting)
  set_property(TARGET LeapResourceTest PROPERTY FOLDER "Utility")
endif()
