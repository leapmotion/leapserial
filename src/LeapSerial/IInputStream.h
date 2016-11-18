// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <ios>
#include <iosfwd>
#include <stdexcept>

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

    /// <returns>
    /// The number of bytes remaining in the input stream
    /// </returns>
    /// <remarks>
    /// Some input streams, particularly network streams, do not support a length concept.  This
    /// information cannot always be determined statically, especially in the case of filter
    /// streams whose underlying stream may be determined at runtime.
    /// </remarks>
    virtual std::streamsize Length(void) { return -1; }

    /// <summary>
    /// Gets the stream offset
    /// </summary>
    /// <returns>The current offset, if supported, otherwise -1</returns>
    virtual std::streampos Tell(void) { return -1; }

    /// <summary>
    /// Clears all error conditions possibly set on the underlying stream, if they exist
    /// </summary>
    virtual void Clear(void) {}

    /// <summary>
    /// Sets the stream offset
    /// </summary>
    /// <returns>This, if the operation is supported, otherwise throw</returns>
    virtual IInputStream* Seek(std::streampos off) { throw std::runtime_error("Seek is not implemented for this stream type"); }
  };
}
