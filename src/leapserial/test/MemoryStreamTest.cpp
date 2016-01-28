// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "MemoryStream.h"
#include <gtest/gtest.h>

class MemoryStreamTest:
  public testing::Test
{};

TEST_F(MemoryStreamTest, WriteAndBack) {
  leap::MemoryStream ms;

  const char helloWorld[] = "Hello world!";
  ASSERT_TRUE(ms.Write(helloWorld, sizeof(helloWorld)));
  ASSERT_TRUE(ms.Write(helloWorld, sizeof(helloWorld)));

  char buf[sizeof(helloWorld)];
  ASSERT_EQ(sizeof(helloWorld), ms.Skip(sizeof(helloWorld)));
  ASSERT_EQ(sizeof(helloWorld), ms.Read(buf, sizeof(buf)));
  ASSERT_STREQ(helloWorld, buf);
}
