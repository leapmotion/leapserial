// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ArchiveJSON.h"
#include "field_serializer.h"
#include "Descriptor.h"
#include <iostream>

using namespace leap;

struct not_implemented_exception :
  public std::runtime_error
{
public:
  not_implemented_exception() : std::runtime_error("This function is not yet implemented!") {}
};

OArchiveJSON::OArchiveJSON(leap::OutputStreamAdapter& osa, bool escapeSlashes) :
  OArchiveRegistry(osa),
  EscapeSlashes(escapeSlashes),
  os(osa.GetStdStream())
{}

void OArchiveJSON::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteBool(bool value) {
  if (value)
    os << "true";
  else
    os << "false";
}

void OArchiveJSON::WriteInteger(int8_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(uint8_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(int16_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(uint16_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(int32_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(uint32_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(int64_t value) {
  os << value;
}

void OArchiveJSON::WriteInteger(uint64_t value) {
  os << value;
}

void OArchiveJSON::WriteFloat(float value) {
  os << value;
}

void OArchiveJSON::WriteInteger(int64_t value, uint8_t) {
  os << value;
}

void OArchiveJSON::WriteFloat(double value) {
  os << value;
}

void OArchiveJSON::WriteObjectReference(const field_serializer& serializer, const void* pObj) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteObject(const field_serializer& serializer, const void* pObj) {
  serializer.serialize(*this, pObj);
}

void OArchiveJSON::WriteDescriptor(const descriptor& descriptor, const void* pObj) {
  if (PrettyPrint) {
    os << "{\n";
  }
  else
    os << "{";

  currentTabLevel++;

  const uint8_t* pBase = static_cast<const uint8_t*>(pObj);
  for (const auto &field_descriptor : descriptor.field_descriptors) {
    if (PrettyPrint)
      TabOut();
    os << "\"" << field_descriptor.name << "\":";
    if (PrettyPrint)
      os << ' ';
    
    const void* pChildObj = pBase + field_descriptor.offset;
    field_descriptor.serializer.serialize(*this, pChildObj);

    if(PrettyPrint)
      os << ",\n";
    else
      os << ",";
  }
  if (!descriptor.field_descriptors.empty()) {
    os.seekp(PrettyPrint ? -2 : -1, std::ios::cur); //Remove the trailing ","

    // Reintroduce line break if needed
    if (PrettyPrint)
      os << "\n";
  }

  for (const auto& iter : descriptor.identified_descriptors) {
    const auto& field_descriptor = iter.second;
    const void* pChildObj = static_cast<const char*>(pObj)+field_descriptor.offset;
    os << "\"" << field_descriptor.name << "\":";
    if (PrettyPrint)
      os << ' ';
    field_descriptor.serializer.serialize(*this, pChildObj);
    os << (PrettyPrint ? ",\n" : ",");
  }
  if (!descriptor.identified_descriptors.empty()) {
    os.seekp(PrettyPrint ? -2 : -1, std::ios::cur); //Remove the trailing ","

    // Reintroduce line break if needed
    if (PrettyPrint)
      os << "\n";
  }

  currentTabLevel--;

  if (PrettyPrint)
    TabOut();
  os << '}';
}

void OArchiveJSON::WriteArray(IArrayReader&& ary) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteDictionary(IDictionaryReader&& dictionary)
{
  throw not_implemented_exception();
}

uint64_t OArchiveJSON::SizeInteger(int64_t value, uint8_t ncb) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeFloat(float value) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeFloat(double value) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeBool(bool value) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeObjectReference(const field_serializer& serializer, const void* pObj) const {
  throw not_implemented_exception();
}
uint64_t OArchiveJSON::SizeDescriptor(const descriptor& descriptor, const void* pObj) const {
  throw not_implemented_exception();
}

uint64_t OArchiveJSON::SizeArray(IArrayReader&& ary) const {
  throw not_implemented_exception();
}

uint64_t OArchiveJSON::SizeDictionary(IDictionaryReader&& dictionary) const
{
  throw not_implemented_exception();
}

void OArchiveJSON::TabOut(void) const {
  if (TabWidth)
    for (size_t i = TabWidth * (currentTabLevel + TabLevel); i--;)
      os << ' ';
  else
    for (size_t i = currentTabLevel + TabLevel; i--;)
      os << '\t';
}

IArchiveJSON::IArchiveJSON(std::istream& is) {
  
}

void IArchiveJSON::Skip(uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  throw not_implemented_exception();
}

IArchive::ReleasedMemory IArchiveJSON::ReadObjectReferenceResponsible(IArchive::ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) {
  throw not_implemented_exception();
}

void* IArchiveJSON::ReadObjectReference(const create_delete& cd, const field_serializer& desc) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadByteArray(void* pBuf, uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) {
  throw not_implemented_exception();
}

bool IArchiveJSON::ReadBool() {
  throw not_implemented_exception();
}

uint64_t IArchiveJSON::ReadInteger(uint8_t ncb) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadFloat(float& value) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadFloat(double& value) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadArray(IArrayAppender&& ary) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadDictionary(IDictionaryInserter&& dictionary) {
  throw not_implemented_exception();
}
