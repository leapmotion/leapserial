// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ProtobufType.h"
#include "OArchiveImplV0.h"
#include "field_serializer.h"
#include "Descriptor.h"
#include "Utility.hpp"
#include <iostream>

using namespace leap;

OArchiveImplV0::OArchiveImplV0(IOutputStream& os) : OArchiveImpl(os) {}

void OArchiveImplV0::WriteArray(IArrayReader&& ary) {
  uint32_t n = (uint32_t)ary.size();
  WriteSize(n);

  for (uint32_t i = 0; i < n; i++)
    ary.serializer.serialize(*this, ary.get(i));
}

uint64_t OArchiveImplV0::SizeArray(IArrayReader&& ary) const {
  uint64_t sz = sizeof(uint32_t);
  size_t n = ary.size();

  for (size_t i = 0; i < n; i++)
    sz += ary.serializer.size(*this, ary.get(i));
  return sz;
}
