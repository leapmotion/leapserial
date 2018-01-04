// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "TestObject.h"
#include <LeapSerial/LeapSerial.h>
#include <gtest/gtest.h>

using namespace Test;

class SerialFormatTest :
  public testing::Test
{
public:
  static const Native::TestObject s_nativeObj;
};

const Native::TestObject SerialFormatTest::s_nativeObj = {
  "Wankel Rotary Engine",
  false,
  'x',
  -43,
  std::numeric_limits<uint64_t>::max() - 42,
  "I am the queen of France",
  { "I wanna drink goat's blood!", "But Timmy, It's only Tuesday", "Awww...." },
  Native::VALUE_TWO
};

static const char* LeapArchiveString =
  "\x0a\x8b\x01\x00\x10\x78\x18\xd5\xff\xff\xff\xff\xff\xff\xff\xff\x01\x20\xd5\xff\xff"
  "\xff\xff\xff\xff\xff\xff\x01\x2a\x1c\x18\x00\x00\x00\x49\x20\x61\x6d\x20\x74\x68\x65"
  "\x20\x71\x75\x65\x65\x6e\x20\x6f\x66\x20\x46\x72\x61\x6e\x63\x65\x32\x4f\x03\x00\x00"
  "\x00\x1b\x00\x00\x00\x49\x20\x77\x61\x6e\x6e\x61\x20\x64\x72\x69\x6e\x6b\x20\x67\x6f"
  "\x61\x74\x27\x73\x20\x62\x6c\x6f\x6f\x64\x21\x1c\x00\x00\x00\x42\x75\x74\x20\x54\x69"
  "\x6d\x6d\x79\x2c\x20\x49\x74\x27\x73\x20\x6f\x6e\x6c\x79\x20\x54\x75\x65\x73\x64\x61"
  "\x79\x08\x00\x00\x00\x41\x77\x77\x77\x2e\x2e\x2e\x2e\x0a\x01\x21";

TEST_F(SerialFormatTest, ReadFromLeapArchive) {
  Native::TestObject testObj;
  Native::TestObject testObj2;
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  std::stringstream ss2(std::ios::in | std::ios::out | std::ios::binary);

  ss.write(LeapArchiveString, 142);
  leap::Serialize(ss2, s_nativeObj);

  leap::Deserialize(ss, testObj);
  leap::Deserialize(ss2, testObj2);
  ASSERT_TRUE(testObj.unserialized.empty());
  ASSERT_EQ(testObj.a, s_nativeObj.a);
  ASSERT_EQ(testObj.b, s_nativeObj.b);
  ASSERT_EQ(testObj.c, s_nativeObj.c);
  ASSERT_EQ(testObj.d, s_nativeObj.d);
  ASSERT_EQ(testObj.e, s_nativeObj.e);
  ASSERT_EQ(testObj.f, s_nativeObj.f);
  ASSERT_EQ(testObj.g, s_nativeObj.g);
}
