#include "stdafx.h"
#include "ArchiveJSON.h"

using namespace leap;

struct not_implemented_exception :
  public std::runtime_error
{
public:
  not_implemented_exception() : std::runtime_error("This function is not yet implemented!") {}
};

OArchiveJSON::OArchiveJSON(std::ostream& os) : os(os) {}

void OArchiveJSON::WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteBool(bool value) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteInteger(int64_t value, uint8_t ncb) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteFloat(float value) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteFloat(double value) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteObjectReference(const field_serializer& serializer, const void* pObj) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteObject(const field_serializer& serializer, const void* pObj) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteDescriptor(const descriptor& descriptor, const void* pObj) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) {
  throw not_implemented_exception();
}

void OArchiveJSON::WriteDictionary(
  uint64_t n,
  const field_serializer& keyDesc,
  std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc,
  std::function<const void*()> valueEnumerator
  )
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

uint64_t OArchiveJSON::SizeArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) const {
  throw not_implemented_exception();
}

uint64_t OArchiveJSON::SizeDictionary(
  uint64_t n,
  const field_serializer& keyDesc,
  std::function<const void*()> keyEnumerator,
  const field_serializer& valueDesc,
  std::function<const void*()> valueEnumerator
  ) const
{
  throw not_implemented_exception();
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

void IArchiveJSON::ReadArray(std::function<void(uint64_t)> sizeBufferFn, const field_serializer& t_serializer, std::function<void*()> enumerator, uint64_t expectedEntries) {
  throw not_implemented_exception();
}

void IArchiveJSON::ReadDictionary(const field_serializer& keyDesc, void* key, const field_serializer& valueDesc, void* value, std::function<void(const void* key, const void* value)> inserter) {
  throw not_implemented_exception();
}
