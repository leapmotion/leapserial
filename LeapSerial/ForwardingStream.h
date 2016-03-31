// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"

namespace leap {
  /// <summary>
  /// Input stream that relays operations to the held stream without taking ownership of it
  /// </summary>
  class ForwardingInputStream :
    public leap::IInputStream
  {
  public:
    ForwardingInputStream(leap::IInputStream& is) :
      is(is)
    {}

  private:
    leap::IInputStream& is;

  public:
    bool IsEof(void) const { return is.IsEof(); }
    std::streamsize Read(void* pBuf, std::streamsize ncb) { return is.Read(pBuf, ncb); }
    std::streamsize Skip(std::streamsize ncb) { return is.Skip(ncb); }
    std::streamsize Length(void) { return is.Length(); }
  };

  class ForwardingOutputStream :
    public leap::IOutputStream
  {
  public:
    ForwardingOutputStream(leap::IOutputStream& os) :
      os(os)
    {}

  private:
    leap::IOutputStream& os;

  public:
    bool Write(const void* pBuf, std::streamsize ncb) override { return os.Write(pBuf, ncb); }
    CopyResult Write(IInputStream& is, void* scratch, std::streamsize ncbScratch, std::streamsize& ncb) override { return os.Write(is, scratch, ncbScratch, ncb); }
    void Flush(void) override { os.Flush(); }
  };
}
