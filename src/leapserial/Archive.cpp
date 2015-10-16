// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "Archive.h"
#include <iostream>

using namespace leap;

std::streamsize InputStreamAdapter::Read(void* pBuf, std::streamsize ncb) {
  is.read((char*)pBuf, ncb);
  return is ? is.gcount() : -1;
}

std::streamsize InputStreamAdapter::Skip(std::streamsize ncb) {
  is.ignore(ncb);
  return is ? is.gcount() : -1;
}

bool OutputStreamAdapter::Write(const void* pBuf, std::streamsize ncb) {
  return (bool)os.write((const char*)pBuf, ncb);
}
