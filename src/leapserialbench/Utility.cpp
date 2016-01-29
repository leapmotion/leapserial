// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "Utility.h"
#include <iomanip>
#include <iostream>

using namespace std::chrono;

std::ostream& operator<<(std::ostream& os, const format_duration_t<long long, std::nano>& rhs) {
  if (minutes(1) <= rhs.duration)
    return os << duration_cast<minutes>(rhs.duration).count() << " min";
  if (seconds(100) <= rhs.duration)
    return os << duration_cast<seconds>(rhs.duration).count() << " s";
  if (seconds(1) <= rhs.duration) {
    auto fraction = duration_cast<milliseconds>(rhs.duration).count() % 1000;
    return os << duration_cast<seconds>(rhs.duration).count() << '.' << std::setfill('0') << std::setw(3) << fraction << " s";
  }
  if (milliseconds(1) <= rhs.duration)
    return os << duration_cast<milliseconds>(rhs.duration).count() << " ms";
  if (microseconds(1) <= rhs.duration)
    return os << duration_cast<microseconds>(rhs.duration).count() << " us";
  return os << rhs.duration.count() << " ns";
}
