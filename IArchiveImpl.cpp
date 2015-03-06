#include "stdafx.h"
#include "IArchiveImpl.h"
#include <iostream>

using namespace leap;

IArchiveImpl::IArchiveImpl(std::istream& is, internal::AllocationBase& alloc) :
  is(is),
  alloc(alloc)
{
  // This sentry addition means we never have to test objId against zero
  objMap[0] = nullptr;

  // Object ID #1 is the root object
  objMap[1] = alloc.GetRoot();
}

IArchiveImpl::~IArchiveImpl(void) {}

void* IArchiveImpl::Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) {
  auto q = objMap.find(objId);
  if (q != objMap.end())
    return q->second;

  // Not yet initialized, allocate and queue up
  void*& pObj = objMap[objId];
  pObj = alloc.Register(cd);
  work.push(deserialization_task(&serializer, objId, pObj));
  return pObj;
}

void IArchiveImpl::Read(void* pBuf, uint64_t ncb) {
  if(!is.read((char*) pBuf, ncb))
    throw std::runtime_error("End of file reached prematurely");
  m_count += ncb;
}

void IArchiveImpl::Skip(uint64_t ncb) {
  is.ignore(ncb);
}

std::istream& IArchiveImpl::GetStream() const {
  return is;
}

void IArchiveImpl::Process(const deserialization_task& task) {
  objMap[1] = task.pObj;
  work.push(task);

  // Continue to work as long as there is work to be done
  for(; !work.empty(); work.pop()) {
    deserialization_task& task = work.front();

    // Identifier/type comes first
    auto id_type = ReadVarint();

    // Then we need the size (if it's available)
    size_t ncb;
    switch (static_cast<serial_type>(id_type & 7)) {
    case serial_type::b32:
      ncb = 4;
      break;
    case serial_type::b64:
      ncb = 8;
      break;
    case serial_type::string:
      // Size fits right here
      ncb = static_cast<size_t>(ReadVarint());
      break;
    case serial_type::varint:
      // No idea how much, leave it to the consumer
      ncb = 0;
      break;
    }

    task.serializer->deserialize(*this, task.pObj, ncb);
  }
}