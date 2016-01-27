// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "CompressionStream.h"
#include <algorithm>
#include <memory.h>
#include <zlib/zlib.h>

using namespace leap;

ZStreamBase::ZStreamBase(void) :
  strm{ new z_stream }
{
  strm->zalloc = nullptr;
  strm->zfree = nullptr;
  strm->opaque = nullptr;
}

ZStreamBase::ZStreamBase(ZStreamBase&& rhs) :
  strm(std::move(rhs.strm))
{}

ZStreamBase::~ZStreamBase(void) {
  if(strm)
    deflateEnd(strm.get());
}

DecompressionStream::DecompressionStream(IInputStream& is) :
  InputFilterStreamBase(is)
{
  inflateInit(strm.get());
  strm->avail_in = 0;
  strm->next_in = nullptr;
}

bool DecompressionStream::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut) {
  // Decompress into our buffer space
  strm->next_out = reinterpret_cast<uint8_t*>(output);
  strm->avail_out = ncbOut;
  strm->next_in = reinterpret_cast<const uint8_t*>(input);
  strm->avail_in = ncbIn;
  if (inflate(strm.get(), Z_NO_FLUSH) == Z_STREAM_ERROR)
    return false;

  // Store the amount we actually read/wrote
  ncbIn -= strm->avail_in;
  ncbOut -= strm->avail_out;
  return true;
}

CompressionStream::CompressionStream(IOutputStream& os, int level) :
  OutputFilterStreamBase(os)
{
  if (level < -1 || 9 < level)
    throw std::invalid_argument("Compression stream level must be in the range [0, 9]");

  deflateInit(strm.get(), level);
}

CompressionStream::~CompressionStream(void) {
  Flush();
}

bool CompressionStream::Transform(const void* input, size_t& ncbIn, void* output, size_t& ncbOut, bool flush) {
  strm->avail_in = static_cast<uint32_t>(ncbIn);
  strm->next_in = reinterpret_cast<const uint8_t*>(input);

  // Compress one, update results
  strm->next_out = reinterpret_cast<uint8_t*>(output);
  strm->avail_out = ncbOut;
  int rs = deflate(strm.get(), flush ? Z_FULL_FLUSH : Z_NO_FLUSH);

  ncbIn -= strm->avail_in;
  ncbOut -= strm->avail_out;
  return rs == Z_OK;
}
