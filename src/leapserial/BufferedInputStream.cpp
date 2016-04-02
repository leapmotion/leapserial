#include "stdafx.h"
#include "BufferedInputStream.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

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

  if (m_readOffset == m_lastValidByte) {
    // Reset criteria, no data left in the buffer
    m_readOffset = 0;
    m_lastValidByte = 0;
  }
  return skipCount;
}

std::streamsize BufferedInputStream::Length(void) {
  return static_cast<std::streamsize>(m_lastValidByte - m_readOffset);
}
