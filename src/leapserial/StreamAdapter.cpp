// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "StreamAdapter.h"
#include <iostream>

using namespace leap;

InputStreamAdapter::InputStreamAdapter(std::istream& is) :
  is(is)
{}

InputStreamAdapter::InputStreamAdapter(std::unique_ptr<std::istream> pis) :
  is(*pis),
  pis(std::move(pis))
{}

InputStreamAdapter::InputStreamAdapter(const InputStreamAdapter& rhs) :
  is(rhs.is)
{}

bool InputStreamAdapter::IsEof(void) const {
  return is.eof();
}

InputStreamAdapter::~InputStreamAdapter(void) {}

std::streamsize InputStreamAdapter::Read(void* pBuf, std::streamsize ncb) {
  is.read((char*)pBuf, ncb);
  if (is.fail() && !is.eof())
    return -1;
  return is.gcount();
}

std::streamsize InputStreamAdapter::Skip(std::streamsize ncb) {
  is.seekg(ncb, std::ios::cur);
  if (is.fail() && !is.eof())
    return -1;
  return is.gcount();
}

std::streamsize InputStreamAdapter::Length(void) {
  auto pos = is.tellg();
  is.seekg(0, std::ios::end);
  auto retVal = is.tellg();
  is.seekg(pos, std::ios::beg);
  return retVal;
}

std::streampos InputStreamAdapter::Tell(void) {
  return is.tellg();
}

void InputStreamAdapter::Clear(void) {
  is.clear();
}

InputStreamAdapter* InputStreamAdapter::Seek(std::streampos off) {
  is.seekg(off, std::ios::beg);
  return this;
}

OutputStreamAdapter::OutputStreamAdapter(std::ostream& os) :
  os(os)
{}

OutputStreamAdapter::OutputStreamAdapter(std::unique_ptr<std::ostream> pos) :
  os(*pos),
  pos(std::move(pos))
{}

OutputStreamAdapter::OutputStreamAdapter(const OutputStreamAdapter& rhs) :
  os(rhs.os)
{}

OutputStreamAdapter::~OutputStreamAdapter(void) {}

bool OutputStreamAdapter::Write(const void* pBuf, std::streamsize ncb) {
  return (bool)os.write((const char*)pBuf, ncb);
}
