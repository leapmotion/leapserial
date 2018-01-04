// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "TestObject.h"
#include <LeapSerial/ArchiveJSON.h>
#include <LeapSerial/LeapSerial.h>
#include <gtest/gtest.h>

class ArchiveJSONTest :
  public testing::Test
{};

using namespace Test;

namespace {
struct JSONTestObject {
  float a;
  int b;
  std::string c;

  static leap::descriptor GetDescriptor(void) {
    return{
      { "field_a", &JSONTestObject::a },
      { "field_b", &JSONTestObject::b },
      { "c", &JSONTestObject::c }
    };
  }
};
}

TEST_F(ArchiveJSONTest, VerifyPreciceJSON) {
  std::stringstream ss(std::ios::in | std::ios::out);

  JSONTestObject obj = { 1.1f, 200, "Hello world!" };

  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  const auto json = ss.str();
  ASSERT_STREQ(
    R"({"field_a":1.1,"field_b":200,"c":"Hello world!"})",
    json.c_str()
  );
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

namespace {
struct SimpleArray {
  std::vector<int> objArray;

  static leap::descriptor GetDescriptor(void) {
    return{
      { "a", &SimpleArray::objArray }
    };
  }
};
}

TEST_F(ArchiveJSONTest, ArrayWriter) {
  SimpleArray obj;
  obj.objArray = { 1, 2, 3, 999 };

  std::stringstream ss;
  leap::Serialize<leap::OArchiveJSON>(ss, obj);

  std::string json = ss.str();
  ASSERT_STREQ("{\"a\":[1,2,3,999]}", json.c_str());
}
