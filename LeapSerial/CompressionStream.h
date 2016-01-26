// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"
#include <cstdint>
#include <memory>
#include <vector>

struct z_stream_s;

namespace leap {
  template<typename T>
  class DecompressionStreamT;

  class ZStreamBase {
  public:
    ZStreamBase(void);
    ZStreamBase(ZStreamBase&& rhs);
    ~ZStreamBase(void);

  protected:
    // zlib state
    std::unique_ptr<z_stream_s> strm;
  };

  /// <summary>
  /// Input decompression stream
  /// <summary>
  class DecompressionStream :
    public IInputStream,
    public ZStreamBase
  {
  public:
    /// <summary>
    /// Initializes the decompression stream at the specified compression level
    /// </summary>
    explicit DecompressionStream(IInputStream& is);

  private:
    IInputStream& is;

    // Fail flag
    bool fail = false;

    // Chunk most recently read from the input stream.  This is generally used as a temporary buffer.
    std::vector<uint8_t> inputChunk;

    // Most recently decompressed chunk and the number of bytes available
    size_t ncbAvail = 0;
    std::vector<uint8_t> buffer;

  public:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
  };

  /// <summary>
  /// Output decompression stream
  /// <summary>
  class CompressionStream :
    public IOutputStream,
    public ZStreamBase
  {
  public:
    /// <summary>
    /// Initializes the compression stream at the specified compression level
    /// </summary>
    /// <param name="os">The underlying stream</param>
    /// <param name="level">The compression level, a value in the range 0 to 9.  Set to -1 to use the default.</param>
    explicit CompressionStream(IOutputStream& os, int level = -1);

    ~CompressionStream(void);

  private:
    // Base output stream, where our data goes
    IOutputStream& os;

    // Output buffer used as scratch space
    std::vector<uint8_t> buffer;

    // Fail bit, used to indicate something went wrong with compression
    bool fail = false;

    bool Write(const void* pBuf, std::streamsize ncb, int flushFlag);

  public:
    // IOutputStream overrides:
    bool Write(const void* pBuf, std::streamsize ncb) override;
    void Flush(void) override;
  };
}
