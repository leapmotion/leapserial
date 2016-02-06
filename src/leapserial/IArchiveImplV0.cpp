// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "IArchiveImplV0.h"
#include "field_serializer.h"
#include "serial_traits.h"
#include "ProtobufType.h"
#include "Utility.hpp"
#include <iostream>
#include <sstream>

using namespace leap;

IArchiveImplV0::IArchiveImplV0(IInputStream& is) : IArchiveImpl(is) { }

IArchiveImplV0::IArchiveImplV0(std::istream& is) : IArchiveImpl(is) {}

void IArchiveImplV0::ReadArray(IArrayAppender&& ary) {
  // Read the number of entries first:
  uint32_t nEntries;
  ReadByteArray(&nEntries, sizeof(nEntries));
  ary.reserve(nEntries);

  // Now loop until we get the desired number of entries from the stream
  for (size_t i = 0; i < nEntries; i++)
    ary.serializer.deserialize(*this, ary.allocate(), 0);
}