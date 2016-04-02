// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <leapserial/BoundedStream.h>
#include <leapserial/LeapSerial.h>
#include <gtest/gtest.h>

class BoundedStreamTest:
  public testing::Test
{};

TEST_F(BoundedStreamTest, NoReadPastLimit) {
  std::stringstream ss("one two");
  leap::BoundedInputStream bos {
    leap::make_unique<leap::InputStreamAdapter>(ss),
    3
  };

  uint8_t buf[10];
  ASSERT_EQ(3, bos.Read(buf, sizeof(buf))) << "Read more characters than should have been possible from the bounded input stream";
  ASSERT_TRUE(bos.IsEof()) << "Should have been at EOF after reading to the character limit";
}
