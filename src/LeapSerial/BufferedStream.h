// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "BufferedInputStream.h"
#include "IOutputStream.h"

namespace leap {
  class BufferedStream final :
    public BufferedInputStream,
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
    // A mutable version of the buffer pointer
    void* const buffer;

  public:
    bool Write(const void* pBuf, std::streamsize ncb) override;
  };
}
