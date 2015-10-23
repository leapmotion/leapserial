// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>

static_assert(
  std::is_same<
    std::iterator_traits<leap::descriptors::const_iterator>::value_type,
    leap::descriptor_entry
  >::value,
  "Iterator does not have a correct value type"
);
static_assert(
  std::is_same<
    std::iterator_traits<leap::descriptors::const_iterator>::reference,
    const leap::descriptor_entry&
  >::value,
  "Iterator does not have a correct reference type"
);

TEST(SerializerEnumerationTest, CanCount) {
  size_t i = 0;
  for (auto& cur : leap::descriptors{})
    i++;
  ASSERT_NE(0, i) << "Failed to enumerate any serialization descriptors";
}

namespace {
  struct HasADescriptor {
    int member;

    static leap::descriptor GetDescriptor(void) {
      return{
        &HasADescriptor::member
      };
    }
  };
}

TEST(SerializerEnumerationTest, CanFindAnonymousType) {
  for (auto& cur : leap::descriptors{})
    if (cur.ti == typeid(HasADescriptor))
      return;
  FAIL() << "Failed to find a descriptor for an anonymous type";
}
