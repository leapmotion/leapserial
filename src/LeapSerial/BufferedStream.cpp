// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "BufferedStream.h"
#include <memory.h>

using namespace leap;

BufferedStream::BufferedStream(void* buffer, size_t ncbBuffer, size_t ncbInitialValid) :
  BufferedInputStream{ buffer, ncbBuffer, ncbInitialValid },
  buffer(buffer)
{}

bool BufferedStream::Write(const void* pBuf, std::streamsize ncb) {
  if (ncbBuffer - m_lastValidByte < static_cast<size_t>(ncb))
    // Limit hit, no resize possible!
    return false;

  memcpy(
    static_cast<uint8_t*>(buffer) + m_lastValidByte,
    pBuf,
    static_cast<size_t>(ncb)
  );
  m_lastValidByte += static_cast<size_t>(ncb);
  return true;
}
