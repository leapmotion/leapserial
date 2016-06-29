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
    explicit InputFilterStreamBase(std::unique_ptr<IInputStream>&& is);

  protected:
    const std::unique_ptr<IInputStream> is;

    // EOF flag
    bool eof = false;

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
    bool IsEof(void) const override { return eof; }
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
    explicit OutputFilterStreamBase(std::unique_ptr<IOutputStream>&& os);

    ~OutputFilterStreamBase(void);

  protected:
    // Base output stream, where our data goes
    const std::unique_ptr<IOutputStream> os;

    // Output buffer used as scratch space
    std::vector<uint8_t> buffer;

    // Fail bit, used to indicate something went wrong with compression
    bool fail = false;

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
