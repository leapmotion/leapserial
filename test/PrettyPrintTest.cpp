#include "stdafx.h"
#include "Serializer.h"
#include "LeapSerial/ArchiveJSON.h"
#include <gtest/gtest.h>

class PrettyPrintTest:
  public testing::Test
{};

namespace {

class NestedObject {
public:
  int value = 201;

  static leap::descriptor GetDescriptor(void) {
    return {
      {"x", &NestedObject::value}
    };
  }
};

class HasNestedObject {
public:
  NestedObject obj;
  int val = 101;

  static leap::descriptor GetDescriptor(void) {
    return{
      {"a", &HasNestedObject::obj},
      {"b", &HasNestedObject::val}
    };
  }
};

}

TEST_F(PrettyPrintTest, SimplePrettyPrint) {
  HasNestedObject obj;

  std::string json;
  {
    std::ostringstream os;
    leap::OArchiveJSON jsonAr(os);
    jsonAr.PrettyPrint = true;
    jsonAr.WriteObject(
      leap::field_serializer_t<HasNestedObject>::GetDescriptor(),
      &obj
    );
    json = os.str();
  }

  // Read one line at a time
  std::vector<std::string> lines;
  std::istringstream is(json);
  for (std::string line; std::getline(is, line); lines.push_back(std::move(line)));

  // Verify we have the right number of lines
  ASSERT_EQ(6UL, lines.size()) << "Line count was unexpectedly short";

  // First and last lines are easy to check
  ASSERT_STREQ("{", lines.front().c_str()) << "First character of a json object was not {";
  ASSERT_STREQ("}", lines.back().c_str()) << "Last character of a json object was not }";

  ASSERT_STREQ("\t\"a\":{", lines[1].c_str());
  ASSERT_STREQ("\t\t\"x\":201", lines[2].c_str());
  ASSERT_STREQ("\t},", lines[3].c_str());
  ASSERT_STREQ("\t\"b\":101", lines[4].c_str());
}
