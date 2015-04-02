#include "stdafx.h"
#include "Archive.h"
#include "Descriptor.h"
#include "Serializer.h"

using namespace leap;
using namespace leap::internal;

int64_t IArchive::ReadVarint(void) {
  uint8_t ch;
  int64_t retVal = 0;
  size_t ncb = 0;

  do {
    Read(&ch, 1);
    retVal |= uint64_t(ch & 0x7F) << (ncb * 7);;
    ++ncb;
  }
  while (ch & 0x80);
  return retVal;
}