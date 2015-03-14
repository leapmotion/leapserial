#include "stdafx.h"
#include "IArchiveImpl.h"
#include "serial_traits.h"
#include <iostream>

using namespace leap;

IArchiveImpl::IArchiveImpl(std::istream& is, void* pRootObj) :
  is(is)
{
  // This sentry addition means we never have to test objId against zero
  objMap[0] = {nullptr, nullptr};

  // Object ID #1 is the root object
  objMap[1] = {pRootObj, nullptr};
}

IArchiveImpl::~IArchiveImpl(void) {
  ClearObjectTable();
}

void* IArchiveImpl::Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) {
  auto q = objMap.find(objId);
  if (q != objMap.end())
    return q->second.pObj;

  // Not yet initialized, allocate and queue up
  auto& entry = objMap[objId];
  entry.pObj = cd.pfnAlloc();
  entry.pfnFree = cd.pfnFree;
  work.push(deserialization_task(&serializer, objId, entry.pObj));
  return entry.pObj;
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

void IArchiveImpl::Transfer(internal::AllocationBase& alloc) {
  for (auto& cur : objMap)
    if (cur.second.pfnFree)
      // Transfer cleanup responsibility to the allocator
      alloc.garbageList.push_back(
        std::make_pair(
          cur.second.pObj,
          cur.second.pfnFree
        )
      );

  objMap.clear();
}

size_t IArchiveImpl::ClearObjectTable(void) {
  size_t n = 0;
  for (auto& cur : objMap)
    if (cur.second.pfnFree) {
      // One more entry freed
      n++;

      // Transfer cleanup responsibility to the allocator
      cur.second.pfnFree(cur.second.pObj);
    }

  objMap.clear();
  return n;
}

void IArchiveImpl::Process(const deserialization_task& task) {
  objMap[1] = {task.pObj, nullptr};
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