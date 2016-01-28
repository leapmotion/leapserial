// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ProtobufType.h"
#include "OArchiveImpl.h"
#include "field_serializer.h"
#include "Descriptor.h"
#include "Utility.hpp"
#include <iostream>

using namespace leap;

OArchiveImpl::OArchiveImpl(IOutputStream& os) :
  OArchiveRegistry(os)
{
  objMap[nullptr] = 0;
}

OArchiveImpl::~OArchiveImpl(void) {
  if (pfnDtor)
    pfnDtor(pOsMem);
}

void OArchiveImpl::WriteSize(uint32_t sz) {
  os.Write(&sz, sizeof(uint32_t));
}

uint32_t OArchiveImpl::RegisterObject(const field_serializer& serializer, const void*pObj) {
  auto q = objMap.find(pObj);
  if (q == objMap.end()) {
    // Obtain a new identifier
    ++lastID;

    // Need to register this object for serialization
    deferred.push(work(lastID, &serializer, pObj));

    // Now set the ID that we will be returning to the user and tracking later
    q = objMap.emplace(pObj, lastID).first;
  }
  return q->second;
}

void OArchiveImpl::WriteObject(const field_serializer& serializer, const void* pObj) {
  RegisterObject(serializer, pObj);
  Process(); //Write the newly registered object immediately
}

void OArchiveImpl::WriteObjectReference(const field_serializer& serializer, const void* pObj) {
  auto objId = RegisterObject(serializer, pObj);
  WriteSize(objId); //Write the object ID for later reference.
}

void OArchiveImpl::WriteDescriptor(const descriptor& descriptor, const void* pObj) {
  // Stationary descriptors first:
  for (const auto& field_descriptor : descriptor.field_descriptors)
    field_descriptor.serializer.serialize(
      *this,
      static_cast<const char*>(pObj)+field_descriptor.offset
    );

  // Then variable descriptors:
  for (const auto& cur : descriptor.identified_descriptors) {
    const auto& identified_descriptor = cur.second;
    const void* pChildObj = static_cast<const char*>(pObj)+identified_descriptor.offset;

    // Has identifier, need to write out the ID with the type and then the payload
    auto type = Protobuf::GetSerialType(identified_descriptor.serializer.type());
    WriteInteger(
      (identified_descriptor.identifier << 3) | static_cast<int>(type),
      sizeof(int)
    );

    // Decide whether this is a counted sequence or not:
    switch (type) {
    case Protobuf::serial_type::string:
      // Counted string, write the size first
      WriteInteger((int64_t)identified_descriptor.serializer.size(*this, pChildObj), sizeof(uint64_t));
      break;
    default:
      // Nothing else requires that the size be written
      break;
    }

    // Now handoff to serialization proper
    identified_descriptor.serializer.serialize(*this, pChildObj);
  }
}

uint64_t OArchiveImpl::SizeDescriptor(const descriptor& descriptor, const void* pObj) const {
  uint64_t retVal = 0;
  for (const auto& field_descriptor : descriptor.field_descriptors)
    retVal += field_descriptor.serializer.size(
      *this,
      static_cast<const char*>(pObj)+field_descriptor.offset
    );

  for (const auto& cur : descriptor.identified_descriptors) {
    const auto& identified_descriptor = cur.second;

    // Need the type of the child object and its size proper
    uint64_t ncbChild =
      identified_descriptor.serializer.size(
        *this,
        static_cast<const char*>(pObj) + identified_descriptor.offset
      );

    // Add the size required to encode type information and identity information to the
    // size proper of the child object
    const auto type = Protobuf::GetSerialType(identified_descriptor.serializer.type());
    retVal +=
      leap::serial_traits<uint32_t>::size(
        *this,
        (identified_descriptor.identifier << 3) | static_cast<int>(type)
      ) +
      ncbChild;

    if (type == Protobuf::serial_type::string)
      // Need to know the size-of-the-size
      retVal += leap::serial_traits<uint64_t>::size(*this, ncbChild);
  }
  return retVal;
}

