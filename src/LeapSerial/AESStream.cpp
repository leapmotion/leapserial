// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "AESStream.h"
#include <aes/rijndael-alg-fst.h>
#include <memory.h>

using namespace leap;

AES256Base::AES256Base(const std::array<uint8_t, 32>& key) :
  ctx(new rijndael_context)
{
  rijndaelKeySetup(ctx.get(), key.data(), 256);
  memset(feedback, 0, sizeof(feedback));
  NextBlock();
}

AES256Base::~AES256Base(void) {}

void AES256Base::NextBlock(void) {
  // Encrypt the last fully encrypted block, we will use this as the basis for xoring the next one
  rijndaelEncrypt(ctx.get(), feedback, feedback);
  feedbackPtr = feedback;
}

AESEncryptionStream::AESEncryptionStream(std::unique_ptr<IOutputStream>&& os, const std::array<uint8_t, 32>& key) :
  OutputFilterStreamBase(std::move(os)),
  AES256Base(key)
{}

bool AESEncryptionStream::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool) {
  size_t nWritten = 0;
  while (nWritten < ncbIn && nWritten < ncbOut) {
    // Instantiate a block if we have to:
    if (feedbackPtr == feedbackEnd)
      NextBlock();

    // XOR to implement our CBC mode
    *feedbackPtr ^= static_cast<const uint8_t*>(input)[nWritten];
    reinterpret_cast<uint8_t*>(output)[nWritten] = *feedbackPtr;

    // Advance
    feedbackPtr++;
    nWritten++;
  }
  ncbOut = nWritten;
  ncbIn = nWritten;

  return true;
}


AESDecryptionStream::AESDecryptionStream(std::unique_ptr<IInputStream>&& is, const std::array<uint8_t, 32>& key) :
  AES256Base(key),
  is(std::move(is))
{}

std::streamsize AESDecryptionStream::Length(void) {
  // We do not add any bytes relative to the underlying stream, we can just return its
  // byte count directly
  return is->Length();
}

std::streampos AESDecryptionStream::Tell(void) {
  return is->Tell();
}

std::streamsize AESDecryptionStream::Read(void* pBuf, std::streamsize ncb) {
  std::streamsize nRead = is->Read(pBuf, ncb);

  for (std::streamsize i = 0; i < nRead; i++) {
    // Generate next block if needed:
    if (feedbackPtr == feedbackEnd)
      NextBlock();

    uint8_t encrypted = static_cast<uint8_t*>(pBuf)[i];
    static_cast<uint8_t*>(pBuf)[i] ^= *feedbackPtr;
    *feedbackPtr++ = encrypted;
  }

  return nRead;
}

std::streamsize AESDecryptionStream::Skip(std::streamsize ncb) {
  uint8_t dump[1024];
  std::streamsize nReadRemain;
  for (nReadRemain = ncb; nReadRemain > 1024;) {
    std::streamsize nRead = Read(dump, sizeof(dump));
    if (nRead <= 0)
      return ncb - nReadRemain;
    nReadRemain -= nRead;
  }

  if(nReadRemain) {
    std::streamsize lastRead = Read(dump, nReadRemain);
    if (0 < lastRead)
      nReadRemain -= lastRead;
  }
  return ncb - nReadRemain;
}
