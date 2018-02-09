// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "RoundTrip.h"
#include "RoundTripData.h"
#include "RoundTrip.pb.h"
#include "Utility.h"
#include "aes/rijndael-alg-fst.h"
#include <LeapSerial/AESStream.h>
#include <LeapSerial/BufferedStream.h>
#include <LeapSerial/LeapSerial.h>

using namespace std::chrono;

static const std::array<uint8_t, 32> sc_key = { { 0x44, 0x99, 0x66 } };

RoundTrip::RoundTrip(void) {
  buffer.resize(ncbRead);
}

nanoseconds RoundTrip::RoundTripLeapSerial(void) {
  
  leap::BufferedStream stream{ buffer.data(), buffer.size(), buffer.size() };

  auto start = high_resolution_clock::now();
  uint8_t temp[1024];
  while (!stream.IsEof())
    stream.Read(temp, sizeof(temp));
  return high_resolution_clock::now() - start;
}

int RoundTrip::Benchmark(std::ostream& os) {
  os << (buffer.size() / (1024 * 1024)) << "MB buffer size" << std::endl;
  os << "Trivial read:   " << std::flush;
  os << format_duration(RoundTripLeapSerial()) << std::endl;
  return 0;
}
