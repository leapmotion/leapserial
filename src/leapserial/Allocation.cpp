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

void* AllocationBase::GetRoot(void)
{
  return this + 1;
}