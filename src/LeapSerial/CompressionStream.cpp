// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "CompressionStream.h"
#include "LeapSerial.h"
#include <algorithm>
#include <memory.h>
#include <zlib/zlib.h>
#include <bzip2/bzlib.h>

namespace leap {

struct Zlib {
  Zlib(void) {
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;
  }

  // zlib state
  z_stream_s strm;
};

// Zlib decompressor
template<> Decompressor<Zlib>::Decompressor(void) : impl{leap::make_unique<Zlib>()} { inflateInit(&impl->strm); }
template<> Decompressor<Zlib>::~Decompressor(void) { inflateEnd(&impl->strm); }

// Zlib compressor
template<> Compressor<Zlib>::Compressor(int level) : impl{leap::make_unique<Zlib>()} {
  if (level < -1 || 9 < level)
    throw std::invalid_argument("Compression stream level must be in the range [0, 9]");

  deflateInit(&impl->strm, level);
}
template<> Compressor<Zlib>::~Compressor(void) { deflateEnd(&impl->strm); }

struct BZip2 {
  BZip2(void) {
    strm.bzalloc = nullptr;
    strm.bzfree = nullptr;
    strm.opaque = nullptr;
    strm.avail_in = 0;
    strm.next_in = nullptr;
  }

  // zlib state
  bz_stream strm;
};

template<> Decompressor<BZip2>::Decompressor(void) : impl{leap::make_unique<BZip2>()} { BZ2_bzDecompressInit(&impl->strm, 0, 0); }
template<> Decompressor<BZip2>::~Decompressor(void) { BZ2_bzDecompressEnd(&impl->strm); }

template<> Compressor<BZip2>::Compressor(int level) : impl{leap::make_unique<BZip2>()} {
  if (level < -1 || 9 < level)
    throw std::invalid_argument("Compression stream level must be in the range [0, 9]");

  if (level == -1)
    level = 9;

  BZ2_bzCompressInit(&impl->strm, level, 0, 0);
}
template<> Compressor<BZip2>::~Compressor(void) { BZ2_bzCompressEnd(&impl->strm); }

template<typename T>
DecompressionStream<T>::DecompressionStream(std::unique_ptr<IInputStream>&& is) :
  InputFilterStreamBase(std::move(is))
{}

template<typename T>
bool DecompressionStream<T>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) {
  return false;
}

template<typename T>
CompressionStream<T>::CompressionStream(std::unique_ptr<IOutputStream>&& os, int level) :
  OutputFilterStreamBase(std::move(os)),
  Compressor<T>(level)
{}

template<typename T>
CompressionStream<T>::~CompressionStream(void)
{}

template<typename T>
bool CompressionStream<T>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) {
  return false;
}

// Zlib specialization

template<>
bool DecompressionStream<Zlib>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) {
  // Decompress into our buffer space
  impl->strm.next_out = reinterpret_cast<uint8_t*>(output);
  impl->strm.avail_out = static_cast<uint32_t>(ncbOut);
  impl->strm.next_in = reinterpret_cast<const uint8_t*>(input);
  impl->strm.avail_in = static_cast<uint32_t>(ncbIn);
  if (inflate(&impl->strm, Z_NO_FLUSH) == Z_STREAM_ERROR)
    return false;

  // Store the amount we actually read/wrote
  ncbIn -= impl->strm.avail_in;
  ncbOut -= impl->strm.avail_out;
  return true;
}

template<>
CompressionStream<Zlib>::~CompressionStream(void) {
  uint8_t buf[256];

  // Completed!  Finish writing anything that remains to be written, and keep going
  // as long as zlib fills up the proffered output buffer.
  for (;;) {
    impl->strm.avail_in = 0;
    impl->strm.next_in = nullptr;
    impl->strm.next_out = buf;
    impl->strm.avail_out = sizeof(buf);

    int ret = deflate(&impl->strm, Z_FINISH);
    switch (ret) {
    case Z_STREAM_END:  // Return case when we are at the end
    case Z_OK:          // Return case when more data exists to be written

      // Handoff to lower level stream to complete the write
      os->Write(buf, sizeof(buf) - impl->strm.avail_out);

      if (ret == Z_STREAM_END)
        // Clean return
        return;
      break;
    default:
      // Something went wrong, and we can't throw from here, so we just have to give up
      return;
    }
  }
}

