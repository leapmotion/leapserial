#include "stdafx.h"
#include <gtest/gtest.h>

#include "OArchiveImpl.h"
#include "IArchiveImpl.h"

#include <sstream>

class LeapArchiveTest :
  public testing::Test 
{};

void TestVarint(int64_t testNumber) {
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  leap::OArchiveImpl oArch(ss);
  leap::IArchiveImpl iArch(ss, nullptr);

  oArch.WriteVarint(testNumber);
  auto output = iArch.ReadVarint();
  ASSERT_EQ(output, testNumber);
}

TEST_F(LeapArchiveTest, VarintTest) {
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  int64_t number;

  leap::OArchiveImpl oArch(ss);
  leap::IArchiveImpl iArch(ss, &number);

  ASSERT_EQ(10, leap::OArchive::VarintSize(-43));

  TestVarint(0);
  TestVarint(42);
  TestVarint(300);
  TestVarint(-42);
  TestVarint(std::numeric_limits<int64_t>::max());
  TestVarint(std::numeric_limits<int64_t>::min());
}