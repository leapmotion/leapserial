// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>
#include <array>
#include <sstream>
#include <type_traits>

TEST(PathologicalTest, ComplexMapOfStringToVectorTest) {
  std::stringstream ss;

  // Serialize the complex map type first
  {
    std::map<std::string, std::vector<int>> mm;
    mm["a"] = std::vector<int>{ 1, 2, 3 };
    mm["b"] = std::vector<int>{ 4, 5, 6 };
    mm["c"] = std::vector<int>{ 7, 8, 9 };

    leap::Serialize(ss, mm);
  }

  // Try to get it back:
  std::map<std::string, std::vector<int>> mm;
  leap::Deserialize(ss, mm);

  ASSERT_EQ(1UL, mm.count("a")) << "Deserialized map was missing an essential member";
  ASSERT_EQ(1UL, mm.count("b")) << "Deserialized map was missing an essential member";
  ASSERT_EQ(1UL, mm.count("c")) << "Deserialized map was missing an essential member";

  const auto& a = mm["a"];
  ASSERT_EQ(3UL, a.size()) << "Vector was sized incorrectly";

  const auto& c = mm["c"];
  ASSERT_EQ(3UL, c.size()) << "Vector was sized incorrectly";
  ASSERT_EQ(7, c[0]);
  ASSERT_EQ(8, c[1]);
  ASSERT_EQ(9, c[2]);
}
