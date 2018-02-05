// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include <array>
#include <cstdint>

namespace leap {
  std::array<uint8_t, 10> ToBase128(uint64_t val, size_t& ncb);
  uint8_t SizeBase128(uint64_t val);
  uint64_t FromBase128(uint8_t* data, size_t ncb);
}
