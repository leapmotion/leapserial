// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"
#include <cstdint>
#include <vector>

namespace leap {
  class MemoryStream:
    public leap::IInputStream,
    public leap::IOutputStream
  {
  public:
    MemoryStream(void);

  private:
    // The buffered data proper
    std::vector<uint8_t> buffer;

    // A pointer to our READ offset in the buffer
    size_t m_readOffset = 0;

    // A pointer to our WRITE offset in the buffer
    size_t m_writeOffset = 0;

  public:
    bool Write(const void* pBuf, std::streamsize ncb) override;
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
    std::streamsize Length(void) override;
  };
}
