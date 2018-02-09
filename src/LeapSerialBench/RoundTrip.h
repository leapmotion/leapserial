// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Benchmark.h"
#include <chrono>
#include <iosfwd>
#include <vector>
#include <cstddef>

class RoundTrip :
  public IBenchmark
{
public:
  RoundTrip(void);

private:
  const size_t ncbRead = 1024 * 1024 * 100;
  std::vector<uint8_t> buffer;

  std::chrono::nanoseconds RoundTripLeapSerial(void);
  std::chrono::nanoseconds RoundTripProtobuf(void);
  std::chrono::nanoseconds RoundTripFlatbuffer(void);
  std::chrono::nanoseconds RoundTripFlatbufferNative(void);
  std::chrono::nanoseconds RoundTripProtobufNative(void);

public:
  int Benchmark(std::ostream& os) override;
};
