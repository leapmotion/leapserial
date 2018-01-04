// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <LeapSerial/LeapSerial.h>
#include <gtest/gtest.h>
#include <array>
#include <sstream>
#include <type_traits>

struct HasChronoMembers {
  std::chrono::seconds one{ 4929 };
  std::chrono::nanoseconds two{ 4929 };
  std::chrono::duration<short, std::ratio<24 * 60 * 60, 1>> three{ 2 };
  std::chrono::duration<float> four{ 1.2f };

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasChronoMembers::one,
      &HasChronoMembers::two,
      &HasChronoMembers::three,
      &HasChronoMembers::four
    };
  }
};

TEST(ChronoTypesTest, DurationMembers) {
  const HasChronoMembers hcm{};

  std::stringstream ss;
  leap::Serialize(ss, hcm);
  auto re = leap::Deserialize<HasChronoMembers>(ss);

  ASSERT_EQ(hcm.one, re->one) << "Seconds did not deserialize properly";
  ASSERT_EQ(hcm.two, re->two) << "Nanoseconds did not deserialize properly";
  ASSERT_EQ(hcm.three, re->three) << "Custom days duration did not deserialize properly";
  ASSERT_EQ(hcm.four, re->four) << "Floating point duration did not deserialize properly";
}
