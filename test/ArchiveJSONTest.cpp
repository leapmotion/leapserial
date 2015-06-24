#include "stdafx.h"
#include <gtest/gtest.h>

#include "Serializer.h"
#include "TestObject.h"

#include "ArchiveJSON.h"
#include "DataStructures/Value.h"

class ArchiveJSONTest :
  public testing::Test
{};

using namespace Test;

struct JSONTestObject {
  float a, b;

  static leap::descriptor GetDescriptor(void) {
    return{
      { "field_a", &JSONTestObject::a },
      { "field_b", &JSONTestObject::b }
    };
  }
};

TEST_F(ArchiveJSONTest, WriteToJSON) {
  std::stringstream ss(std::ios::in | std::ios::out);

  JSONTestObject obj = { 1.1f, 2.2f };

  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  const auto string = ss.str();

  Value v = Value::FromJSON(string);

  ASSERT_TRUE(v.IsHash()) << "Object was not written as a hash";
  const auto a = v.HashGet("field_a");
  const auto b = v.HashGet("field_b");
  ASSERT_TRUE(a.IsNumeric() && b.IsNumeric()) << "Field types were not determined correctly";
  ASSERT_EQ(v.HashGetAs<float>("field_a"), obj.a) << "Value a not written correctly";
  ASSERT_EQ(v.HashGetAs<float>("field_b"), obj.b) << "Value b not written correctly";
}