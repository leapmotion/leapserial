#include "stdafx.h"
#include "ArchiveFlatbuffer.h"
#include "field_serializer.h"
#include "Descriptor.h"

#include <iostream>

using namespace leap;

/// Flatbuffers importaint format information taken from 
//// https://google.github.io/flatbuffers/md__internals.html
///
/// offsets are always written as 32 bit integers
/// 
/// Structs are all aligned to their size and are written in order.
///
/// Tables start with soffset_t (a int32_t) to the vtable for the object,
/// followed by all fields as aligned scalars. not all fields need to be present.
/// 
/// Vtables are constructed of voffset_ts(a uint16_t).
/// 
/// Strings are vectors of bytes, always null terminated
/// vectors are stored as contiguous alingned scalars with a uint32_t element count.
/// The standard implementation writes buffers backwards as this reduces the amount
/// of bookkeeping overhead.


/// Basic buffer structure:
/// uint32_t offset to root object in the buffer
/// //Begin a vtable
/// uint16_t size of vtable in bytes (including this)
/// uint16_t size of object in bytes (including vtable offset)
/// for N elements in the vtable
/// uint16_t offset of Nth field
/// 

struct not_implemented_exception :
  public std::runtime_error
{
public:
  not_implemented_exception() : std::runtime_error("This function is not yet implemented!") {}
};

IArchiveFlatbuffer::IArchiveFlatbuffer(std::istream& is) {
  is.clear();
  is.seekg(0, std::ios::end);
  const auto sz = is.tellg();
  m_data.resize((unsigned int)sz);
  is.seekg(0, std::ios::beg);
  is.read((char*)&m_data[0], m_data.size());
}

IArchiveFlatbuffer::~IArchiveFlatbuffer() {

}

void IArchiveFlatbuffer::Skip(uint64_t ncb) {
  m_count += ncb;
}

void IArchiveFlatbuffer::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  m_currentTableOffset.push(GetValue<uint32_t>(0));
  m_currentVTableOffset.push(m_currentTableOffset.top() - GetValue<int32_t>(m_currentTableOffset.top()));
  m_currentVTableSize.push(GetValue<uint16_t>(m_currentTableOffset.top()));
  m_currentObjectSize.push(GetValue<uint16_t>(m_currentTableOffset.top() + sizeof(uint16_t)));

  sz.deserialize(*this, pObj, 0);
}

IArchive::ReleasedMemory IArchiveFlatbuffer::ReadObjectReferenceResponsible(IArchive::ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) { 
  throw not_implemented_exception();
}

void* IArchiveFlatbuffer::ReadObjectReference(const create_delete& cd, const field_serializer& desc) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) {
  std::vector<const field_descriptor*> orderedDescriptors;

  for (const auto& field_descriptor : descriptor.field_descriptors)
    orderedDescriptors.push_back(&field_descriptor);

  for (uint32_t i = 1; i <= descriptor.identified_descriptors.size(); i++) {
    auto found = descriptor.identified_descriptors.find(i);
    if (found == descriptor.identified_descriptors.end())
      throw std::runtime_error("Missing an identifier, behavior unhandled by IArchiveFlatbuffer");
    orderedDescriptors.push_back(&(found->second));
  }

  for (const auto& field_descriptor : orderedDescriptors) {
    field_descriptor->serializer.deserialize(*this,
      static_cast<char*>(pObj)+field_descriptor->offset,
      0
    );
    m_currentVTableEntry++;
  }

}

void IArchiveFlatbuffer::ReadByteArray(void* pBuf, uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadString(void* pBuf, uint64_t charCount, uint8_t charSize) {
  throw not_implemented_exception();
}

bool IArchiveFlatbuffer::ReadBool() { 
  return !!GetCurrentTableValue<uint8_t>();
}

uint64_t IArchiveFlatbuffer::ReadInteger(uint8_t ncb) {
  switch (ncb) {
  case 1:
    return GetCurrentTableValue<uint8_t>();
  case 2:
    return GetCurrentTableValue<uint16_t>();
  case 4:
    return GetCurrentTableValue<uint32_t>();
  case 8:
    return GetCurrentTableValue<uint64_t>();
  default:
    throw std::runtime_error("Cannot read non power of 2 sized integer");
  }
}

void IArchiveFlatbuffer::ReadFloat(float& value) { 
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadFloat(double& value) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadArray(const field_serializer& sz, uint64_t n, std::function<void*()> enumerator) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadDictionary(const field_serializer& keyDesc, void* key, const field_serializer& valueDesc, void* value, std::function<void(const void* key, const void* value)> inserter) {
  throw not_implemented_exception();
}
