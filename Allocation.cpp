#include "stdafx.h"
#include "Allocation.h"
#include "field_serializer.h"

using namespace leap;
using namespace leap::internal;

AllocationBase::AllocationBase(void) {}

AllocationBase::~AllocationBase(void) {
  for (auto& cur : garbageList)
    cur.second(cur.first);
}

void* AllocationBase::Register(const create_delete& cd) {
  // Allocate, record the allocation, record the operation required for deletion,
  // and return the resulting pointer to the caller.
  void* retVal = cd.pfnAlloc();
  garbageList.push_back(std::make_pair(retVal, cd.pfnFree));
  return retVal;
}

void* AllocationBase::GetRoot(void)
{
  return this + 1;
}