#pragma once
#include "Archive.h"

namespace leap {
  namespace LeapArchive {
    enum class serial_type {
      // Null type, this type is never serialized
      ignored = -1,
  
      varint = 0,
      b64 = 1,
      string = 2,
      b32 = 5
    };
    
    inline serial_type GetSerialType(leap::serial_primitive prim) {
      switch(prim) {
        case serial_primitive::ignored:
          return serial_type::ignored;
        case serial_primitive::boolean:
        case serial_primitive::i8:
        case serial_primitive::i16:
        case serial_primitive::i32:
        case serial_primitive::i64:
          return serial_type::varint;
        case serial_primitive::f32:
          return serial_type::b32;
        case serial_primitive::f64:
          return serial_type::b64;
        default:
          return serial_type::string;
      }
    }
  }
}
