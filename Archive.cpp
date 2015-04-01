#include "stdafx.h"
#include "Archive.h"
#include "Descriptor.h"
#include "Serializer.h"

using namespace leap;
using namespace leap::internal;

uint16_t OArchive::VarintSize(int64_t value) {
  // Number of bits of significant data
  unsigned long n;
  uint64_t x = value;

#ifdef _MSC_VER
#ifdef _M_X64
  _BitScanReverse64(&n, x);
  n++;
#else
  // Need to fake a 64-bit scan
  DWORD* pDW = (DWORD*) &x;
  if (pDW[1]) {
    _BitScanReverse(&n, pDW[1]);
    n += 32;
  }
  else
    _BitScanReverse(&n, pDW[0]);
  n++;
#endif
#else
  n = 64 - __builtin_clzll(x);
#endif

  // Round up value divided by 7, that's the number of bytes we need to output
  return static_cast<uint16_t>((n + 6) / 7);
}

void OArchive::WriteVarint(int64_t value) const {
  uint8_t varint[10];
  
  // Serialize out
  size_t ncb = 0;
  for (uint64_t i = *reinterpret_cast<uint64_t*>(&value); i; i >>= 7, ncb++)
    varint[ncb] = ((i & ~0x7F) ? 0x80 : 0) | (i & 0x7F);

  if (ncb)
    // Write out our composed varint
    Write(varint, ncb);
  else
    // Just write one byte of zero
    Write(&ncb, 1);
}

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