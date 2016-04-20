// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "TestProtobufLS.hpp"
#include <gtest/gtest.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <LeapSerial/LeapSerial.h>
#include <array>
#include <iomanip>
#include <sstream>

namespace io = google::protobuf::io;
namespace cl = google::protobuf::compiler;

class CountsErrors :
  public cl::MultiFileErrorCollector
{
public:
  struct Bad {
    std::string filename;
    int line;
    int column;
    std::string message;
  };

  std::vector<Bad> badness;

  void AddError(const std::string& filename, int line, int column, const std::string& message) override {
    badness.push_back({ filename, line, column, message });
  }

  friend std::ostream& operator<<(std::ostream& os, const CountsErrors& rhs) {
    for (auto& bad : rhs.badness)
      os << bad.filename << ":" << bad.line << ':' << bad.column << ": " << bad.message << std::endl;
    return os;
  }
};

class MemorySourceTree :
  public cl::SourceTree
{
public:
  MemorySourceTree(const std::initializer_list<std::pair<const std::string, std::string>>& entries) :
    files(entries)
  {}

  std::unordered_map<std::string, std::string> files;

  io::ZeroCopyInputStream* Open(const std::string& filename) override {
    auto q = files.find(filename);
    if (q == files.end())
      return nullptr;
    return new io::ArrayInputStream{ q->second.data(), (int) q->second.length() };
  }
};

TEST(SchemaTest, BasicDescriptorSerialization) {
  std::stringstream ss;

  // This step is the only step that's required to dump a full schema in proto1
  // format.  The rest of this function is just to parse it
  leap::Serialize<leap::protobuf_v1>(ss, Person::GetDescriptor());

  // Now we have to parse the protobuf file
  const google::protobuf::FileDescriptor* fd;
  CountsErrors error_collector;
  MemorySourceTree mst{
    { "root.proto", ss.str() }
  };

  // Importer utility class does most of the work for us, we just need to tell it
  // how to find files and where to report issues
  cl::Importer imp{ &mst, &error_collector };
  fd = imp.Import("root.proto");
  ASSERT_NE(nullptr, fd) << error_collector;

  // This part verifies that the regenerated schema is correct.
  auto personDesc = fd->FindMessageTypeByName("Person");
  ASSERT_NE(nullptr, personDesc) << "Failed to find person message in schema";
  ASSERT_NE(nullptr, personDesc->FindFieldByName("name"));
  ASSERT_NE(nullptr, personDesc->FindFieldByName("id"));
  ASSERT_NE(nullptr, personDesc->FindFieldByName("email"));
  ASSERT_NE(nullptr, personDesc->FindFieldByName("phone"));
  ASSERT_NE(nullptr, personDesc->FindFieldByName("pets"));
}
