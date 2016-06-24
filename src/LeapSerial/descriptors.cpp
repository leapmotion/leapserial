// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "descriptors.h"
#include "Descriptor.h"
#include <atomic>

using namespace leap;

descriptor_entry::descriptor_entry(const std::type_info& ti, const descriptor& (*pfnDesc)()) :
  Next(descriptors::Link(*this)),
  ti(ti),
  pfnDesc(pfnDesc)
{}

const descriptor_entry* descriptors::s_pHead = nullptr;

#if _MSC_VER <= 1800
// Used for MSVC2013 because it doesn't have magic statics, so we have to initialize all
// of our cached descriptors to ensure that we don't wind up with initialization races.
static bool global_initializer = [] {
  for (auto& cur : descriptors{})
    cur.pfnDesc();
  return true;
}();
#endif

const descriptor_entry* descriptors::Link(descriptor_entry& entry) {
  auto retVal = s_pHead;
  s_pHead = &entry;
  return retVal;
}
