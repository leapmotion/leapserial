// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"
#include <cstdint>
#include <memory>
#include <vector>

struct z_stream_s;

namespace leap {
  /// <summary>
  /// Input decompression stream
  /// <summary>
  class InputFilterStreamBase :
    public IInputStream
  {
  public:
    /// <summary>
    /// Initializes the decompression stream at the specified compression level
    /// </summary>
    explicit InputFilterStreamBase(IInputStream& is);

  private:
    IInputStream& is;

    // Fail flag
    bool fail = false;

    // Chunk most recently read from the input stream.  This is generally used as a temporary buffer.
    size_t inChunkRemain = 0;
    std::vector<uint8_t> inputChunk;

    // Chunk most recently transformed by the base type
    size_t ncbAvail = 0;
    std::vector<uint8_t> buffer;

  protected:
    /// <summary>
    /// In-memory transform operation
    /// </summary>
    /// <param name="ncbIn">On input, the size of input; on output, the number of bytes parsed from input</param>
    /// <param name="ncbOut">The number of bytes in the output buffer; on output, the number of bytes written to the output buffer</param>
    virtual bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) = 0;

  public:
    // IInputStream overrides:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
  };

  /// <summary>
  /// Output decompression stream
  /// <summary>
  class OutputFilterStreamBase :
    public IOutputStream
  {
  public:
    /// <summary>
    /// Initializes the compression stream at the specified compression level
    /// </summary>
    /// <param name="os">The underlying stream</param>
    /// <param name="level">The compression level, a value in the range 0 to 9.  Set to -1 to use the default.</param>
    explicit OutputFilterStreamBase(IOutputStream& os, int level = -1);

    ~OutputFilterStreamBase(void);

  private:
    // Base output stream, where our data goes
    IOutputStream& os;

    // Output buffer used as scratch space
    std::vector<uint8_t> buffer;

    // Fail bit, used to indicate something went wrong with compression
    bool fail = false;

  protected:
    /// <summary>
    /// In-memory transform operation
    /// </summary>
    /// <param name="ncbIn">On input, the size of input; on output, the number of bytes parsed from input</param>
    /// <param name="ncbOut">The number of bytes in the output buffer; on output, the number of bytes written to the output buffer</param>
    /// <param name="flush">True if this is a flush operation</param>
    virtual bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) = 0;

    bool Write(const void* pBuf, std::streamsize ncb, bool flush);

  public:
    // IOutputStream overrides:
    bool Write(const void* pBuf, std::streamsize ncb) override;
    void Flush(void) override;
  };
}
