// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"

namespace leap {
  namespace Protobuf {
    enum class serial_type {
      // Null type, this type is never serialized
      ignored = -1,

      varint = 0,
      b64 = 1,
      string = 2,
      b32 = 5
    };

    inline serial_type GetSerialType(::leap::serial_atom prim) {
      switch(prim) {
        case serial_atom::ignored:
          return serial_type::ignored;
        case serial_atom::boolean:
        case serial_atom::i8:
        case serial_atom::i16:
        case serial_atom::i32:
        case serial_atom::i64:
          return serial_type::varint;
        case serial_atom::f32:
          return serial_type::b32;
        case serial_atom::f64:
        case serial_atom::f80:
          return serial_type::b64;
        default:
          return serial_type::string;
      }
    }
  }
}
