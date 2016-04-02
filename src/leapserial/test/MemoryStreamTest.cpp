// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <leapserial/MemoryStream.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <numeric>

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

TEST_F(MemoryStreamTest, UnboundedCopy) {
  leap::MemoryStream ms;
  for (uint8_t i = 0; i < 100; i++) {
    std::array<uint8_t, 1024> buf;
    std::iota(buf.begin(), buf.end(), i);
    ms.Write(buf.data(), sizeof(buf));
  }

  leap::MemoryStream msr;

  std::streamsize ncbWritten = -1;
  msr.Write(ms, ncbWritten);

  ASSERT_EQ(1024 * 100, ncbWritten);
  ASSERT_EQ(1024 * 100, msr.Length());
}
