// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <LeapSerial/LeapSerial.h>
#include <LeapSerial/BufferedStream.h>
#include <LeapSerial/CompressionStream.h>
#include <LeapSerial/ForwardingStream.h>
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

class CompressionStreamTest:
  public testing::Test
{};

namespace {
  struct SimpleStruct {
    SimpleStruct(void) = default;
    SimpleStruct(SimpleStruct&& rhs) :
      value(std::move(rhs.value))
    {}

    std::string value;

    static leap::descriptor GetDescriptor(void) {
      return{
        &SimpleStruct::value
      };
    }
  };
}

static const char sc_ryecatcher[] = "I thought what I'd do is I'd pretend I was one of those deaf-mutes";

static SimpleStruct MakeSimpleStruct(size_t r) {
  SimpleStruct val;
  val.value.resize((sizeof(sc_ryecatcher) - 1) << r);

  // Reduplicate 2^r times
  char* p = &val.value[0];
  for (size_t i = 1 << r; i--;) {
    memcpy(p, sc_ryecatcher, sizeof(sc_ryecatcher) - 1);
    p += sizeof(sc_ryecatcher) - 1;
  }
  return val;
}

TEST_F(CompressionStreamTest, KnownValueRoundTrip) {
  std::vector<uint8_t> vec(0x100);
  std::iota(vec.begin(), vec.end(), 0);

  std::stringstream ss;

  {
    leap::CompressionStream<leap::Zlib> cs(
      leap::make_unique<leap::OutputStreamAdapter>(ss)
    );
    cs.Write(vec.data(), vec.size());
  }

  ss.seekg(0);

  leap::DecompressionStream<leap::Zlib> ds{
    leap::make_unique<leap::InputStreamAdapter>(ss)
  };

  std::vector<uint8_t> read(vec.size());
  ASSERT_EQ(vec.size(), ds.Read(read.data(), read.size())) << "Did not read expected number of bytes";
  ASSERT_EQ(vec, read) << "Reconstructed vector was not read back intact";
}

TEST_F(CompressionStreamTest, CompressionPropCheck) {
  auto val = MakeSimpleStruct(4);

  std::stringstream ss;
  {
    leap::CompressionStream<leap::Zlib> cs{
      leap::make_unique<leap::OutputStreamAdapter>(ss),
      9
    };
    leap::Serialize(cs, val);
  }

  std::string str = ss.str();
  ASSERT_GT(val.value.size(), str.size()) << "Compression call did not actually compress anything";
}

class CompressionStreamTestF :
  public testing::TestWithParam<int>
{};

TEST_P(CompressionStreamTestF, RoundTrip) {
  auto val = MakeSimpleStruct(GetParam());

  std::stringstream ss;
  {
    leap::CompressionStream<leap::Zlib> cs{
      leap::make_unique<leap::OutputStreamAdapter>(ss)
    };
    leap::Serialize(cs, val);
  }

  SimpleStruct reacq;
  {
    leap::DecompressionStream<leap::Zlib> ds{
      leap::make_unique<leap::InputStreamAdapter>(ss)
    };
    leap::Deserialize(ds, reacq);
  }

  ASSERT_EQ(val.value, reacq.value);
}

TEST_P(CompressionStreamTestF, TruncatedStreamTest) {
  auto val = MakeSimpleStruct(GetParam());

  std::string str;
  {
    std::stringstream ss;
    leap::CompressionStream<leap::Zlib> cs{
      leap::make_unique<leap::OutputStreamAdapter>(ss)
    };
    leap::Serialize(cs, val);
    str = ss.str();
  }

  str.resize(str.size() / 2);
  ASSERT_FALSE(str.empty());

  // Deserialization should cause a problem
  SimpleStruct reacq;
  {
    std::stringstream ss(std::move(str));
    leap::DecompressionStream<leap::Zlib> ds{
      leap::make_unique<leap::InputStreamAdapter>(ss)
    };
    ASSERT_ANY_THROW(leap::Deserialize(ds, reacq));
  }
}

INSTANTIATE_TEST_CASE_P(
  MultiRoundTrip,
  CompressionStreamTestF,
  ::testing::Values(4, 18)
);

TEST_F(CompressionStreamTest, EofCheck) {
  char buf[200];
  leap::BufferedStream bs(buf, sizeof(buf));
  leap::CompressionStream<leap::Zlib> cs{ leap::make_unique<leap::ForwardingOutputStream>(bs) };
  leap::DecompressionStream<leap::Zlib> dcs{ leap::make_unique<leap::ForwardingInputStream>(bs) };

  // EOF not initially set until a read is attempted
  ASSERT_FALSE(dcs.IsEof());
  ASSERT_EQ(0, dcs.Read(buf, 1));
  ASSERT_TRUE(dcs.IsEof());

  // Write, but EOF flag should be sticky
  ASSERT_TRUE(cs.Write("a", 1));
  cs.Flush();
  ASSERT_TRUE(dcs.IsEof());
  ASSERT_EQ(1, dcs.Read(buf, 1));
  ASSERT_FALSE(dcs.IsEof());

  // Write again and attempt to read more than is available--this should also set EOF
  cs.Write("abc", 3);
  cs.Flush();
  ASSERT_EQ(3, dcs.Read(buf, sizeof(buf)));
  ASSERT_TRUE(dcs.IsEof());
}
