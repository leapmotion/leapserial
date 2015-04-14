#include "stdafx.h"
#include <gtest/gtest.h>

#include "ArchiveFlatbuffer.h"
#include "Serializer.h"
#include "TestObject.h"
#include "TestObject_generated.h"

#include "flatbuffers/flatbuffers.h"
#include <sstream>

class ArchiveFlatbufferTest :
  public testing::Test 
{};

using namespace Test;
TEST_F(ArchiveFlatbufferTest, ReadFromFlatbufferMessage) {

  //Generate the flatbuffer message
  flatbuffers::FlatBufferBuilder fbb;

  auto strOffset = fbb.CreateString("I am the queen of France");
  flatbuffers::Offset<flatbuffers::String> strings[3];
  strings[0] = fbb.CreateString("I wanna drink goat's blood!");
  strings[1] = fbb.CreateString("But Timmy, it's only Tuesday");
  strings[2] = fbb.CreateString("Awww....");
  auto vecOffset = fbb.CreateVector(strings, 3);

  auto testObj = Flatbuffer::CreateTestObject(fbb,
    true,
    Flatbuffer::TestEnum_VALUE_THREE,
    'x',
    -42,
    std::numeric_limits<uint64_t>::max() - 42,
    strOffset,
    vecOffset
    );

  fbb.Finish(testObj);
  
  std::string fbString;
  auto bufferPointer = (const char*)fbb.GetBufferPointer();
  fbString.assign(bufferPointer, bufferPointer + fbb.GetSize());

  std::stringstream ss(fbString, std::ios::in | std::ios::out | std::ios::binary);

  Native::TestObject parsedObj;

  leap::Deserialize<leap::IArchiveFlatbuffer>(ss, parsedObj);

  ASSERT_TRUE(parsedObj.unserialized.empty());
  ASSERT_EQ(parsedObj.a, true);
  ASSERT_EQ(parsedObj.b, 'x');
  ASSERT_EQ(parsedObj.c, -42);
  ASSERT_EQ(parsedObj.d, std::numeric_limits<uint64_t>::max() - 42);
  ASSERT_EQ(parsedObj.e, "I am the queen of France");
  ASSERT_EQ(parsedObj.f[0], "I wanna drink goat's blood!");
  ASSERT_EQ(parsedObj.f[1], "But Timmy, it's only Tuesday");
  ASSERT_EQ(parsedObj.f[2], "Awww....");
  ASSERT_EQ(static_cast<Native::TestEnum>(parsedObj.g), Native::VALUE_THREE);
}

TEST_F(ArchiveFlatbufferTest, WriteAlignment) {
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);

  flatbuffers::FlatBufferBuilder fbb;

  fbb.PushElement((uint8_t)1);
  fbb.PushElement((uint32_t)20);
  fbb.PushElement((uint64_t)123);
  fbb.PushElement((uint16_t)3);
  fbb.PushElement((uint32_t)22);

  std::string fbString;
  auto bufferPointer = (const char*)fbb.GetBufferPointer();
  fbString.assign(bufferPointer, bufferPointer + fbb.GetSize());

  leap::OArchiveFlatbuffer oarchive(ss);
  oarchive.WriteInteger(1, 1);
  oarchive.WriteInteger(20, 4);
  oarchive.WriteInteger(123, 8);
  oarchive.WriteInteger(3, 2);
  oarchive.WriteInteger(22, 4);
  oarchive.Finish();

  const auto string = ss.str();

  ASSERT_EQ(string, fbString);
}

TEST_F(ArchiveFlatbufferTest, WriteFlatbufferMessage) {
  Native::TestObject nativeObj = {
    "Wankel Rotary Engine",
    false,
    'x',
    -42,
    std::numeric_limits<uint64_t>::max() - 42,
    "I am the queen of France",
    { "I wanna drink goat's blood!", "But Timmy, It's only Tuesday", "Awww...." },
    Native::VALUE_TWO
  };

  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  leap::Serialize<leap::OArchiveFlatbuffer>(ss, nativeObj);

  auto bufferString = ss.str();
  auto fbObj = Flatbuffer::GetTestObject(bufferString.data());

  ASSERT_EQ(!!fbObj->a(), false);
  ASSERT_EQ(fbObj->b(), 'x');
  ASSERT_EQ(fbObj->c(), -42);
  ASSERT_EQ(fbObj->d(), std::numeric_limits<uint64_t>::max() - 42);
  ASSERT_STREQ(fbObj->e()->c_str(), "I am the queen of France");
  ASSERT_STREQ(fbObj->f()->Get(0)->c_str(), "I wanna drink goat's blood!");
  ASSERT_STREQ(fbObj->f()->Get(1)->c_str(), "But Timmy, It's only Tuesday");
  ASSERT_STREQ(fbObj->f()->Get(2)->c_str(), "Awww....");
  ASSERT_EQ(fbObj->g(), static_cast<Flatbuffer::TestEnum>(Native::VALUE_TWO)); }
