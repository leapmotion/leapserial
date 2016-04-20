// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "Archive.h"
#include <iostream>

using namespace leap;

OArchive::OArchive(IOutputStream& os):
  os(os)
{}

OArchiveRegistry::OArchiveRegistry(IOutputStream& os) :
  OArchive(os)
{}

const char* leap::ToString(serial_atom atom) {
  switch (atom) {
  case serial_atom::ignored:
    return "[?]";
  case serial_atom::boolean:
    return "bool";
  case serial_atom::i8:
    return "i8";
  case serial_atom::ui8:
    return "ui8";
  case serial_atom::i16:
    return "i16";
  case serial_atom::ui16:
    return "ui16";
  case serial_atom::i32:
    return "i32";
  case serial_atom::ui32:
    return "ui32";
  case serial_atom::i64:
    return "i64";
  case serial_atom::ui64:
    return "ui64";
  case serial_atom::f32:
    return "f32";
  case serial_atom::f64:
    return "f64";
  case serial_atom::f80:
    return "f80";
  case serial_atom::reference:
    return "ref";
  case serial_atom::array:
    return "array";
  case serial_atom::string:
    return "string";
  case serial_atom::map:
    return "map";
  case serial_atom::descriptor:
    return "descr";
  case serial_atom::finalized_descriptor:
    return "DESCR";
  }
  throw std::invalid_argument("Attempted to ToString an unrecognized serial atom type");
}
