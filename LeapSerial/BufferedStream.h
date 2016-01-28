// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"

namespace leap {
  class BufferedStream :
    public leap::IInputStream,
    public leap::IOutputStream
  {
  public:
    BufferedStream(void* buffer, size_t ncbBuffer);

  private:
    // The buffered data proper
    void* const buffer;
    const size_t ncbBuffer;

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