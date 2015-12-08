// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>
#include <sstream>
#include <type_traits>

static_assert(leap::has_serializer<int>::value, "Single primitive");
static_assert(leap::has_serializer<std::map<int, int>>::value, "Map of primitives");
static_assert(leap::has_serializer<std::map<std::string, std::string>>::value, "Map of nonprimitives");
static_assert(leap::has_serializer<std::unordered_map<int, int>>::value, "Unordered map of primitives");
static_assert(leap::has_serializer<std::unordered_map<std::string, std::string>>::value, "Unordered map of nonprimitives");
static_assert(leap::serializer_is_irresponsible<std::map<std::string, std::vector<int*>>>::value, "Irresponsible allocator was not correctly inferred");
static_assert(!leap::serializer_is_irresponsible<std::map<std::string, std::vector<int>>>::value, "Irresponsibility incorrectly inferred in a responsible allocator");

struct DummyStruct {};
static_assert(!leap::has_serializer<std::map<int, DummyStruct>>::value, "A map of DummyStruct was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::unordered_map<int, DummyStruct>>::value, "An unordered_map of DummyStruct was incorrectly classified as having a valid serializer");

struct SimpleStruct {
  std::map<int, int> myMap;
  std::unordered_map<int, int> myUnorderedMap;

  static leap::descriptor GetDescriptor(void) {
    return{
      &SimpleStruct::myMap,
      &SimpleStruct::myUnorderedMap
    };
  }
};

TEST(MapTest, CanSerializeMaps) {
  std::stringstream ss;

  {
    SimpleStruct x;
    x.myMap[0] = 555;
    x.myMap[232] = 12;
    x.myUnorderedMap[29] = 44;
    x.myUnorderedMap[30] = 4014;
    leap::Serialize(ss, x);
  }

  // Deserialize, make sure map entries are in the right place
  auto x = leap::Deserialize<SimpleStruct>(ss);
  ASSERT_EQ(2UL, x->myMap.size()) << "Map has an incorrect number of elements";
  ASSERT_EQ(555, x->myMap[0]);
  ASSERT_EQ(12, x->myMap[232]);

  // Same for unordered map
  ASSERT_EQ(2UL, x->myUnorderedMap.size()) << "Unordered map has an incorrect number of elements";
  ASSERT_EQ(44, x->myUnorderedMap[29]);
  ASSERT_EQ(4014, x->myUnorderedMap[30]);
}
