set(
  LeapSerialBench_SRCS
  LeapSerialBench.cpp
  LeapSerialBench.h
  Benchmark.h
  Encryption.cpp
  Encryption.h
  Utility.h
  Utility.cpp
)

add_pch(LeapSerialBench_SRCS "stdafx.h" "stdafx.cpp")
add_executable(LeapSerialBench ${LeapSerialBench_SRCS})
target_link_libraries(LeapSerialBench LeapSerial)
