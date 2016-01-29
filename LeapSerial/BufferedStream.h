// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"

namespace leap {
  class BufferedStream final :
    public leap::IInputStream,
    public leap::IOutputStream
  {
  public:
    /// <summary>Constructor for a stream built on a fixed-size underlying buffer</summary>
    /// <param name="buffer">The underlying buffer itself</param>
    /// <param name="ncbBuffer">The size of the buffer</param>
    /// <param name="ncbInitialValid">
    /// Counts the number of bytes from the beginning of the stream that may be read immediately
    /// after construction of this type
    /// </param>
    BufferedStream(void* buffer, size_t ncbBuffer, size_t ncbInitialValid = 0);

  private:
    // The buffered data proper
    void* const buffer;
    const size_t ncbBuffer;

    // A pointer to our READ offset in the buffer
    size_t m_readOffset = 0;

    // A pointer to our WRITE offset in the buffer
    size_t m_writeOffset;

    // True if the last read operation resulted in EOF
    bool m_eof = false;

  public:
    bool Write(const void* pBuf, std::streamsize ncb) override;
    bool IsEof(void) const override { return m_eof; }
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
    std::streamsize Length(void) override;
  };
}