void OArchiveImpl::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  if(writeSize)
    WriteSize((uint32_t)ncb);

  os.Write(pBuf, ncb);
}

void OArchiveImpl::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  WriteSize((uint32_t)charCount);
  WriteByteArray(pBuf, charCount*charSize);
}

void OArchiveImpl::WriteBool(bool value) {
  WriteInteger(value, sizeof(bool));
}

void OArchiveImpl::WriteInteger(int64_t value, uint8_t) {
  size_t ncb = 0;
  if (value) {
    // Write out our composed varint
    auto buf = ToBase128(value, ncb).data();
    WriteByteArray(buf, ncb);
  }
  else
    // Just write one byte of zero
    WriteByteArray(&ncb, 1);
}

void OArchiveImpl::WriteArray(IArrayReader&& ary) {
  uint32_t n = (uint32_t)ary.size();

  if(ary.immutable_size()) {
    if (n & 0x80000000)
      throw std::runtime_error("Cannot serialize fixed arrays of more than 0x7FFFFFFF bytes");
    WriteSize(n);
    for (uint32_t i = 0; i < n; i++)
      ary.serializer.serialize(*this, ary.get(i));
  }
  else {
    // OR with 0x80000000 to signal mode 2 for array writing
    WriteSize(n | 0x80000000);
    for (uint32_t i = 0; i < n; i++) {
      const void* const pObj = ary.get(i);
      uint64_t ncb = ary.serializer.size(*this, pObj);
      WriteInteger(ncb);
      ary.serializer.serialize(*this, pObj);
    }
  }
}

uint64_t OArchiveImpl::SizeArray(IArrayReader&& ary) const {
  uint64_t sz = sizeof(uint32_t);
  size_t n = ary.size();

  if (ary.immutable_size())
    for (size_t i = 0; i < n; i++)
      sz += ary.serializer.size(*this, ary.get(i));
  else
    for (size_t i = 0; i < n; i++) {
      uint64_t ncb = ary.serializer.size(*this, ary.get(i));
      sz += ncb + SizeInteger(ncb, 8);
    }
  return sz;
}

void OArchiveImpl::WriteDictionary(IDictionaryReader&& dictionary)
{
  uint32_t n = static_cast<uint32_t>(dictionary.size());
  WriteSize((uint32_t)n);

  while (dictionary.next()) {
    dictionary.key_serializer.serialize(*this, dictionary.key());
    dictionary.value_serializer.serialize(*this, dictionary.value());
    if (!n--)
      break;
  }
  if (n)
    throw std::runtime_error("Dictionary size function reported a count inconsistent with the number of entries enumerated");
}

uint64_t OArchiveImpl::SizeDictionary(IDictionaryReader&& dictionary) const
{
  uint64_t retVal = sizeof(uint32_t);
  size_t n = dictionary.size();
  while (dictionary.next()) {
    retVal += dictionary.key_serializer.size(*this, dictionary.key());
    retVal += dictionary.value_serializer.size(*this, dictionary.value());
    if (!n--)
      break;
  }
  if (n)
    throw std::runtime_error("Dictionary size function reported a count inconsistent with the number of entries enumerated");
  return retVal;
}

uint64_t OArchiveImpl::SizeInteger(int64_t value, uint8_t) const {
  return leap::SizeBase128(value);
}

void OArchiveImpl::Process(void) {
  for (; !deferred.empty(); deferred.pop()) {
    work& w = deferred.front();

    // Write the expected size first in a string-type field.  Identifier first, string
    // type, then the length
    WriteInteger(
      (w.id << 3) |
      static_cast<uint32_t>(Protobuf::serial_type::string),
      sizeof(uint32_t)
    );
    WriteInteger(w.serializer->size(*this, w.pObj), sizeof(uint32_t));

    // Now hand off to this type's serialization behavior
    w.serializer->serialize(*this, w.pObj);
  }
}
