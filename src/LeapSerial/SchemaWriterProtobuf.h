// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IOutputStream.h"
#include <memory>
#include <unordered_set>
#include <vector>

namespace leap {
  struct descriptor;
  struct field_serializer;

  namespace protobuf {
    enum class Version {
      Proto1,
      Proto2,
      Proto3
    };
  }

  /// <summary>
  /// Writes the complete schema rooted at some specified descriptor
  /// </summary>
  class SchemaWriterProtobuf
  {
  public:
    SchemaWriterProtobuf(const descriptor& desc, protobuf::Version version = protobuf::Version::Proto1);

    const descriptor& Desc;
    const protobuf::Version Version;

  private:
    // Current tab level
    size_t tabLevel = 0;

    // Types to be written:
    std::vector<const descriptor*> awaiting;

    // Types already encountered:
    std::unordered_set<const descriptor*> encountered;

    // Synthetic types we had to create while parsing:
    std::vector<std::unique_ptr<descriptor>> synthetic;
    std::vector<std::string> syntheticNames;

    // Manipulators
    struct indent;
    struct name;
    struct type;

  public:
    void Write(IOutputStream& os);
    void Write(std::ostream& os);
  };

  class SchemaWriterProtobuf2:
    public SchemaWriterProtobuf
  {
  public:
    SchemaWriterProtobuf2(const descriptor& desc) :
      SchemaWriterProtobuf(desc, protobuf::Version::Proto2)
    {}
  };
}
