// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "CompressionStream.h"
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

}
