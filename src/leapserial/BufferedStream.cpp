// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "BufferedStream.h"
#include <algorithm>
#include <memory.h>

using namespace leap;

BufferedStream::BufferedStream(void* buffer, size_t ncbBuffer, size_t ncbInitialValid) :
  buffer(buffer),
  ncbBuffer(ncbBuffer),
  m_writeOffset(ncbInitialValid)
{}

bool BufferedStream::Write(const void* pBuf, std::streamsize ncb) {
  if (ncbBuffer - m_writeOffset < static_cast<size_t>(ncb))
    // Limit hit, no resize possible!
    return false;

  memcpy(
    static_cast<uint8_t*>(buffer) + m_writeOffset,
    pBuf,
    static_cast<size_t>(ncb)
  );
  m_writeOffset += static_cast<size_t>(ncb);
  return true;
}

std::streamsize BufferedStream::Read(void* pBuf, std::streamsize ncb) {
  const void* pSrcData = static_cast<uint8_t*>(buffer) + m_readOffset;
  ncb = Skip(ncb);
  memcpy(pBuf, pSrcData, static_cast<size_t>(ncb));
  return ncb;
}

std::streamsize BufferedStream::Skip(std::streamsize ncb) {
  auto skipCount = std::min(
    ncb,
    static_cast<std::streamsize>(m_writeOffset - m_readOffset)
  );
  m_eof = skipCount != ncb;
  m_readOffset += static_cast<size_t>(skipCount);

  if (m_readOffset == m_writeOffset) {
    // Reset criteria, no data left in the buffer
    m_readOffset = 0;
    m_writeOffset = 0;
  }
  return skipCount;
}

std::streamsize BufferedStream::Length(void) {
  return static_cast<std::streamsize>(m_writeOffset - m_readOffset);
}
