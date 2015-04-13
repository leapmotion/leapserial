#include "stdafx.h"
#include "ArchiveFlatbuffer.h"
#include "field_serializer.h"
#include "Descriptor.h"

#include <iostream>
#include <cstring>

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

//Returns the size of the element as stored in a Table or VTable
uint8_t GetFieldSize(leap::serial_primitive p) {
  switch (p) {
  case leap::serial_primitive::boolean:
  case leap::serial_primitive::i8:
    return 1;
  case leap::serial_primitive::i16:
    return 2;
  case leap::serial_primitive::i32:
    return 4;
  case leap::serial_primitive::i64:
    return 8;
  case leap::serial_primitive::array:
  case leap::serial_primitive::string:
  case leap::serial_primitive::map:
    return 4; //stored as 32 bit offsets
  default:
    throw std::runtime_error("Unsupported type detected");
  }
}

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
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  const auto tableOffset = GetValue<uint32_t>(0);
  

  m_offset = tableOffset;
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

  const auto tableOffset = m_offset;
  const auto vTableOffset = tableOffset - GetValue<int32_t>(m_offset);
  //const auto vTableSize = GetValue<uint16_t>(vTableOffset);
  //const auto objectSize = GetValue<uint16_t>(vTableOffset + sizeof(uint16_t));

  uint8_t vTableEntry = 0;

  for (const auto& field_descriptor : orderedDescriptors) {
    const auto fieldOffset = GetValue<uint16_t>(vTableOffset + 4 + (vTableEntry * sizeof(uint16_t)));
    m_offset = tableOffset + fieldOffset;

    field_descriptor->serializer.deserialize(*this,
      static_cast<char*>(pObj)+field_descriptor->offset,
      0
    );

    vTableEntry++;
  }
}

void IArchiveFlatbuffer::ReadByteArray(void* pBuf, uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) {
  const auto baseOffset = m_offset;
  const auto stringOffset = baseOffset + GetValue<uint32_t>(baseOffset);
  
  const auto size = GetValue<uint32_t>(stringOffset);

  auto* dstString = getBufferFn(size);

  memcpy(dstString, &m_data[stringOffset + sizeof(uint32_t)], size);
}

bool IArchiveFlatbuffer::ReadBool() { 
  return !!GetValue<uint8_t>(m_offset);
}

uint64_t IArchiveFlatbuffer::ReadInteger(uint8_t ncb) {
  switch (ncb) {
  case 1:
    return GetValue<uint8_t>(m_offset);
  case 2:
    return GetValue<uint16_t>(m_offset);
  case 4:
    return GetValue<uint32_t>(m_offset);
  case 8:
    return GetValue<uint64_t>(m_offset);
  default:
    throw std::runtime_error("Cannot read non power of 2 sized integer");
  }
}

void IArchiveFlatbuffer::ReadFloat(float& value) { 
  value = GetValue<float>(m_offset);
}

void IArchiveFlatbuffer::ReadFloat(double& value) {
  value = GetValue<double>(m_offset);
}

void IArchiveFlatbuffer::ReadArray(std::function<void(uint64_t)> sizeBufferFn, const field_serializer& t_serializer, std::function<void*()> enumerator, uint64_t expectedEntries) {
  const auto baseOffset = m_offset;
  const auto arrayOffset = baseOffset + GetValue<uint32_t>(baseOffset);

  const auto size = GetValue<uint32_t>(arrayOffset);

  sizeBufferFn(size);

  const auto step = GetFieldSize(t_serializer.type());
  for (uint32_t i = 0 ; i < size; ++i) {
    m_offset = arrayOffset + sizeof(uint32_t) + (i * step);
    t_serializer.deserialize(*this, enumerator(), 0);
  }
}

void IArchiveFlatbuffer::ReadDictionary(const field_serializer& keyDesc, void* key, const field_serializer& valueDesc, void* value, std::function<void(const void* key, const void* value)> inserter) {
  throw not_implemented_exception();
}
