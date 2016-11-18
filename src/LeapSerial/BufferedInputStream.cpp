// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "BufferedInputStream.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <memory.h>

using namespace leap;

BufferedInputStream::BufferedInputStream(const void* buffer, size_t ncbBuffer, size_t ncbInitialValid) :
  buffer(buffer),
  ncbBuffer(ncbBuffer),
  m_lastValidByte(ncbInitialValid)
{}

std::streamsize BufferedInputStream::Read(void* pBuf, std::streamsize ncb) {
  const void* pSrcData = static_cast<const uint8_t*>(buffer) + m_readOffset;
  ncb = Skip(ncb);
  memcpy(pBuf, pSrcData, static_cast<size_t>(ncb));
  return ncb;
}

std::streamsize BufferedInputStream::Skip(std::streamsize ncb) {
  auto skipCount = std::min(
    ncb,
    static_cast<std::streamsize>(m_lastValidByte - m_readOffset)
  );
  m_eof = skipCount != ncb;
  m_readOffset += static_cast<size_t>(skipCount);

  return skipCount;
}

std::streamsize BufferedInputStream::Length(void) {
  return static_cast<std::streamsize>(m_lastValidByte - m_readOffset);
}

IInputStream* BufferedInputStream::Seek(std::streampos pos) {
  m_readOffset = std::min<std::streamoff>(std::max<std::streamoff>(0, pos), m_lastValidByte);
  m_eof = m_readOffset != pos && m_readOffset > 0;
  return this;
}
