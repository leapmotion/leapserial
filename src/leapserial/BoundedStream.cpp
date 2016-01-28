// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "BoundedStream.h"

using namespace leap;

BoundedInputStream::BoundedInputStream(std::unique_ptr<IInputStream>&& is, std::streamsize readLimit) :
  ReadLimit(readLimit),
  is(std::move(is))
{}

BoundedInputStream::~BoundedInputStream(void) {}

bool BoundedInputStream::IsEof(void) const {
  return m_totalRead == ReadLimit || is->IsEof();
}

std::streamsize BoundedInputStream::Read(void* pBuf, std::streamsize ncb) {
  std::streamsize limit = ReadLimit - m_totalRead;
  if(ncb > limit)
    ncb = static_cast<std::streamsize>(limit);

  auto rs = is->Read(pBuf, ncb);
  if (0 < rs)
    m_totalRead += rs;
  return rs;
}

std::streamsize BoundedInputStream::Skip(std::streamsize ncb) {
  std::streamsize limit = ReadLimit - m_totalRead;
  if (ncb > limit)
    ncb = static_cast<std::streamsize>(limit);
  auto rs = is->Skip(ncb);
  if (0 < rs)
    m_totalRead += rs;
  return rs;
}

BoundedOutputStream::BoundedOutputStream(std::unique_ptr<IOutputStream>&& os, std::streamsize writeLimit) :
  WriteLimit(writeLimit),
  os(std::move(os))
{}

bool BoundedOutputStream::Write(const void* pBuf, std::streamsize ncb) {
  std::streamsize limit = WriteLimit - m_totalWritten;
  if (ncb > limit)
    return false;
  if (!os->Write(pBuf, ncb))
    return false;

  m_totalWritten += ncb;
  return true;
}

void BoundedOutputStream::Flush(void) {
  os->Flush();
}
