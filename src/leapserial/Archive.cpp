// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "Archive.h"
#include <iostream>

using namespace leap;

std::streamsize InputStreamAdapter::Read(void* pBuf, std::streamsize ncb) {
  is.read((char*)pBuf, ncb);
  return is ? is.gcount() : -1;
}

std::streamsize InputStreamAdapter::Skip(std::streamsize ncb) {
  is.ignore(ncb);
  return is ? is.gcount() : -1;
}

bool OutputStreamAdapter::Write(const void* pBuf, std::streamsize ncb) {
  return (bool)os.write((const char*)pBuf, ncb);
}

const char* leap::ToString(serial_atom atom) {
  switch (atom) {
  case serial_atom::ignored:
    return "[?]";
  case serial_atom::boolean:
    return "bool";
  case serial_atom::i8:
    return "i8";
  case serial_atom::i16:
    return "i16";
  case serial_atom::i32:
    return "i32";
  case serial_atom::i64:
    return "i64";
  case serial_atom::f32:
    return "f32";
  case serial_atom::f64:
    return "f64";
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