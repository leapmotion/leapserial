// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "serialization_error.h"
#include <memory>
#include <stdexcept>

namespace leap {
  struct descriptor;
  enum class serial_atom;

  namespace internal {
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

      const char* ToProtobufField(serial_atom atom);

      struct serialization_error :
        public ::leap::serialization_error
      {
        serialization_error(std::string&& err);
        serialization_error(const descriptor& descriptor);
      };
    }
  }
}
