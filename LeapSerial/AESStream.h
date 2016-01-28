// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "FilterStreamBase.h"
#include <array>

struct _aes256_context;

namespace leap {
  class AES256Base
  {
  public:
    AES256Base(const std::array<uint8_t, 32>& key);
    ~AES256Base(void);

  protected:
    std::unique_ptr<_aes256_context> ctx;

    // Number of transform bytes remaining, and the chained-forward block itself
    size_t blockByte = 16;
    std::array<uint8_t, 16> chained;

    // Previously encrypted bytes
    std::array<uint8_t, 16> lastBlock;

    void NextBlock(void);
  };

  /// <summary>
  /// Implements an AES stream encryption cipher in CFB mode
  /// </summary>
  class AESEncryptionStream :
    public OutputFilterStreamBase,
    public AES256Base
  {
  public:
    /// <summary>
    /// Initializes the encryption stream
    /// </summary>
    AESEncryptionStream(std::unique_ptr<IOutputStream>&& os, const std::array<uint8_t, 32>& key);

  protected:
    // OutputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) override;
  };

  /// <summary>
  /// Implements an AES stream decryption cipher in CFB mode
  /// </summary>
  class AESDecryptionStream :
    public InputFilterStreamBase,
    public AES256Base
  {
  public:
    AESDecryptionStream(std::unique_ptr<IInputStream>&& is, const std::array<uint8_t, 32>& key);

  protected:
    // InputFilterStreamBase overrides:
    bool Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) override;
  };
}
