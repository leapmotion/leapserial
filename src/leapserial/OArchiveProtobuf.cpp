// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "OArchiveProtobuf.h"
#include "Descriptor.h"
#include "field_serializer.h"
#include "ProtobufUtil.hpp"
#include "Utility.hpp"
#include <iostream>
#include <sstream>

using namespace leap;
using leap::internal::protobuf::serialization_error;
using leap::internal::protobuf::WireType;
using leap::internal::protobuf::ToWireType;

OArchiveProtobuf::OArchiveProtobuf(IOutputStream& os):
  os(os)
{}

void OArchiveProtobuf::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {

}

void OArchiveProtobuf::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  os.Write(pBuf, charCount);
}

void OArchiveProtobuf::WriteInteger(int64_t value, uint8_t) {
  size_t ncb = 0;
  auto varint = leap::ToBase128(value, ncb);

  if (ncb)
    // Write out our composed varint
    os.Write(varint.data(), ncb);
  else
    // Just write one byte of zero
    os.Write(&ncb, 1);
}

void OArchiveProtobuf::WriteFloat(float value) {
  os.Write(&value, sizeof(value));
}

void OArchiveProtobuf::WriteFloat(double value) {
  os.Write(&value, sizeof(value));
}

void OArchiveProtobuf::WriteObjectReference(const field_serializer& serializer, const void* pObj) {

}

void OArchiveProtobuf::WriteObject(const field_serializer& serializer, const void* pObj) {
  // Root object, no identifier.  We just pass control to the serializer.
  try {
    serializer.serialize(*this, pObj);
  }
  catch (serialization_error& ex) {
    std::ostringstream ss;
    ss << "While processing the root object:" << std::endl
       << ex.what();
    throw leap::internal::protobuf::serialization_error{ ss.str() };
  }
}

void OArchiveProtobuf::WriteDescriptor(const descriptor& descriptor, const void* pObj) {
  if (!descriptor.field_descriptors.empty())
    throw leap::internal::protobuf::serialization_error{ descriptor };

  // Now we write out the identified fields in order.
  leap::internal::Pusher<decltype(curDescEntry)> p(curDescEntry);
  for (const auto& identified_descriptor : descriptor.identified_descriptors) {
    const auto& member_field = identified_descriptor.second;
    const void* pMember = reinterpret_cast<const uint8_t*>(pObj) + member_field.offset;

    // Header with the identifier and wire type.  For some serializers, we are responsible for writing out
    // the wire type; for other serializers, the wire type must be written out by the serializer itself.
    switch(member_field.serializer.type()) {
    case serial_atom::boolean:
    case serial_atom::i8:
    case serial_atom::ui8:
    case serial_atom::i16:
    case serial_atom::ui16:
    case serial_atom::i32:
    case serial_atom::ui32:
    case serial_atom::i64:
    case serial_atom::ui64:
    case serial_atom::f32:
    case serial_atom::f64:
      // These types are all context-free, we are responsible for writing the identifier here, and the
      // type itself will handle things from there.
      WriteInteger((member_field.identifier << 3) | (size_t)ToWireType(member_field.serializer.type()), 8);
      break;
    case serial_atom::string:
    case serial_atom::descriptor:
    case serial_atom::finalized_descriptor:
      // Counted strings, we need to write out the header and then the length verbatim
      WriteInteger((member_field.identifier << 3) | (size_t)WireType::LenDelimit, 8);
      WriteInteger(member_field.serializer.size(*this, pMember), 8);
      break;
    case serial_atom::reference:
      break;
    case serial_atom::array:
      // Array type is very inefficient.  It stamps out the field name and type for each entry in the array.
    case serial_atom::map:
      // Map type is implemented basically the same way as array, except entries are pairs
      curDescEntry = &identified_descriptor;
      break;
    case serial_atom::ignored:
      throw std::runtime_error("Invalid serialization atom type returned");
    }

    // Now we write the payload proper
    member_field.serializer.serialize(*this, pMember);
  }
}

void OArchiveProtobuf::WriteArray(IArrayReader&& ary) {
  WireType wireType = ToWireType(ary.serializer.type());
  uint64_t key = (curDescEntry->first << 3) | (size_t)wireType;
  size_t n = ary.size();
  for (size_t i = 0; i < n; i++) {
    WriteInteger(key, 8);

    const void* pObj = ary.get(i);
    if (wireType == WireType::LenDelimit)
      WriteInteger(ary.serializer.size(*this, pObj), 8);
    ary.serializer.serialize(*this, pObj);
  }
}

