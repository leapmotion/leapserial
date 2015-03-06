#include "stdafx.h"
#include "OArchiveImpl.h"
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

uint32_t OArchiveImpl::RegisterObject(const field_serializer& serializer, const void* pObj)
{
  auto q = objMap.find(pObj);
  if (q == objMap.end()) {
    // Obtain a new identifier
    ++lastID;

    // Need to register this object for serialization
    deferred.push(work(lastID, &serializer, pObj));

    // Now set the ID that we will be returning to the user and tracking later
    objMap[pObj] = lastID;
    return lastID;
  }

  // Object ID allocated
  return q->second;
}

void OArchiveImpl::Write(const void* pBuf, uint64_t ncb) const {
  os.write((const char*) pBuf, ncb);
}

std::ostream& OArchiveImpl::GetStream() const {
  return os;
}

void OArchiveImpl::Process(void) {
  for (; !deferred.empty(); deferred.pop()) {
    work& w = deferred.front();

    // Write the expected size first in a string-type field.  Identifier first, string
    // type, then the length
    WriteVarint(
      (w.id << 3) |
      static_cast<uint32_t>(serial_type::string)
    );
    WriteVarint(w.serializer->size(w.pObj));

    // Now hand off to this type's serialization behavior
    w.serializer->serialize(*this, w.pObj);
  }
}