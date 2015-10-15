// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <memory>
#include <stdexcept>

namespace leap {
  struct descriptor;
  enum class serial_atom;

  namespace protobuf {
    enum class WireType {
      Varint = 0,
      QuadWord = 1,
      LenDelimit = 2,
      StartGroup = 3,  // Deprecated
      EndGroup = 4,    // Deprecated
      DoubleWord = 5,

      // Nonstandard extension
      ObjReference = 7
    };

    protobuf::WireType ToWireType(serial_atom atom);

    struct serialization_error :
      public std::runtime_error
    {
      serialization_error(std::string what) :
        std::runtime_error(std::move(what))
      {}
      serialization_error(const descriptor& descriptor);
    };
  }
}
