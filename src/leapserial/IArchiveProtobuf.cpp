// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "IArchiveProtobuf.h"
#include "Descriptor.h"
#include "field_serializer.h"
#include "ProtobufUtil.hpp"
#include "Utility.hpp"
#include <iostream>

using namespace leap;

IArchiveProtobuf::IArchiveProtobuf(IInputStream& is) :
  is(is)
{}

void IArchiveProtobuf::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  sz.deserialize(*this, pObj, 0);
}

IArchive::ReleasedMemory IArchiveProtobuf::ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) {
  return{nullptr, nullptr};
}

void IArchiveProtobuf::Skip(uint64_t ncb) {
  is.Skip(ncb);
  m_count += ncb;
}

bool IArchiveProtobuf::ReadSingle(const descriptor& descriptor, void* pObj) {
  uint64_t v = ReadInteger(0);
  if (is.IsEof())
    return false;

  protobuf::WireType type = static_cast<protobuf::WireType>(v & 7);
  uint64_t ident = v >> 3;

  auto q = descriptor.identified_descriptors.find(ident);
  if (q == descriptor.identified_descriptors.end())
    // Skip behavior
    switch (type) {
    case protobuf::WireType::Varint:
      ReadInteger(0);
      break;
    case protobuf::WireType::LenDelimit:
      Skip(ReadInteger(0));
      break;
    case protobuf::WireType::DoubleWord:
      Skip(4);
      break;
    case protobuf::WireType::QuadWord:
      Skip(8);
      break;
    case protobuf::WireType::StartGroup:
    case protobuf::WireType::EndGroup:
      throw std::runtime_error("Unexpected protobuf wire type");
    case protobuf::WireType::ObjReference:
      throw std::runtime_error("Cannot serialize object references");
    }
  else
    // Straight handoff to deserialize
    q->second.serializer.deserialize(
      *this,
      reinterpret_cast<uint8_t*>(pObj) + q->second.offset,
      0
    );
  return true;
}

void IArchiveProtobuf::ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) {
  if (!descriptor.field_descriptors.empty())
    throw leap::protobuf::serialization_error{ descriptor };

  if (m_pCurDesc)
    // We are not the root type.  The root type is not length-delimited, but all embedded messages
    // will be.  So, we read out the length here in this case.
    ncb = ReadInteger(8);

  leap::internal::Pusher<decltype(m_pCurDesc)> r(m_pCurDesc);
  m_pCurDesc = &descriptor;

  if(ncb)
  {
    uint64_t maxCount = m_count + ncb;
    while (m_count < maxCount)
      if(!ReadSingle(descriptor, pObj))
        throw std::runtime_error("Premature end of input stream");
  }
  else
    while (ReadSingle(descriptor, pObj));
}

void IArchiveProtobuf::ReadByteArray(void* pBuf, uint64_t ncb) {
}

void IArchiveProtobuf::ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) {
  uint64_t n = ReadInteger(0);
  void* pBuf = getBufferFn(n);
  is.Read(pBuf, n);
  m_count += n;
}

bool IArchiveProtobuf::ReadBool(void) {
  return !!ReadInteger(0);
}

uint64_t IArchiveProtobuf::ReadInteger(uint8_t) {
  size_t ncb = 0;
  uint8_t buf[10];
  do if(is.Read(buf, 1) < 0)
    return ~0;
  while (buf[ncb++] & 0x80);
  m_count += ncb;
  return leap::FromBase128(buf, ncb);
}

void IArchiveProtobuf::ReadArray(IArrayAppender&& ary) {
  // Protobuf array deserialization is funny, it's just a bunch of single entries repeated
  // over and over again.
  void* pEntry = ary.allocate();
  ary.serializer.deserialize(*this, pEntry, 0);
}

void IArchiveProtobuf::ReadDictionary(IDictionaryInserter&& dictionary)
{
  // Read out length field first
  uint64_t ncb = ReadInteger(8);
  uint64_t maxCount = m_count + ncb;

  // Key and value
  uint64_t keyIdent = ReadInteger(8) >> 3;
  if (keyIdent != 1)
    throw std::runtime_error("Key provided more than once for a map entry");
  dictionary.key_serializer.deserialize(*this, dictionary.key(), maxCount - m_count);

  uint64_t valueIdent = ReadInteger(8) >> 3;
  if (valueIdent != 2)
    throw std::runtime_error("Expected the key to be provided first, then the value");
  dictionary.value_serializer.deserialize(*this, dictionary.insert(), maxCount - m_count);
  if (m_count != maxCount)
    throw std::runtime_error("Stray bytes encountered after deserializing an object");
}

void* IArchiveProtobuf::ReadObjectReference(const create_delete& cd, const field_serializer& desc) {
  return nullptr;
}