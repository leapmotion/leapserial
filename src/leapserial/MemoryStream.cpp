// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "MemoryStream.h"
#include <algorithm>

using namespace leap;

MemoryStream::MemoryStream(void) {}

bool MemoryStream::Write(const void* pBuf, std::streamsize ncb) {
  if (buffer.size() - m_writeOffset < ncb)
    // Exponential growth on the buffer:
    buffer.resize(
      std::max(
        buffer.size() * 2,
        m_writeOffset + static_cast<size_t>(ncb)
      )
    );

  memcpy(
    buffer.data() + m_writeOffset,
    pBuf,
    static_cast<size_t>(ncb)
  );
  m_writeOffset += static_cast<size_t>(ncb);
  return true;
}

bool MemoryStream::IsEof(void) const {
  return m_readOffset >= buffer.size();
}

std::streamsize MemoryStream::Read(void* pBuf, std::streamsize ncb) {
  const void* pSrcData = buffer.data() + m_readOffset;
  ncb = Skip(ncb);
  memcpy(pBuf, pSrcData, static_cast<size_t>(ncb));
  return ncb;
}

std::streamsize MemoryStream::Skip(std::streamsize ncb) {
  ncb = std::min(
    ncb,
    static_cast<std::streamsize>(buffer.size() - m_readOffset)
  );
  m_readOffset += static_cast<size_t>(ncb);

  if (m_readOffset == buffer.size()) {
    // Reset criteria, no data left in the buffer
    m_readOffset = 0;
    m_writeOffset = 0;
  }
  return ncb;
}

std::streamsize MemoryStream::Length(void) {
  return static_cast<std::streamsize>(m_writeOffset - m_readOffset);
}
