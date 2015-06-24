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
uint8_t GetFieldSize(leap::serial_atom p) {
  switch (p) {
  case leap::serial_atom::boolean:
  case leap::serial_atom::i8:
    return 1;
  case leap::serial_atom::i16:
    return 2;
  case leap::serial_atom::i32:
    return 4;
  case leap::serial_atom::i64:
    return 8;
  case leap::serial_atom::array:
  case leap::serial_atom::string:
  case leap::serial_atom::descriptor:
    return 4; //stored as 32 bit offsets
  default:
    throw std::runtime_error("Unsupported type detected");
  }
}

bool WillStoreAsOffset(const OArchiveFlatbuffer& arch, const leap::field_serializer& sz, const void* pObj) {
  switch (sz.type()) {
  case leap::serial_atom::array:
  case leap::serial_atom::string:
  case leap::serial_atom::descriptor:
    return true;
  default:
    return false;
  }
}

std::vector<const field_descriptor*> SortFields(const descriptor& descriptor) {
  std::vector<const field_descriptor*> orderedDescriptors;
  for (const auto& field_descriptor : descriptor.field_descriptors)
    orderedDescriptors.push_back(&field_descriptor);

  for (uint32_t i = 1; i <= descriptor.identified_descriptors.size(); i++) {
    auto found = descriptor.identified_descriptors.find(i);
    if (found == descriptor.identified_descriptors.end())
      throw std::runtime_error("Missing an identifier, behavior unhandled by IArchiveFlatbuffer");
    orderedDescriptors.push_back(&(found->second));
  }

  return orderedDescriptors;
}

OArchiveFlatbuffer::OArchiveFlatbuffer(std::ostream& os) : os(os) {}

void OArchiveFlatbuffer::Finish() {
  //And copy the buffer backwards into the stream...
  for (auto dataIter = m_builder.rbegin(); dataIter != m_builder.rend(); dataIter++) {
    os.write((const char*)&*dataIter, 1);
  }
}

void OArchiveFlatbuffer::WriteRelativeOffset(const void* pObj) {
  auto found = m_offsets.find(pObj);
  if (found == m_offsets.end())
    throw std::runtime_error("Attempted to write an offset for an object which doesn't have one!");

  WriteInteger((m_builder.size() + sizeof(uint32_t)) - found->second, 4);
}

//Taken from flatbuffers.h
inline size_t PaddingBytes(size_t buf_size, size_t scalar_size) {
  return ((~buf_size) + 1) & (scalar_size - 1);
}

void OArchiveFlatbuffer::Align(uint8_t alignment) {
  m_largestAligned = std::max(m_largestAligned, alignment);
  const auto padding = PaddingBytes(m_builder.size(), alignment);
  m_builder.resize(m_builder.size() + padding, 0);
}

void OArchiveFlatbuffer::PreAlign(uint32_t len, uint8_t alignment) {
  const auto padding = PaddingBytes(m_builder.size() + len, alignment);
  m_builder.resize(m_builder.size() + padding, 0);
}

void OArchiveFlatbuffer::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  throw not_implemented_exception();
}

void OArchiveFlatbuffer::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) { 
  if (charSize != 1)
    throw std::runtime_error("Flatbuffers does not support non ASCII/UTF-8 strings");

  //Prealign the size field.
  PreAlign((uint32_t)charCount + 1, sizeof(uint32_t));

  //Always null terminated
  m_builder.push_back(0);

  const char* pStr = (const char*)pBuf;
  for (int i = (int)(charCount - 1); i >= 0; --i) {
    m_builder.push_back(pStr[i]);
  }

  WriteInteger((uint32_t)(charCount));
  SaveOffset(m_currentFieldPtr, (uint32_t)m_builder.size());
}

void OArchiveFlatbuffer::WriteBool(bool value) { 
  WriteInteger((uint8_t)value);
}

void OArchiveFlatbuffer::WriteInteger(int64_t value, uint8_t ncb) {
  Align(ncb);
  
  uint64_t bits = *reinterpret_cast<uint64_t*>(&value);
  for (int i = ncb-1; i >= 0; --i) {
    const uint8_t lowerBits = (uint8_t)(bits >> (i * 8) & 0x0000000000000FF);
    m_builder.push_back(lowerBits);
  }
}

void OArchiveFlatbuffer::WriteFloat(float value) { 
  WriteInteger(*reinterpret_cast<uint32_t*>(&value));
}

void OArchiveFlatbuffer::WriteFloat(double value) { 
  WriteInteger(*reinterpret_cast<uint64_t*>(&value));
}

void OArchiveFlatbuffer::WriteObjectReference(const field_serializer& serializer, const void* pObj) { 
  throw not_implemented_exception();
}

void OArchiveFlatbuffer::WriteObject(const field_serializer& serializer, const void* pObj) {
  //We have to have offsets to all children before we can actually write the object offset, so pass on
  //to the descriptor...
  serializer.serialize(*this, pObj);

  //make sure the whole buffer is aligned
  PreAlign(sizeof(uint32_t), m_largestAligned);

  //Now write the root offset of the object
  WriteRelativeOffset(pObj);

  Finish();
}

