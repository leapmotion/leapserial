// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ArchiveLeapSerial.h"
#include "Allocation.h"
#include "ProtobufType.h"
#include "field_serializer.h"
#include "Descriptor.h"
#include "Utility.hpp"
#include <iostream>
#include <sstream>

using namespace leap;

IArchiveLeapSerial::IArchiveLeapSerial(IInputStream& is) :
  is(is)
{
  // This sentry addition means we never have to test objId against zero
  objMap[0] = { nullptr, nullptr };
}

IArchiveLeapSerial::IArchiveLeapSerial(std::istream& is) :
  is(*new InputStreamAdapter{ is }),
  pIsMem(&this->is),
  pfnDtor([](void* ptr) {
    delete (InputStreamAdapter*)ptr;
  })
{
  // This sentry addition means we never have to test objId against zero
  objMap[0] = { nullptr, nullptr };
}

IArchiveLeapSerial::~IArchiveLeapSerial(void) {
  if(pfnDtor)
    pfnDtor(pIsMem);
  ClearObjectTable();
}

void IArchiveLeapSerial::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  Process(
    deserialization_task(
      &sz,
      0,
      pObj
    )
  );

  // If objects exist that require transference, then we have an error
  if (!pOwner) {
    if (ClearObjectTable())
      throw std::runtime_error(
        "Attempted to perform an allocator-free deserialization on a stream whose types are not completely responsible for their own cleanup"
      );
  }
  else {
    Transfer(*pOwner);
  }
}

void* IArchiveLeapSerial::ReadObjectReference(const create_delete& cd,  const leap::field_serializer &sz) {
    // We expect to find an ID in the input stream
  uint32_t objId;
  ReadByteArray(&objId, sizeof(objId));

  // Now we just perform a lookup into our archive and store the result here
  return Lookup(cd, sz, objId);
}

IArchive::ReleasedMemory IArchiveLeapSerial::ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) {
  // Object ID, then directed registration:
  uint32_t objId;
  ReadByteArray(&objId, sizeof(objId));

  // Verify no unique pointer aliases:
  if (isUnique && IsReleased(objId))
    throw std::runtime_error("Attempted to map the same object into two distinct unique pointers");

  return Release(pfnAlloc, sz, objId);
}

void* IArchiveLeapSerial::Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) {
  auto q = objMap.find(objId);
  if (q != objMap.end())
    return q->second.pObject;

  // Not yet initialized, allocate and queue up
  auto& entry = objMap[objId];
  entry.pObject = cd.pfnAlloc();
  entry.pfnFree = cd.pfnFree;
  work.push(deserialization_task(&serializer, objId, entry.pObject));
  return entry.pObject;
}

void IArchiveLeapSerial::ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) {
  uint64_t countLimit = Count() + ncb;
  for (const auto& field_descriptor : descriptor.field_descriptors)
    field_descriptor.serializer.deserialize(
      *this,
      static_cast<char*>(pObj)+field_descriptor.offset,
      0
    );

  if (!ncb)
    // Impossible for there to be more fields, we don't have a sizer
    return;

  // Identified fields, read them in
  while (Count() < countLimit) {
    // Ident/type field first
    uint64_t ident = ReadInteger(sizeof(uint64_t));
    uint64_t ncbChild;
    switch ((Protobuf::serial_type)(ident & 7)) {
    case Protobuf::serial_type::string:
      ncbChild = ReadInteger(sizeof(uint64_t));
      break;
    case Protobuf::serial_type::b64:
      ncbChild = 8;
      break;
    case Protobuf::serial_type::b32:
      ncbChild = 4;
      break;
    case Protobuf::serial_type::varint:
      ncbChild = 0;
      break;
    default:
      throw std::runtime_error("Unexpected type field encountered");
    }

    // See if we can find the descriptor for this field:
    auto q = descriptor.identified_descriptors.find(ident >> 3);
    if (q == descriptor.identified_descriptors.end())
      // Unrecognized field, need to skip
      if (static_cast<Protobuf::serial_type>(ident & 7) == Protobuf::serial_type::varint)
        // Just read a varint in that we discard right away
        ReadInteger(sizeof(uint64_t));
      else
        // Skip the requisite number of bytes
        Skip(static_cast<size_t>(ncbChild));
    else
      // Hand off to child class
      q->second.serializer.deserialize(
        *this,
        static_cast<char*>(pObj)+q->second.offset,
        static_cast<size_t>(ncbChild)
      );
  }

  if (Count() > countLimit) {
    std::ostringstream os;
    os << "Deserialization error, read " << (Count() - countLimit + ncb) << " bytes and expected to read " << ncb << " bytes";
    throw std::runtime_error(os.str());
  }
}

