set(LeapSerial_SRCS
  AESStream.h
  AESStream.cpp
  Allocation.h
  Allocation.cpp
  Archive.h
  Archive.cpp
  ArchiveFlatbuffer.h
  ArchiveFlatbuffer.cpp
  ArchiveJSON.h
  ArchiveJSON.cpp
  ArchiveProtobuf.h
  base.h
  BoundedStream.h
  BoundedStream.cpp
  BufferedStream.h
  BufferedStream.cpp
  CompressionStream.h
  CompressionStream.cpp
  Descriptor.h
  Descriptor.cpp
  descriptors.h
  descriptors.cpp
  field_descriptor.h
  field_serializer.h
  field_serializer_t.h
  FilterStreamBase.h
  FilterStreamBase.cpp
  ForwardingStream.h
  IArchiveProtobuf.cpp
  OArchiveProtobuf.cpp
  IArchiveImpl.h
  IArchiveImpl.cpp
  IArray.h
  IDictionary.h
  IInputStream.h
  IOutputStream.h
  LeapSerial.h
  MemoryStream.h
  MemoryStream.cpp
  OArchiveImpl.h
  OArchiveImpl.cpp
  optional.h
  ProtobufType.h
  ProtobufUtil.cpp
  ProtobufUtil.hpp
  SchemaWriterProtobuf.h
  SchemaWriterProtobuf.cpp
  serial_traits.h
  serialization_error.h
  serialization_error.cpp
  StreamAdapter.h
  StreamAdapter.cpp
  Utility.hpp
  Utility.cpp
)

rewrite_header_paths(LeapSerial_SRCS)
add_pch(LeapSerial_SRCS "stdafx.h" "stdafx.cpp")
add_library(LeapSerial ${LeapSerial_SRCS})
target_link_libraries(LeapSerial aes zlib)
add_subdirectory(test)

install(TARGETS LeapSerial EXPORT LeapSerialTargets
  DESTINATION lib
  COMPONENT LeapSerial
  CONFIGURATIONS ${CMAKE_CONFIGURATION_TYPES}
)

target_include_directories(
  LeapSerial
  INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>"
)
