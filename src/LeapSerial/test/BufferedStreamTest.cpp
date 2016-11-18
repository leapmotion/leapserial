// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <LeapSerial/BufferedStream.h>
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

TEST_F(BufferedStreamTest, ReadWithNoWrite) {
  char helloWorld[] = "Hello world!";
  leap::BufferedStream bs{ helloWorld, sizeof(helloWorld), sizeof(helloWorld) - 1 };

  char buf[sizeof(helloWorld) * 2] = {};
  ASSERT_EQ(sizeof(helloWorld) - 1, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ(helloWorld, buf);

  ASSERT_EQ(0, bs.Read(buf, sizeof(buf)));
  ASSERT_TRUE(bs.IsEof());
}

TEST_F(BufferedStreamTest, EofCheck) {
  char buf[200];
  leap::BufferedStream bs{ buf, sizeof(buf) };

  // EOF not initially set until a read is attempted
  ASSERT_FALSE(bs.IsEof());
  ASSERT_EQ(0, bs.Read(buf, 1));
  ASSERT_TRUE(bs.IsEof());

  // Write, but EOF flag should be sticky
  ASSERT_TRUE(bs.Write("a", 1));
  ASSERT_TRUE(bs.IsEof());
  ASSERT_EQ(1, bs.Read(buf, 1));
  ASSERT_FALSE(bs.IsEof());

  // Write again and attempt to read more than is available--this should also set EOF
  bs.Write("abc", 3);
  ASSERT_EQ(3, bs.Read(buf, sizeof(buf)));
  ASSERT_TRUE(bs.IsEof());
}

TEST_F(BufferedStreamTest, SeekCheck) {
  char helloWorld[] = "Hello world!";
  leap::BufferedStream bs{ helloWorld, sizeof(helloWorld), sizeof(helloWorld) - 1 };

  char buf[sizeof(helloWorld) * 2] = {};
  ASSERT_EQ(sizeof(helloWorld) - 1, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ(helloWorld, buf);

  bs.Clear();
  bs.Seek(0);
  memset(buf, 0, sizeof(buf));

  ASSERT_EQ(sizeof(helloWorld) - 1, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ(helloWorld, buf);

  bs.Clear();
  bs.Seek(5);
  memset(buf, 0, sizeof(buf));

  ASSERT_EQ(sizeof(helloWorld) - 6, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ(" world!", buf);

  bs.Seek(1);
  memset(buf, 0, sizeof(buf));
  ASSERT_EQ(sizeof(helloWorld) - 2, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ("ello world!", buf);

  bs.Seek(0);
  ASSERT_FALSE(bs.IsEof());
  bs.Seek(std::streamoff(-1)); //-1 is invalid
  ASSERT_FALSE(bs.IsEof());
  ASSERT_EQ(sizeof(helloWorld) - 1, bs.Read(buf, sizeof(buf)));
  ASSERT_STREQ(helloWorld, buf);

  bs.Seek(100);
  ASSERT_TRUE(bs.IsEof());
  ASSERT_EQ(0, bs.Read(buf, sizeof(buf)));
}