void OArchiveFlatbuffer::WriteDescriptor(const descriptor& descriptor, const void* pObj) { 

  const auto orderedDescriptors = SortFields(descriptor);

  //Write all the children that will be stored as offsets first
  for (auto field_iter = orderedDescriptors.rbegin(); field_iter != orderedDescriptors.rend(); field_iter++) {
    const auto& field_descriptor = **field_iter;
    const void* pChildObj = static_cast<const char*>(pObj)+field_descriptor.offset;
    if (WillStoreAsOffset(*this, field_descriptor.serializer, pChildObj)) {
      m_currentFieldPtr = pChildObj;
      field_descriptor.serializer.serialize(*this, pChildObj);
    }
  }

  //Now write the table(or struct), in order.  We will also store the inverse offsets of the fields in reverse
  //order.  The vTable will also be saved and queried in this format
  VTable vTable;
  const uint32_t tableEnd = (uint32_t)m_builder.size();
  for (auto field_iter = orderedDescriptors.rbegin(); field_iter != orderedDescriptors.rend(); field_iter++) {
    const auto& field_descriptor = **field_iter;
    const void* pChildObj = static_cast<const char*>(pObj)+field_descriptor.offset;
    if (WillStoreAsOffset(*this, field_descriptor.serializer, pChildObj)) {
      WriteRelativeOffset(pChildObj);
    }
    else {
      field_descriptor.serializer.serialize(*this, pChildObj);
    }
    vTable.offsets.push_back((uint32_t)m_builder.size() - tableEnd);
  }
  
  //If there are no identified descriptors, then we are a struct and do not care about vtable stuff.
  if (descriptor.identified_descriptors.empty())
    return;

  const uint32_t tableStart = (uint32_t)(m_builder.size() + sizeof(int32_t) + PaddingBytes(m_builder.size(), sizeof(int32_t)));
  SaveOffset(pObj, tableStart);

  //Save the offset of the table (we've yet to write the vtable offset, so add the space it'll take)
  vTable.size = (uint16_t)((2 + orderedDescriptors.size()) * sizeof(uint16_t));
  vTable.tableSize = (uint16_t)(tableStart - tableEnd);
  vTable.offset = tableStart + vTable.size;

  auto insertionResult = m_vTables.insert(vTable);
  
  //Final entry for the table
  WriteInteger((int32_t)(insertionResult.first->offset - tableStart));

  //If a matching vtable exists already, we're done
  if (!insertionResult.second)
    return;

  const uint16_t tableBaseOffset = vTable.tableSize;
  for (const uint16_t fieldOffset : vTable.offsets) {
    WriteInteger((uint16_t)(tableBaseOffset - fieldOffset));
  }

  WriteInteger(vTable.tableSize);
  WriteInteger(vTable.size);
}

void OArchiveFlatbuffer::WriteArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) { 
  //We need to write these twice in somce cases, so store the pointers & go back through in reverse
  std::vector<const void*> elements;
  for (uint64_t i = 0; i < n; i++)
    elements.push_back(enumerator());

  const auto arrayPtr = m_currentFieldPtr;

  for (auto i = elements.rbegin(); i != elements.rend(); i++) {
    m_currentFieldPtr = *i;
    desc.serialize(*this, *i);
  }

  //If this is an array of a type that is stored by offset, store the offsets...
  if (WillStoreAsOffset(*this, desc, elements[0])) {
    for (auto i = elements.rbegin(); i != elements.rend(); i++) {
      WriteRelativeOffset(*i);
    }
  }

  WriteInteger((uint32_t)n);
  SaveOffset(arrayPtr, (uint32_t)m_builder.size());
}

void OArchiveFlatbuffer::WriteDictionary(
  uint64_t n,
  const field_serializer& keyDesc,
  std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc,
  std::function<const void*()> valueEnumerator
  )
{ 
  throw not_implemented_exception();
}

uint64_t OArchiveFlatbuffer::SizeInteger(int64_t value, uint8_t ncb) const { 
  return ncb;
}
uint64_t OArchiveFlatbuffer::SizeFloat(float value) const { 
  return sizeof(float);
}
uint64_t OArchiveFlatbuffer::SizeFloat(double value) const { 
  return sizeof(double);
}
uint64_t OArchiveFlatbuffer::SizeBool(bool value) const { 
  return sizeof(bool);
}
uint64_t OArchiveFlatbuffer::SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const { 
  return sizeof(uint32_t);
}
uint64_t OArchiveFlatbuffer::SizeObjectReference(const field_serializer& serializer, const void* pObj) const { 
  return sizeof(uint32_t);
}
uint64_t OArchiveFlatbuffer::SizeDescriptor(const descriptor& descriptor, const void* pObj) const { 
  // If stored as a struct, return the size of the struct
  if (descriptor.identified_descriptors.empty()) {
    uint64_t size = 0;
    for (const auto& field_descriptor : descriptor.field_descriptors)
      size += field_descriptor.serializer.size(*this, pObj);
    return size;
  }
  
  // If stored as a table, return the offset size.
  return sizeof(uint32_t);
}

