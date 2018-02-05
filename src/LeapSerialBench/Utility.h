// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include <chrono>
#include <iosfwd>

template<typename Rep, typename Period>
struct format_duration_t {
  std::chrono::duration<Rep, Period> duration;
};

template<typename Rep, typename Period>
format_duration_t<Rep, Period> format_duration(std::chrono::duration<Rep, Period> duration) {
  return{ duration };
}

std::ostream& operator<<(std::ostream& os, const format_duration_t<long long, std::nano>& rhs);

template<typename Rep, typename Period>
std::ostream& operator<<(std::ostream& os, const format_duration_t<Rep, Period>& rhs) {
  return os << format_duration_t<long long, std::nano>{ rhs.duration };
}
