set(
  LeapSerialBench_SRCS
  LeapSerialBench.cpp
  LeapSerialBench.h
  Benchmark.h
  Encryption.cpp
  Encryption.h
  RoundTrip.h
  RoundTrip.cpp
  RoundTripData.h
  RoundTrip.fbs
  RoundTrip.proto
  RoundTrip.pb.h
  RoundTrip_generated.h
  Utility.h
  Utility.cpp
)

add_custom_command(
  OUTPUT RoundTrip.pb.h
  COMMAND protobuf::protoc
  ARGS --cpp_out ${CMAKE_CURRENT_BINARY_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} RoundTrip.pb.cc}
  COMMENT "Generating ${generated_files} from ${proto_file}"
  VERBATIM
)

add_custom_command(
  OUTPUT RoundTrip_generated.h
  COMMAND FlatBuffers::flatc -c -I ${CMAKE_CURRENT_SOURCE_DIR} -o ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/RoundTrip.fbs
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/RoundTrip.fbs
  COMMENT "Generating TestObject_generated.h"
  VERBATIM
)

add_pch(LeapSerialBench_SRCS "stdafx.h" "stdafx.cpp")
add_executable(LeapSerialBench ${LeapSerialBench_SRCS})
target_link_libraries(LeapSerialBench LeapSerial)
