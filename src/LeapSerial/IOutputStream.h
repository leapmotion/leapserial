// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include <cstdint>
#include <ios>
#include <iosfwd>

namespace leap {
  class IInputStream;

  /// <summary>
  /// Stream adaptor interface for use with the Archive type
  /// </summary>
  class IOutputStream {
  public:
    virtual ~IOutputStream(void) {}

    enum class CopyResult {
      // The requested number of bytes were transferred
      Ok,

      // The requested number of bytes could not be transferred because the input stream
      // reached the end of file
      InputStreamEof,

      // An error occurred while attempting to read the input stream
      InputStreamError,

      // A call to IOutputStream::Write returned false during the transfer operation
      OutputStreamWriteFail
    };

    /// <summary>
    /// Writes all of the specified bytes to the output stream
    /// </summary>
    virtual bool Write(const void* pBuf, std::streamsize ncb) = 0;

    /// <summary>
    /// Writes the specified number of bytes from the specified input stream
    /// </summary>
    /// <param name="is">The source of bytes to be written</param>
    /// <param name="ncb">
    /// The number of bytes to be written; on output, the number of bytes written.
    /// </param>
    /// <remarks>
    /// If ncb is -1, this function will never return CopyResult::Ok, and will write bytes
    /// from the input stream until the input stream is exhausted.
    ///
    /// If this function returns OutputStreamWriteFail, then some bytes read from the input
    /// stream could not be transferred to the output stream.  These bytes are lost.
    /// </remarks>
    CopyResult Write(IInputStream& is, std::streamsize& ncb) {
      uint8_t buf[1024];
      return Write(is, buf, sizeof(buf), ncb);
    }

    /// <summary>
    /// Writes the specified number of bytes from the specified input stream
    /// </summary>
    /// <param name="is">The source of bytes to be written</param>
    /// <param name="ncb">
    /// The number of bytes to be written; on output, the number of bytes written.
    /// </param>
    /// <param name="scratch">Scratch space used for the transfer operation</param>
    /// <param name="ncbScratch">The size of the scratch space</param>
    /// <remarks>
    /// If ncb is -1, this function will never return CopyResult::Ok, and will write bytes
    /// from the input stream until the input stream is exhausted.
    ///
    /// If this function returns OutputStreamWriteFail, then some bytes read from the input
    /// stream could not be transferred to the output stream.  These bytes will be contained
    /// in the scratch space.
    /// </remarks>
    virtual CopyResult Write(IInputStream& is, void* scratch, std::streamsize ncbScratch, std::streamsize& ncb) {
      std::streamsize ncbRemain = ncb;
      ncb = 0;

      if (ncbRemain < 0)
        for (;;) {
          std::streamsize ss = is.Read(scratch, ncbScratch);
          if (!ss)
            return CopyResult::InputStreamEof;
          if (ss < 0)
            return CopyResult::InputStreamError;

          ncb += ss;
          if (!Write(scratch, ss))
            return CopyResult::OutputStreamWriteFail;
        }
      else
        while (ncbRemain) {
          std::streamsize ss = is.Read(
            scratch,
            ncbRemain < ncbScratch ? ncbRemain : ncbScratch
          );
          if (!ss)
            return CopyResult::InputStreamEof;
          if (ss < 0)
            return CopyResult::InputStreamError;

          ncb += ss;
          ncbRemain -= ss;
          if (!Write(scratch, ss))
            return CopyResult::OutputStreamWriteFail;
        }

      return CopyResult::Ok;
    }

    /// <summary>
    /// Causes any unwritten data to be flushed from memory
    /// </summary>
    virtual void Flush(void) {}
  };
}
