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

static std::vector<std::string> GeneratePrettyJson(int tabWidth) {
  HasNestedObject obj;

  std::string json;
  {
    std::ostringstream os;
    leap::OArchiveJSON jsonAr(os);
    jsonAr.PrettyPrint = true;
    jsonAr.TabWidth = tabWidth;
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
  return lines;
}

static void VerifyLinesWithCharacter(const std::vector<std::string>& lines, char ch, size_t nch) {
  auto LineWithChar = [=] (const char* expected, const std::string& line, size_t w) {
    for (size_t i = 0; i < nch * w; i++)
      ASSERT_EQ(ch, line[i]) << "Whitespace mismatch";
    ASSERT_STREQ(expected, line.c_str() + nch * w);
  };

  // Verify we have the right number of lines
  ASSERT_EQ(6UL, lines.size()) << "Line count was unexpectedly short";

  // First and last lines are easy to check
  ASSERT_STREQ("{", lines.front().c_str()) << "First character of a json object was not {";
  ASSERT_STREQ("}", lines.back().c_str()) << "Last character of a json object was not }";

  // Formatting levels for the middle lines
  LineWithChar("\"a\":{", lines[1], 1);
  LineWithChar("\"a\":{", lines[1].c_str(), 1);
  LineWithChar("\"x\":201", lines[2].c_str(), 2);
  LineWithChar("},", lines[3].c_str(), 1);
  LineWithChar("\"b\":101", lines[4].c_str(), 1);
}

TEST_F(PrettyPrintTest, SimplePrettyPrintWithTabs) {
  auto lines = GeneratePrettyJson(0);
  VerifyLinesWithCharacter(lines, '\t', 1);
}

TEST_F(PrettyPrintTest, SimplePrettyPrintWithSpaces) {
  auto lines = GeneratePrettyJson(1);
  VerifyLinesWithCharacter(lines, ' ', 1);
}