void OArchiveProtobuf::WriteDictionary(IDictionaryReader&& dictionary) {
  // We are responsible for writing out our header constraints just as OArchiveProtobuf is, except we know that
  // the wire type is length-delimited
  uint64_t header = (curDescEntry->first << 3) | (size_t)WireType::LenDelimit;

  // Key always has an ID of 1, as per spec
  WireType keyType = ToWireType(dictionary.key_serializer.type());
  uint64_t keyID = (1 << 3) | static_cast<uint32_t>(keyType);

  // Value always has an ID of 2, as per spec
  WireType valueType = ToWireType(dictionary.value_serializer.type());
  uint64_t valueID = (2 << 3) | static_cast<uint32_t>(valueType);

  // Invariant computation
  uint64_t keyvalSize = leap::SizeBase128(keyID) + leap::SizeBase128(valueID);

  while (dictionary.next()) {
    uint64_t keySize = dictionary.key_serializer.size(*this, dictionary.key());
    uint64_t valSize = dictionary.value_serializer.size(*this, dictionary.value());

    // Need the object header, which will be a LenDelimit struct, and the size in advance
    WriteInteger(header, 8);
    WriteInteger(
      keyvalSize +
      keySize + (keyType == WireType::LenDelimit ? leap::SizeBase128(keySize) : 0) +
      valSize + (valueType == WireType::LenDelimit ? leap::SizeBase128(valSize) : 0),
      8
    );
    
    WriteInteger(keyID, 8);
    if (keyType == WireType::LenDelimit)
      WriteInteger(dictionary.key_serializer.size(*this, dictionary.key()), 8);
    dictionary.key_serializer.serialize(*this, dictionary.key());
    WriteInteger(valueID, 8);
    if (valueType == WireType::LenDelimit)
      WriteInteger(dictionary.value_serializer.size(*this, dictionary.value()), 8);
    dictionary.value_serializer.serialize(*this, dictionary.value());
  }
}

uint64_t OArchiveProtobuf::SizeInteger(int64_t value, uint8_t) const {
  return leap::SizeBase128(value);
}

uint64_t OArchiveProtobuf::SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const {
  return ncb;
}

uint64_t OArchiveProtobuf::SizeObjectReference(const field_serializer& serializer, const void* pObj) const {
  return 0;
}

uint64_t OArchiveProtobuf::SizeDescriptor(const descriptor& descriptor, const void* pObj) const {
  leap::internal::Pusher<decltype(curDescEntry)> p(curDescEntry);
  
  // Context-free.  We just write out the identified fields in order.
  uint64_t retVal = 0;
  for (const auto& identified_descriptor : descriptor.identified_descriptors) {
    const auto& member_field = identified_descriptor.second;

    switch(member_field.serializer.type()) {
    case serial_atom::boolean:
    case serial_atom::i8:
    case serial_atom::ui8:
    case serial_atom::i16:
    case serial_atom::ui16:
    case serial_atom::i32:
    case serial_atom::ui32:
    case serial_atom::i64:
    case serial_atom::ui64:
    case serial_atom::f32:
    case serial_atom::f64:
    case serial_atom::string:
    case serial_atom::descriptor:
    case serial_atom::finalized_descriptor:
      // Only need to record the header size:
      retVal += leap::SizeBase128((member_field.identifier << 3) | (size_t)ToWireType(member_field.serializer.type()));
      break;
    case serial_atom::reference:
      break;
    case serial_atom::array:
    case serial_atom::map:
      curDescEntry = &identified_descriptor;
      break;
    case serial_atom::ignored:
      throw std::runtime_error("Invalid serialization atom type returned");
    }
    
    uint64_t ncb = member_field.serializer.size(
      *this,
      reinterpret_cast<const uint8_t*>(pObj) + member_field.offset
    );

    switch (member_field.serializer.type()) {
    case serial_atom::string:
    case serial_atom::descriptor:
    case serial_atom::finalized_descriptor:
      // These are all counted fields.  GetDescriptor is responsible for writing out the length
      // in advance of these calls, and so we must account for this length here.
      retVal += leap::SizeBase128(ncb);
      break;
    default:
      // Do nothing
      break;
    }
    retVal += ncb;
  }
  // Also need to include the number of bytes that will be needed to represent the size itself
  return retVal;
}

uint64_t OArchiveProtobuf::SizeArray(IArrayReader&& ary) const {
  uint64_t keySize = leap::SizeBase128(
    (curDescEntry->first << 3) | (size_t)ToWireType(ary.serializer.type())
  );
  size_t n = ary.size();
  uint64_t retVal = keySize * n;
  while (n--)
    retVal += ary.serializer.size(*this, ary.get(n));
  return retVal;
}

uint64_t OArchiveProtobuf::SizeDictionary(IDictionaryReader&& dictionary) const {
  uint64_t keySize = leap::SizeBase128(
    (curDescEntry->first << 3) | (size_t)ToWireType(dictionary.key_serializer.type())
  );
  uint64_t retVal = 0;
  size_t n = dictionary.size();
  while (dictionary.next())
    retVal += keySize + dictionary.value_serializer.size(*this, dictionary.value());
  return retVal;
}
