// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "Encryption.h"
#include "Utility.h"
#include "aes/rijndael-alg-fst.h"
#include "LeapSerial/AESStream.h"
#include "LeapSerial/BufferedStream.h"
#include "LeapSerial/LeapSerial.h"

using namespace std::chrono;

static const std::array<uint8_t, 32> sc_key = { 0x44, 0x99, 0x66 };

Encryption::Encryption(void) {
  buffer.resize(ncbRead);
}

nanoseconds Encryption::SimpleRead(void) {
  leap::BufferedStream stream{ buffer.data(), buffer.size(), buffer.size() };

  auto start = high_resolution_clock::now();
  uint8_t temp[1024];
  while (!stream.IsEof())
    stream.Read(temp, sizeof(temp));
  return high_resolution_clock::now() - start;
}

std::chrono::nanoseconds Encryption::DirectEncryptECB(void) {
  rijndael_context rk;
  rijndaelKeySetup(&rk, sc_key.data(), 256);

  auto start = high_resolution_clock::now();
  for (size_t i = 0; i < buffer.size(); i += 16) {
    rijndaelEncrypt(&rk, &buffer[i], &buffer[i]);
  }
  return high_resolution_clock::now() - start;
}

std::chrono::nanoseconds Encryption::DirectEncryptCFB(void) {
  rijndael_context rk;
  rijndaelKeySetup(&rk, sc_key.data(), 256);

  auto start = high_resolution_clock::now();

  unsigned char feedback[16] = {};
  for (size_t i = 0; i < buffer.size(); i += 16) {
    rijndaelEncrypt(&rk, feedback, feedback);
    for (size_t j = 0; j < 16; j++)
      feedback[j] ^= buffer[i + j];
  }
  return high_resolution_clock::now() - start;
}

std::chrono::nanoseconds Encryption::EncryptedRead(void) {
  leap::AESDecryptionStream aes{
    leap::make_unique<leap::BufferedStream>(buffer.data(), buffer.size(), buffer.size()),
    sc_key
  };

  auto start = high_resolution_clock::now();
  uint8_t temp[1024];
  while (!aes.IsEof())
    aes.Read(temp, sizeof(temp));
  return high_resolution_clock::now() - start;
}

int Encryption::Benchmark(std::ostream& os) {
  nanoseconds dt;

  os << (buffer.size() / (1024 * 1024)) << "MB buffer size" << std::endl;
  os << "Trivial read:   " << std::flush;
  os << format_duration(SimpleRead()) << std::endl;
  os << "Direct AES-ECB: " << std::flush;
  os << format_duration(DirectEncryptECB()) << std::endl;
  os << "Direct AES-CFB: " << std::flush;
  os << format_duration(DirectEncryptCFB()) << std::endl;
  os << "Encrypted read: " << std::flush;
  os << format_duration(EncryptedRead()) << std::endl;

  return 0;
}
