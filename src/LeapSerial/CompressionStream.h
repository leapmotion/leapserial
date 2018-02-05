// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "FilterStreamBase.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace leap {

  /// <summary>
  /// Supported compression/decompression implementations:
  /// </summary>
  struct Zlib;
  struct BZip2;

  /// <summary>
  /// Decompression interface
  /// </summary>
  template<typename T>
  class Decompressor {
  public:
    Decompressor(void);
    ~Decompressor(void);
  protected:
    std::unique_ptr<T> impl;
  };

  /// <summary>
  /// Compression interface
  /// </summary>
  template<typename T>
  class Compressor {
  public:
    Compressor(int level = 9);
    ~Compressor(void);
  protected:
    std::unique_ptr<T> impl;
  };

  /// <summary>
  /// Input decompression stream
  /// <summary>
  template<typename T = Zlib>
  class DecompressionStream :
    public InputFilterStreamBase,
    public Decompressor<T>
  {
  public:
    /// <summary>
    /// Initializes the decompression stream
    /// </summary>
    explicit DecompressionStream(std::unique_ptr<IInputStream>&& is);

  protected:
    // InputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut);
  };

  /// <summary>
  /// Output compression stream
  /// <summary>
  template<typename T = Zlib>
  class CompressionStream :
    public OutputFilterStreamBase,
    public Compressor<T>
  {
  public:
    /// <summary>
    /// Initializes the compression stream at the specified compression level
    /// </summary>
    /// <param name="os">The underlying stream</param>
    /// <param name="level">The compression level, a value in the range 0 to 9.  Set to -1 to use the default.</param>
    explicit CompressionStream(std::unique_ptr<IOutputStream>&& os, int level = -1);

    ~CompressionStream(void);

  private:
    // OutputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) override;
  };
}
