#include "stdafx.h"
#include "Serializer.h"
#include "TestObject.h"
#include "ArchiveJSON.h"
#include <gtest/gtest.h>

class ArchiveJSONTest :
  public testing::Test
{};

using namespace Test;

struct JSONTestObject {
  float a;
  int b;

  static leap::descriptor GetDescriptor(void) {
    return{
      { "field_a", &JSONTestObject::a },
      { "field_b", &JSONTestObject::b }
    };
  }
};

TEST_F(ArchiveJSONTest, WriteToJSON) {
  std::stringstream ss(std::ios::in | std::ios::out);

  JSONTestObject obj = { 1.1f, 200 };

  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  const auto json = ss.str();
  ASSERT_STREQ("{\"field_a\":1.1,\"field_b\":200}", json.c_str()) << "Output JSON was not precisely equal to the expected reference";
}

TEST_F(ArchiveJSONTest, VerifyPreciceJSON) {
  std::stringstream ss(std::ios::in | std::ios::out);

  JSONTestObject obj = { 1.1f, 200 };

  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  const auto string = ss.str();
  ASSERT_STREQ("{\"field_a\":1.1,\"field_b\":200}", string.c_str());
}

namespace {
  class NestedObject {
  public:
    int value = 201;

    static leap::descriptor GetDescriptor(void) {
      return{ { "x", &NestedObject::value } };
    }
  };

  class HasNestedObject {
  public:
    NestedObject obj;

    static leap::descriptor GetDescriptor(void) {
      return{
        { "a", &HasNestedObject::obj }
      };
    }
  };
}

TEST_F(ArchiveJSONTest, HasNestedObject) {
  HasNestedObject obj;

  std::stringstream ss;
  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  std::string json = ss.str();
  ASSERT_STREQ("{\"a\":{\"x\":201}}", json.c_str());
}