void IArchiveLeapSerial::ReadArray(IArrayAppender&& ary) {
  // Read the number of entries first:
  uint32_t nEntries;
  ReadByteArray(&nEntries, sizeof(nEntries));
  uint32_t n = nEntries & 0x7FFFFFFF;
  ary.reserve(n);

  if(nEntries & 0x80000000) {
    // Counted-size fields
    for (size_t i = n; i--;)
      ary.serializer.deserialize(
        *this,
        ary.allocate(),
        ReadInteger(8)
      );
  }
  else {
    // Fixed-size fields, just read everything in
    for (size_t i = n; i--;)
      ary.serializer.deserialize(*this, ary.allocate(), 0);
  }
}

void IArchiveLeapSerial::ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) {
  // Read the number of entries first:
  uint32_t nEntries;
  ReadByteArray(&nEntries, sizeof(nEntries));

  auto* pBuf = getBufferFn(nEntries);

  ReadByteArray(pBuf, nEntries * charSize);
}

void IArchiveLeapSerial::ReadDictionary(IDictionaryInserter&& dictionary)
{
  // Read the number of entries first:
  uint32_t nEntries;
  ReadByteArray(&nEntries, sizeof(nEntries));

  // Now read in all values:
  while (nEntries--) {
    dictionary.key_serializer.deserialize(*this, dictionary.key(), 0);
    void* value = dictionary.insert();
    dictionary.value_serializer.deserialize(*this, value, 0);
  }
}

IArchive::ReleasedMemory IArchiveLeapSerial::Release(ReleasedMemory(*pfnAlloc)(), const field_serializer& serializer, uint32_t objId) {
  auto q = objMap.find(objId);
  if (q != objMap.end()) {
    // Object already allocated, we just need to remove control back to ourselves
    q->second.pfnFree = nullptr;
    return {q->second.pObject, q->second.pContext};
  }

  // Not yet initialized, allocate and queue up
  auto& entry = objMap[objId];
  IArchive::ReleasedMemory retVal = pfnAlloc();
  entry.pObject = retVal.pObject;
  entry.pContext = retVal.pContext;
  entry.pfnFree = nullptr;
  work.push(deserialization_task(&serializer, objId, entry.pObject));
  return retVal;
}

bool IArchiveLeapSerial::IsReleased(uint32_t objId) {
  auto q = objMap.find(objId);
  return q != objMap.end() && !q->second.pfnFree;
}

void IArchiveLeapSerial::ReadByteArray(void* pBuf, uint64_t ncb) {
  std::streamsize nRead = is.Read(pBuf, ncb);
  if(nRead != ncb)
    throw std::runtime_error("End of file reached prematurely");
  m_count += ncb;
}

void IArchiveLeapSerial::Skip(uint64_t ncb) {
  is.Skip(ncb);
}

void IArchiveLeapSerial::Transfer(internal::AllocationBase& alloc) {
  for (auto& cur : objMap)
    if (cur.second.pfnFree)
      // Transfer cleanup responsibility to the allocator
      alloc.garbageList.push_back(
        std::make_pair(
          cur.second.pObject,
          cur.second.pfnFree
        )
      );

  objMap.clear();
}

bool IArchiveLeapSerial::ReadBool() {
  bool b;
  ReadByteArray(&b, 1);
  return b;
}

