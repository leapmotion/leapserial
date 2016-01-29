// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include "AESStream.h"
#include "ForwardingStream.h"
#include "MemoryStream.h"
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

static const std::array<uint8_t, 32> sc_key{ {0x99, 0x84, 0x49, 0x28} };

class AESStreamTest:
  public testing::Test
{};

namespace {
  struct SimpleStruct {
    std::string value;

    static leap::descriptor GetDescriptor(void) {
      return{
        &SimpleStruct::value
      };
    }
  };
}

static SimpleStruct MakeSimpleStruct(void) {
  SimpleStruct val;
  val.value = "I thought what I'd do is I'd pretend I was one of those deaf-mutes";

  // Reduplicate 2^4 times
  for (size_t i = 0; i < 4; i++)
    val.value += val.value;
  return val;
}

TEST_F(AESStreamTest, KnownValueRoundTrip) {
  std::vector<uint8_t> vec(0x100);
  std::iota(vec.begin(), vec.end(), 0);

  std::stringstream ss;

  {
    leap::AESEncryptionStream cs(
      leap::make_unique<leap::OutputStreamAdapter>(ss),
      sc_key
    );
    cs.Write(vec.data(), vec.size());
  }

  ss.seekg(0);

  leap::AESDecryptionStream ds{
    leap::make_unique<leap::InputStreamAdapter>(ss),
    sc_key
  };

  std::vector<uint8_t> read(vec.size());
  ASSERT_EQ(vec.size(), ds.Read(read.data(), read.size())) << "Did not read expected number of bytes";
  ASSERT_EQ(vec, read) << "Reconstructed vector was not read back intact";
}

TEST_F(AESStreamTest, ExactSizeCheck) {
  std::vector<uint8_t> vec(45, 0);

  std::stringstream ss;
  leap::AESEncryptionStream cs(
    leap::make_unique<leap::OutputStreamAdapter>(ss),
    sc_key
  );
  cs.Write(vec.data(), vec.size());

  std::string str = ss.str();
  ASSERT_EQ(vec.size(), str.size()) << "CFB-mode AES cipher should preserve the exact length of the input";
}

TEST_F(AESStreamTest, RoundTrip) {
  auto val = MakeSimpleStruct();

  std::stringstream ss;
  {
    leap::AESEncryptionStream cs{
      leap::make_unique<leap::OutputStreamAdapter>(ss),
      sc_key
    };
    leap::Serialize(cs, val);
  }
  SimpleStruct reacq;

  {
    leap::AESDecryptionStream ds{
      leap::make_unique<leap::InputStreamAdapter>(ss),
      sc_key
    };
    leap::Deserialize(ds, reacq);
  }

  ASSERT_EQ(val.value, reacq.value);
}

TEST_F(AESStreamTest, OverReadTest) {
  std::array<uint8_t, 32> key = { 0x99, 0x88 };

  leap::MemoryStream ms;
  {
    leap::AESEncryptionStream enc{
      std::unique_ptr<leap::IOutputStream> { new leap::ForwardingOutputStream{ ms } },
      key
    };
    enc.Write("abcd", 4);
  }

  leap::AESDecryptionStream dec{
    std::unique_ptr<leap::IInputStream> { new leap::ForwardingInputStream{ ms } },
    key
  };
  char buf[25] = {};
  std::streamsize n;
  ASSERT_NO_THROW(n = dec.Read(buf, sizeof(buf)));
  ASSERT_EQ(4, n);
  ASSERT_STREQ("abcd", buf);
}
