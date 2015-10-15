// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ProtobufUtil.hpp"
#include "Archive.h"
#include "Descriptor.h"
#include <sstream>

using namespace leap;

protobuf::WireType leap::protobuf::ToWireType(serial_atom atom) {
  switch (atom) {
  case serial_atom::boolean:
  case serial_atom::i8:
  case serial_atom::i16:
  case serial_atom::i32:
  case serial_atom::i64:
    return protobuf::WireType::Varint;
  case serial_atom::f32:
    return protobuf::WireType::DoubleWord;
  case serial_atom::f64:
    return protobuf::WireType::QuadWord;
  case serial_atom::reference:
  case serial_atom::array:
    return protobuf::WireType::LenDelimit;
  case serial_atom::string:
    return protobuf::WireType::LenDelimit;
  case serial_atom::map:
    break;
  case serial_atom::descriptor:
    return protobuf::WireType::LenDelimit;
  case serial_atom::finalized_descriptor:
    break;
  }
  throw std::invalid_argument("Attempted to find a wire type for an unrecognized serial atom type");
}

static std::string FormatError(const descriptor& descriptor) {
  std::stringstream ss;
  ss << "The OArchiveProtobuf requires that all entries have identifiers" << std::endl
     << "Fields at the following offsets do not have identifiers:" << std::endl;
  for (const auto& field_descriptor : descriptor.field_descriptors)
    ss << "[ " << std::left << std::setw(8) << ToString(field_descriptor.serializer.type()) << " ] @+" << field_descriptor.offset << std::endl;
  return ss.str();
}

protobuf::serialization_error::serialization_error(const descriptor& descriptor) :
  std::runtime_error(FormatError(descriptor))
{}
