// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "descriptors.h"
#include <atomic>

using namespace leap;

descriptor_entry::descriptor_entry(const std::type_info& ti, const descriptor& (*pfnDesc)()) :
  Next(descriptors::Link(*this)),
  ti(ti),
  pfnDesc(pfnDesc)
{}

const descriptor_entry* descriptors::s_pHead = nullptr;

const descriptor_entry* descriptors::Link(descriptor_entry& entry) {
  auto retVal = s_pHead;
  s_pHead = &entry;
  return retVal;
}
