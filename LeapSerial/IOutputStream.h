// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <ios>
#include <iosfwd>

namespace leap {
  /// <summary>
  /// Stream adaptor interface for use with the Archive type
  /// </summary>
  class IOutputStream {
  public:
    virtual ~IOutputStream(void) {}

    /// <summary>
    /// Writes all of the specified bytes to the output stream
    /// </summary>
    virtual bool Write(const void* pBuf, std::streamsize ncb) = 0;

    /// <summary>
    /// Causes any unwritten data to be flushed from memory
    /// </summary>
    virtual void Flush(void) {}
  };
}
