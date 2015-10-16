#include "stdafx.h"
#include "Utility.hpp"

std::array<uint8_t, 10> leap::ToBase128(uint64_t val, size_t& ncb) {
  if (!val)
    return{};

  std::array<uint8_t, 10> varint;

  // Serialize out
  ncb = 0;
  for (uint64_t i = *reinterpret_cast<uint64_t*>(&val); i; i >>= 7, ncb++)
    varint[ncb] = ((i & ~0x7F) ? 0x80 : 0) | (i & 0x7F);
  return varint;
}

uint8_t leap::SizeBase128(uint64_t val) {
  //Bitscan and clz11 do not work with the null case.
  if (val == 0)
    return 1;

  // Number of bits of significant data
  unsigned long n = 0;
  uint64_t x = val;

#ifdef _MSC_VER
#ifdef _M_X64
  _BitScanReverse64(&n, x);
  n++;
#else
  // Need to fake a 64-bit scan
  DWORD* pDW = (DWORD*)&x;
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
  return static_cast<uint8_t>((n + 6) / 7);
}

uint64_t leap::FromBase128(uint8_t* data, size_t ncb) {
  uint64_t retVal = 0;
  for (size_t i = 0; i < ncb; i++)
    retVal |= uint64_t(data[i] & 0x7F) << (i * 7);
  return retVal;
}
