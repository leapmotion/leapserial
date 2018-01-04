// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include <iosfwd>

class IBenchmark {
public:
  virtual int Benchmark(std::ostream& os) = 0;
};
