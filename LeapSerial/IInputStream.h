// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <ios>
#include <iosfwd>

namespace leap {
  /// <summary>
  /// Stream adaptor interface for use with the Archive type
  /// </summary>
  class IInputStream {
  public:
    virtual ~IInputStream(void) {}

    /// <returns>
    /// False if the next call to Read might succeed
    /// </returns>
    virtual bool IsEof(void) const = 0;

    /// <summary>
    /// Reads exactly the specified number of bytes from the input stream
    /// </summary>
    /// <returns>
    /// The number of bytes actually read, -1 if there was an error.  The number of bytes read
    /// may be less than the number of bytes requested if the end of the file was encountered
    /// before the operation completed.
    /// </returns>
    virtual std::streamsize Read(void* pBuf, std::streamsize ncb) = 0;

    /// <summary>
    /// Discards the specified number of bytes from the input stream
    /// </summary>
    virtual std::streamsize Skip(std::streamsize ncb) = 0;
  };
}
