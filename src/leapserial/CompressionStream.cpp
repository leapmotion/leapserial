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
  is(is),
  buffer(1024, 0),
  inputChunk(1024, 0)
{
  inflateInit(strm.get());
  strm->avail_in = 0;
  strm->next_in = nullptr;
}

bool DecompressionStream::IsEof(void) const {
  return is.IsEof() && buffer.empty();
}

std::streamsize DecompressionStream::Read(void* pBuf, std::streamsize ncb) {
  if (fail)
    throw std::runtime_error("Cannot read, stream corrupt");

  std::streamsize total = 0;
  while (ncb) {
    // If there are any data bytes available in holding, prefer to return those:
    if (ncbAvail) {
      // Decide on how many bytes to move, then move those over
      size_t ncbCopy = std::min(ncbAvail, static_cast<size_t>(ncb));
      memcpy(pBuf, &*(buffer.end() - ncbAvail), ncbCopy);
      total += ncbCopy;

      // Reduce by the number of bytes we moved
      reinterpret_cast<uint8_t*&>(pBuf) += ncbCopy;
      ncbAvail -= ncbCopy;
      ncb -= ncbCopy;
      continue;
    }

    // Pump in from the underlying stream if it's empty
    if (!strm->avail_in) {
      auto nRead = is.Read(inputChunk.data(), inputChunk.size());
      if (nRead < 0) {
        fail = true;
        throw std::runtime_error("Unexpected end of file reached");
      }
      if (nRead == 0)
        // Maybe EOF, short-circuit
        return total;

      strm->avail_in = static_cast<uint32_t>(nRead);
      strm->next_in = inputChunk.data();
    }

    // Decompress into our buffer space
    buffer.resize(buffer.capacity());
    strm->avail_out = buffer.size();
    strm->next_out = buffer.data();
    if (inflate(strm.get(), Z_NO_FLUSH) == Z_STREAM_ERROR) {
      fail = true;
      return -1;
    }
    ncbAvail = strm->next_out - buffer.data();
    buffer.resize(ncbAvail);
  }

  return total;
}

std::streamsize DecompressionStream::Skip(std::streamsize ncb) {
  return 0;
}

CompressionStream::CompressionStream(IOutputStream& os, int level) :
  os(os),
  buffer(1024, 0)
{
  if (level < -1 || 9 < level)
    throw std::invalid_argument("Compression stream level must be in the range [0, 9]");

  deflateInit(strm.get(), level);
}

CompressionStream::~CompressionStream(void) {
  if (!fail)
    Write(nullptr, 0, Z_FULL_FLUSH);
}

bool CompressionStream::Write(const void* pBuf, std::streamsize ncb, int flushFlag) {
  if (fail)
    throw std::runtime_error("Cannot write if compression stream is in a failed state");

  strm->avail_in = static_cast<uint32_t>(ncb);
  strm->next_in = reinterpret_cast<const uint8_t*>(pBuf);

  // Pump the compression routine until we are done
  do {
    strm->avail_out = buffer.size();
    strm->next_out = buffer.data();
    int ret = deflate(strm.get(), flushFlag);

    fail = !os.Write(buffer.data(), strm->next_out - buffer.data());
    if (fail)
      return false;
  } while (!strm->avail_out);

  return true;
}

bool CompressionStream::Write(const void* pBuf, std::streamsize ncb) {
  return Write(pBuf, ncb, Z_NO_FLUSH);
}

void CompressionStream::Flush(void) {
  if (fail)
    return;

  Write(nullptr, 0, Z_PARTIAL_FLUSH);
}
