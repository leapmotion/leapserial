// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include <gtest/gtest-all.cc>

using namespace std;

int main(int argc, const char* argv[])
{
  auto& listeners = testing::UnitTest::GetInstance()->listeners();
  testing::InitGoogleTest(&argc, (char**)argv);
  return RUN_ALL_TESTS();
}
