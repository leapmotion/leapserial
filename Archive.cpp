#include "stdafx.h"
#include "Archive.h"
#include <iostream>

using namespace leap;

bool InputStreamAdapter::Read(void* pBuf, uint64_t ncb) {
  return (bool)is.read((char*)pBuf, ncb);
}

bool InputStreamAdapter::Skip(uint64_t ncb) {
  return (bool)is.ignore(ncb);
}

bool OutputStreamAdapter::Write(const void* pBuf, uint64_t ncb) {
  return (bool)os.write((const char*)pBuf, ncb);
}
