#include "stdafx.h"
#include "OArchiveImpl.h"
#include "field_serializer.h"
#include <iostream>

using namespace leap;

OArchiveImpl::OArchiveImpl(std::ostream& os) :
  os(os),
  lastID(0)
{
  // This sentry addition means we never have to nullptr check pObj
  objMap[nullptr] = 0;
}

OArchiveImpl::~OArchiveImpl(void) {}

void OArchiveImpl::WriteSize(uint32_t sz) {
  os.write((const char*) &sz,sizeof(uint32_t));
}

void OArchiveImpl::WriteObject(const field_serializer& serializer, const void* pObj)
{
  auto q = objMap.find(pObj);
  if (q == objMap.end()) {
    // Obtain a new identifier
    ++lastID;

    // Now set the ID that we will be returning to the user and tracking later
    q = objMap.emplace(pObj, lastID).first;
  }

  // Write the expected size first in a string-type field.  Identifier first, string
  // type, then the length
  WriteInteger(
    (q->second << 3) |
    static_cast<uint32_t>(serial_type::string),
    sizeof(uint32_t)
    );
  WriteInteger(serializer.size(*this, pObj), sizeof(uint32_t));

  // Now hand off to this type's serialization behavior
  serializer.serialize(*this, pObj);
}

void OArchiveImpl::WriteObjectReference(const field_serializer& serializer, const void* pObj)
{
  auto q = objMap.find(pObj);
  if (q == objMap.end()) {
    // Obtain a new identifier
    ++lastID;

    // Need to register this object for serialization
    deferred.push(work(lastID, &serializer, pObj));

    // Now set the ID that we will be returning to the user and tracking later
    q = objMap.emplace(pObj, lastID).first;
  }

  WriteSize(q->second);
}

uint64_t OArchiveImpl::SizeObject(const field_serializer& serializer, const void*pObj) const {
  return serializer.size(*this, pObj);
}

void OArchiveImpl::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  if(writeSize)
    WriteSize((uint32_t)ncb);
  
  os.write((const char*) pBuf, ncb);
}

void OArchiveImpl::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  WriteSize((uint32_t)charCount);
  WriteByteArray(pBuf, charCount*charSize);
}

void OArchiveImpl::WriteBool(bool value) {
  WriteInteger(value, sizeof(bool));
}

void OArchiveImpl::WriteInteger(int64_t value, size_t) {
  uint8_t varint[10];

  // Serialize out
  size_t ncb = 0;
  for (uint64_t i = *reinterpret_cast<uint64_t*>(&value); i; i >>= 7, ncb++)
    varint[ncb] = ((i & ~0x7F) ? 0x80 : 0) | (i & 0x7F);

  if (ncb)
    // Write out our composed varint
    WriteByteArray(varint, ncb);
  else
    // Just write one byte of zero
    WriteByteArray(&ncb, 1);
}

void OArchiveImpl::WriteArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) {
  WriteSize((uint32_t)n);
  
  while (n--)
    desc.serialize(*this, enumerator());
}

uint64_t OArchiveImpl::SizeArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) const {
  uint64_t sz = sizeof(uint32_t);
  while (n--)
    sz += desc.size(*this, enumerator());
  return sz;
}

void OArchiveImpl::WriteDictionary(uint64_t n,
  const field_serializer& keyDesc, std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc, std::function<const void*()> valueEnumerator)
{
  WriteSize((uint32_t)n);
  
  while (n--){
    keyDesc.serialize(*this, keyEnumerator());
    valueDesc.serialize(*this, valueEnumerator());
  }
}

uint64_t OArchiveImpl::SizeDictionary(uint64_t n,
  const field_serializer& keyDesc, std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc, std::function<const void*()> valueEnumerator) const
{
  uint64_t retVal = sizeof(uint32_t);
  while (n--) {
    retVal += keyDesc.size(*this, keyEnumerator());
    retVal += valueDesc.size(*this, valueEnumerator());
  }
  return retVal;
}

uint16_t OArchiveImpl::VarintSize(int64_t value) {
  // Number of bits of significant data
  unsigned long n;
  uint64_t x = value;

#ifdef _MSC_VER
#ifdef _M_X64
  _BitScanReverse64(&n, x);
  n++;
#else
  // Need to fake a 64-bit scan
  DWORD* pDW = (DWORD*) &x;
  if (pDW[1]) {
    _BitScanReverse(&n, pDW[1]);
    n += 32;
  }
  else
    _BitScanReverse(&n, pDW[0]);
  n++;
#endif
#else
  n = 64 - __builtin_clzll(x);
#endif

  // Round up value divided by 7, that's the number of bytes we need to output
  return static_cast<uint16_t>((n + 6) / 7);
}

void OArchiveImpl::Process(void) {
  for (; !deferred.empty(); deferred.pop()) {
    work& w = deferred.front();

    WriteObject(*w.serializer, w.pObj);
  }
}