uint64_t OArchiveFlatbuffer::SizeArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) const { 
  return sizeof(uint32_t);
}

uint64_t OArchiveFlatbuffer::SizeDictionary(
  uint64_t n,
  const field_serializer& keyDesc,
  std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc,
  std::function<const void*()> valueEnumerator
  ) const 
{ 
  throw not_implemented_exception();
}

IArchiveFlatbuffer::IArchiveFlatbuffer(std::istream& is) {
  is.clear();
  is.seekg(0, std::ios::end);
  const auto sz = is.tellg();
  m_data.resize((unsigned int)sz);
  is.seekg(0, std::ios::beg);
  is.read((char*)&m_data[0], m_data.size());
}

void IArchiveFlatbuffer::Skip(uint64_t ncb) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) {
  sz.deserialize(*this, pObj, 0); //The root entry is an offset to a table.
}

IArchive::ReleasedMemory IArchiveFlatbuffer::ReadObjectReferenceResponsible(IArchive::ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) { 
  throw not_implemented_exception();
}

void* IArchiveFlatbuffer::ReadObjectReference(const create_delete& cd, const field_serializer& desc) {
  throw not_implemented_exception();
}

void IArchiveFlatbuffer::ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) {
  //If there are only unidentified (non-id type) descriptors, then we're a struct and don't
  //need to do any vtable lookup or field sorting.
  if (descriptor.identified_descriptors.empty()) {
    for (const auto& field_descriptor : descriptor.field_descriptors) {
      field_descriptor.serializer.deserialize(*this,
        static_cast<char*>(pObj)+field_descriptor.offset, 0);
    }
    return;
  }

  const auto orderedDescriptors = SortFields(descriptor);

  //If this is a table, then it's stored by offset...
  const auto tableOffsetOffset = m_offset;
  const auto tableOffset = m_offset + GetValue<uint32_t>(tableOffsetOffset);
  const auto vTableOffset = tableOffset - GetValue<int32_t>(tableOffset);
  //const auto vTableSize = GetValue<uint16_t>(vTableOffset);
  //const auto tableSize = GetValue<uint16_t>(vTableOffset + sizeof(uint16_t));

  uint8_t vTableEntry = 0;
  for (const auto& field_descriptor : orderedDescriptors) {
    const auto fieldOffset = GetValue<uint16_t>(vTableOffset + 4 + (vTableEntry * sizeof(uint16_t)));
    m_offset = tableOffset + fieldOffset; //can't rely on automatic m_offset iteration since fields may be in any order.

    field_descriptor->serializer.deserialize(*this,
      static_cast<char*>(pObj)+field_descriptor->offset,
      0
    );

    vTableEntry++;
  }

  m_offset = tableOffsetOffset + sizeof(uint32_t);
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

  m_offset = baseOffset + sizeof(uint32_t);
}

bool IArchiveFlatbuffer::ReadBool() { 
  const auto offset = m_offset;
  m_offset += sizeof(uint8_t); 
  return !!GetValue<uint8_t>(offset);
}

uint64_t IArchiveFlatbuffer::ReadInteger(uint8_t ncb) {
  const auto offset = m_offset;
  m_offset += ncb;
  switch (ncb) {
  case 1:
    return GetValue<uint8_t>(offset);
  case 2:
    return GetValue<uint16_t>(offset);
  case 4:
    return GetValue<uint32_t>(offset);
  case 8:
    return GetValue<uint64_t>(offset);
  default:
    throw std::runtime_error("Cannot read non power of 2 sized integer");
  }
}

void IArchiveFlatbuffer::ReadFloat(float& value) {
  value = GetValue<float>(m_offset);
  m_offset += sizeof(float);
}

void IArchiveFlatbuffer::ReadFloat(double& value) {
  value = GetValue<double>(m_offset);
  m_offset += sizeof(float);
}

void IArchiveFlatbuffer::ReadArray(std::function<void(uint64_t)> sizeBufferFn, const field_serializer& t_serializer, std::function<void*()> enumerator, uint64_t expectedEntries) {
  const auto baseOffset = m_offset;
  const auto arrayOffset = baseOffset + GetValue<uint32_t>(baseOffset);

  const auto size = GetValue<uint32_t>(arrayOffset);

  sizeBufferFn(size);

  m_offset = arrayOffset + sizeof(uint32_t);
  //const auto step = GetFieldSize(t_serializer.type());
  for (uint32_t i = 0 ; i < size; ++i) {
    //assert(m_offset == arrayOffset + sizeof(uint32_t) + (i * step));
    t_serializer.deserialize(*this, enumerator(), 0);
  }

  m_offset = baseOffset + sizeof(uint32_t);
}

void IArchiveFlatbuffer::ReadDictionary(const field_serializer& keyDesc, void* key, const field_serializer& valueDesc, void* value, std::function<void(const void* key, const void* value)> inserter) {
  throw not_implemented_exception();
}
