// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "FilterStreamBase.h"
#include <array>

struct _rijndael_context;

namespace leap {
  class AES256Base
  {
  public:
    AES256Base(const std::array<uint8_t, 32>& key);
    ~AES256Base(void);

  protected:
    std::unique_ptr<_rijndael_context> ctx;

    // Feedback block and our offset into it:
    uint8_t feedback[16];
    uint8_t* feedbackPtr = feedback;
    uint8_t* const feedbackEnd = feedback + 16;

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
    public IInputStream,
    public AES256Base
  {
  public:
    AESDecryptionStream(std::unique_ptr<IInputStream>&& is, const std::array<uint8_t, 32>& key);

  private:
    const std::unique_ptr<IInputStream> is;

  public:
    // IInputStream overrides:
    bool IsEof(void) const override { return is->IsEof(); }
    std::streamsize Length(void) override;
    std::streampos Tell(void) override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
  };
}
