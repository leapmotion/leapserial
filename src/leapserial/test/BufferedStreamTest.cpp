// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "BufferedStream.h"
#include <gtest/gtest.h>

class BufferedStreamTest :
  public testing::Test
{};

TEST_F(BufferedStreamTest, WriteAndBack) {
  char buf[200];
  leap::BufferedStream bs{ buf, sizeof(buf) };

  const char helloWorld[] = "Hello world!";
  ASSERT_TRUE(bs.Write(helloWorld, sizeof(helloWorld)));
  ASSERT_TRUE(bs.Write(helloWorld, sizeof(helloWorld)));

  char reread[sizeof(helloWorld)];
  ASSERT_EQ(sizeof(helloWorld), bs.Skip(sizeof(helloWorld)));
  ASSERT_EQ(sizeof(helloWorld), bs.Read(reread, sizeof(reread)));
  ASSERT_STREQ(helloWorld, reread);
  ASSERT_STREQ(helloWorld, buf);
}
