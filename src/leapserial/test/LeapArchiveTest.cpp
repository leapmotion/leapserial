// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "OArchiveImpl.h"
#include "IArchiveImpl.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>
#include <sstream>

class LeapArchiveTest :
  public testing::Test 
{};

struct MyRepeatedStructure {
  int* pv;

  static leap::descriptor GetDescriptor(void) {
    return{
      &MyRepeatedStructure::pv
    };
  }
};

struct MySimpleStructure {
  int a;
  int b;

  MySimpleStructure* selfReference;
  MyRepeatedStructure* x;
  MyRepeatedStructure* y;

  std::vector<int> someIntegers;
  std::vector<MyRepeatedStructure*> vectorOfChildren;
  std::string myString;
  std::wstring myWString;

  static leap::descriptor GetDescriptor(void) {
    return{
      &MySimpleStructure::a,
      &MySimpleStructure::b,
      &MySimpleStructure::selfReference,
      &MySimpleStructure::x,
      &MySimpleStructure::y,
      &MySimpleStructure::someIntegers,
      &MySimpleStructure::vectorOfChildren,
      &MySimpleStructure::myString,
      &MySimpleStructure::myWString
    };
  }
};

TEST_F(LeapArchiveTest, ExpectedByteCount) {
  MySimpleStructure mss;
  mss.a = 909;
  mss.b = -1;
  mss.myString = "Hello world";
  mss.myWString = L"Hello world again!";
  mss.selfReference = nullptr;
  mss.x = nullptr;
  mss.y = nullptr;

  std::ostringstream os;
  leap::OArchiveImpl oarch(os);

  // Predict the number of bytes required to serialize:
  leap::descriptor desc = MySimpleStructure::GetDescriptor();
  uint64_t ncb = desc.size(oarch, &mss);

  // Serialize first
  leap::Serialize(os, mss);

  // Verify size equivalence.  We have exactly two bytes of slack because the outermost object
  // is serialized with a type, identifier, and length field, and this takes up two bytes.
  std::string buf = os.str();
  ASSERT_EQ(buf.size(), ncb + 2);
}

void TestVarint(int64_t testNumber) {
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  leap::OArchiveImpl oArch(ss);
  leap::IArchiveImpl iArch(ss);

  oArch.WriteInteger(testNumber);
  auto output = iArch.ReadInteger(8);
  ASSERT_EQ(output, testNumber);
}

TEST_F(LeapArchiveTest, VarintTest) {
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);

  leap::OArchiveImpl oArch(ss);

  ASSERT_EQ(10, oArch.SizeInteger(-43));

  TestVarint(0);
  TestVarint(42);
  TestVarint(300);
  TestVarint(-42);
  TestVarint(std::numeric_limits<int64_t>::max());
  TestVarint(std::numeric_limits<int64_t>::min());
}

TEST_F(LeapArchiveTest, VarintDoubleCheck) {
  std::stringstream ss;
  leap::OArchiveImpl oarch(ss);

  oarch.WriteInteger(150, 8);

  std::string buf = ss.str();
  ASSERT_STREQ("\x96\x01", buf.c_str()) << "Varint serialization is incorrect";

  // Read operation should result in the same value
  leap::internal::Allocation<std::string> alloc;
  leap::IArchiveImpl iarch(ss);
  ASSERT_EQ(150, iarch.ReadInteger(8)) << "Read of a varint didn't return the original value";
}

TEST_F(LeapArchiveTest, VarintSizeExpectationCheck) {
#ifndef _MSC_VER
  ASSERT_EQ(
    29,
    __builtin_clzll(0x7FFFFFFFF)
    );
  ASSERT_EQ(
    28,
    __builtin_clzll(0x800000000)
    );
#endif
  std::stringstream ss;
  leap::OArchiveImpl oarch(ss);

  ASSERT_EQ(
    10,
    oarch.SizeInteger(-1)
    ) << "-1 should have maxed out the varint size requirement";

  ASSERT_EQ(
    1,
    oarch.SizeInteger(0)
    ) << "Boundary case failure";

  ASSERT_EQ(
    2,
    oarch.SizeInteger(128)
    ) << "Boundary case failure";

  ASSERT_EQ(
    6,
    oarch.SizeInteger(0x800000000ULL)
    ) << "Boundary case failure";

  ASSERT_EQ(
    5,
    oarch.SizeInteger(0x7FFFFFFFFULL)
    ) << "Boundary case failure";
}
