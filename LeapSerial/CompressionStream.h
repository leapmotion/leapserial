// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "FilterStreamBase.h"
#include <cstdint>
#include <memory>
#include <vector>

struct z_stream_s;

namespace leap {
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
    public InputFilterStreamBase,
    public ZStreamBase
  {
  public:
    /// <summary>
    /// Initializes the decompression stream at the specified compression level
    /// </summary>
    explicit DecompressionStream(IInputStream& is);

  protected:
    // InputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) override;
  };

  /// <summary>
  /// Output decompression stream
  /// <summary>
  class CompressionStream :
    public OutputFilterStreamBase,
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
    // OutputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) override;
  };
}
