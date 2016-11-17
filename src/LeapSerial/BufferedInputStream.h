// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"

namespace leap {
  /// <summary>
  /// Buffered stream implementing only input
  class BufferedInputStream :
    public leap::IInputStream
  {
  public:
    /// <summary>Constructor for a stream built on a fixed-size underlying buffer</summary>
    /// <param name="buffer">The underlying buffer itself</param>
    /// <param name="ncbBuffer">The size of the buffer</param>
    /// <param name="ncbInitialValid">
    /// Counts the number of bytes from the beginning of the stream that may be read immediately
    /// after construction of this type.
    /// </param>
    BufferedInputStream(const void* buffer, size_t ncbBuffer, size_t ncbInitialValid);

    /// <summary>Overload that assumes all buffer bytes are available to be read</summary>
    BufferedInputStream(const void* buffer, size_t ncbBuffer) :
      BufferedInputStream(buffer, ncbBuffer, ncbBuffer)
    {}

  protected:
    // The buffered data proper
    const void* const buffer;
    const size_t ncbBuffer;

    // The READ offset in the buffer
    std::streamoff m_readOffset = 0;

    // The last valid byte in the buffer
    std::streamoff m_lastValidByte = 0;

    // True if the last read operation resulted in EOF
    bool m_eof = false;

  public:
    bool IsEof(void) const override { return m_eof; }
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
    std::streamsize Length(void) override;
    IInputStream* Seek(std::streampos off) override;
  };
}