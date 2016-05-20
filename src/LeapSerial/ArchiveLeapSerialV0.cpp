// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "ProtobufType.h"
#include "ArchiveLeapSerialV0.h"
#include "field_serializer.h"
#include "Descriptor.h"
#include "Utility.hpp"
#include <iostream>

using namespace leap;


IArchiveLeapSerialV0::IArchiveLeapSerialV0(IInputStream& is) : IArchiveLeapSerial(is) { }

IArchiveLeapSerialV0::IArchiveLeapSerialV0(std::istream& is) : IArchiveLeapSerial(is) {}

void IArchiveLeapSerialV0::ReadArray(IArrayAppender&& ary) {
  // Read the number of entries first:
  uint32_t nEntries;
  ReadByteArray(&nEntries, sizeof(nEntries));
  ary.reserve(nEntries);

  // Now loop until we get the desired number of entries from the stream
  for (size_t i = 0; i < nEntries; i++)
    ary.serializer.deserialize(*this, ary.allocate(), 0);
}

OArchiveLeapSerialV0::OArchiveLeapSerialV0(IOutputStream& os) : OArchiveLeapSerial(os) {}

void OArchiveLeapSerialV0::WriteArray(IArrayReader&& ary) {
  uint32_t n = (uint32_t)ary.size();
  WriteSize(n);

  for (uint32_t i = 0; i < n; i++)
    ary.serializer.serialize(*this, ary.get(i));
}

uint64_t OArchiveLeapSerialV0::SizeArray(IArrayReader&& ary) const {
  uint64_t sz = sizeof(uint32_t);
  size_t n = ary.size();

  for (size_t i = 0; i < n; i++)
    sz += ary.serializer.size(*this, ary.get(i));
  return sz;
}