uint64_t IArchiveLeapSerial::ReadInteger(uint8_t) {
  size_t ncb = 0;
  uint8_t buf[10];
  do ReadByteArray(&buf[ncb], 1);
  while (buf[ncb++] & 0x80);
  return leap::FromBase128(buf, ncb);
}

size_t IArchiveLeapSerial::ClearObjectTable(void) {
  size_t n = 0;
  for (auto& cur : objMap)
    if (cur.second.pfnFree) {
      // One more entry freed
      n++;

      // Transfer cleanup responsibility to the allocator
      cur.second.pfnFree(cur.second.pObject);
    }

  objMap.clear();
  return n;
}

void IArchiveLeapSerial::Process(const deserialization_task& task) {
  objMap[1] = {task.pObject, nullptr};
  work.push(task);

  // Continue to work as long as there is work to be done
  for(; !work.empty(); work.pop()) {
    deserialization_task& task = work.front();

    // Identifier/type comes first
    auto id_type = ReadInteger(8);

    // Then we need the size (if it's available)
    uint64_t ncb = 0;
    switch (static_cast<Protobuf::serial_type>(id_type & 7)) {
    case Protobuf::serial_type::b32:
      ncb = 4;
      break;
    case Protobuf::serial_type::b64:
      ncb = 8;
      break;
    case Protobuf::serial_type::string:
      // Size fits right here
      ncb = ReadInteger(sizeof(ncb));
      break;
    case Protobuf::serial_type::varint:
    case Protobuf::serial_type::ignored:
      break;
    }

    task.serializer->deserialize(*this, task.pObject, ncb);
  }
}


OArchiveLeapSerial::OArchiveLeapSerial(IOutputStream& os) :
  OArchiveRegistry(os)
{
  objMap[nullptr] = 0;
}

OArchiveLeapSerial::~OArchiveLeapSerial(void) {
  if (pfnDtor)
    pfnDtor(pOsMem);
}

void OArchiveLeapSerial::WriteSize(uint32_t sz) {
  os.Write(&sz, sizeof(uint32_t));
}

uint32_t OArchiveLeapSerial::RegisterObject(const field_serializer& serializer, const void*pObj) {
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

void OArchiveLeapSerial::WriteObject(const field_serializer& serializer, const void* pObj) {
  RegisterObject(serializer, pObj);
  Process(); //Write the newly registered object immediately
}

void OArchiveLeapSerial::WriteObjectReference(const field_serializer& serializer, const void* pObj) {
  auto objId = RegisterObject(serializer, pObj);
  WriteSize(objId); //Write the object ID for later reference.
}

void OArchiveLeapSerial::WriteDescriptor(const descriptor& descriptor, const void* pObj) {
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

uint64_t OArchiveLeapSerial::SizeDescriptor(const descriptor& descriptor, const void* pObj) const {
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

void OArchiveLeapSerial::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  if(writeSize)
    WriteSize((uint32_t)ncb);

  os.Write(pBuf, ncb);
}

void OArchiveLeapSerial::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  WriteSize((uint32_t)charCount);
  WriteByteArray(pBuf, charCount*charSize);
}

void OArchiveLeapSerial::WriteBool(bool value) {
  WriteInteger(value, sizeof(bool));
}

void OArchiveLeapSerial::WriteInteger(int64_t value, uint8_t) {
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

void OArchiveLeapSerial::WriteArray(IArrayReader&& ary) {
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

uint64_t OArchiveLeapSerial::SizeArray(IArrayReader&& ary) const {
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

void OArchiveLeapSerial::WriteDictionary(IDictionaryReader&& dictionary)
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

uint64_t OArchiveLeapSerial::SizeDictionary(IDictionaryReader&& dictionary) const
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

uint64_t OArchiveLeapSerial::SizeInteger(int64_t value, uint8_t) const {
  return leap::SizeBase128(value);
}

void OArchiveLeapSerial::Process(void) {
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