template<>
bool CompressionStream<Zlib>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) {
  impl->strm.avail_in = static_cast<uint32_t>(ncbIn);
  impl->strm.next_in = reinterpret_cast<const uint8_t*>(input);

  // Compress one, update results
  impl->strm.next_out = reinterpret_cast<uint8_t*>(output);
  impl->strm.avail_out = static_cast<uint32_t>(ncbOut);
  int rs = deflate(&impl->strm, flush ? Z_FULL_FLUSH : Z_NO_FLUSH);

  ncbIn -= impl->strm.avail_in;
  ncbOut -= impl->strm.avail_out;
  return rs == Z_OK;
}

// BZip2 specialization

template<>
bool DecompressionStream<BZip2>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) {
  // Decompress into our buffer space
  impl->strm.next_out = reinterpret_cast<char*>(output);
  impl->strm.avail_out = static_cast<unsigned int>(ncbOut);
  impl->strm.next_in = const_cast<char*>(reinterpret_cast<const char*>(input));
  impl->strm.avail_in = static_cast<unsigned int>(ncbIn);
  unsigned long avail_in, avail_out;
  int rs;
  do {
    avail_in = impl->strm.avail_in;
    avail_out = impl->strm.avail_out;
    rs = BZ2_bzDecompress(&impl->strm);
  } while (rs == BZ_OK && (avail_in != impl->strm.avail_in || avail_out != impl->strm.avail_out));
  if (rs != BZ_STREAM_END && rs != BZ_OK)
    return false;

  // Store the amount we actually read/wrote
  ncbIn -= impl->strm.avail_in;
  ncbOut -= impl->strm.avail_out;

  return true;
}

template<>
CompressionStream<BZip2>::~CompressionStream(void) {
  char buf[256];

  // Completed!  Finish writing anything that remains to be written, and keep going
  // as long as zlib fills up the proffered output buffer.
  for (;;) {
    impl->strm.avail_in = 0;
    impl->strm.next_in = nullptr;
    impl->strm.next_out = buf;
    impl->strm.avail_out = static_cast<unsigned int>(sizeof(buf));

    int ret = BZ2_bzCompress(&impl->strm, BZ_FINISH);
    switch (ret) {
    case BZ_STREAM_END:  // Return case when we are at the end
    case BZ_FINISH_OK:   // Return case when more data exists to be written

      // Handoff to lower level stream to complete the write
      os->Write(buf, sizeof(buf) - impl->strm.avail_out);

      if (ret == BZ_STREAM_END)
        // Clean return
        return;
      break;
    default:
      // Something went wrong, and we can't throw from here, so we just have to give up
      return;
    }
  }
}

template<>
bool CompressionStream<BZip2>::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) {
  impl->strm.avail_in = static_cast<unsigned int>(ncbIn);
  impl->strm.next_in = const_cast<char*>(reinterpret_cast<const char*>(input));

  // Compress one, update results
  impl->strm.next_out = reinterpret_cast<char*>(output);
  impl->strm.avail_out = static_cast<unsigned int>(ncbOut);
  int rs;
  do {
    rs = BZ2_bzCompress(&impl->strm, flush ? BZ_FLUSH : BZ_RUN);
  } while (rs == BZ_FLUSH_OK && impl->strm.avail_out > 0);
  ncbIn -= impl->strm.avail_in;
  ncbOut -= impl->strm.avail_out;
  return rs == BZ_RUN_OK;
}

}

// Explicit template specialization for the supported types:
// Explicit template specialization for the supported types:
template class leap::DecompressionStream<leap::Zlib>;
template class leap::CompressionStream<leap::Zlib>;
template class leap::DecompressionStream<leap::BZip2>;
template class leap::CompressionStream<leap::BZip2>;
