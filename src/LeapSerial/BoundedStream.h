// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"
#include <memory>

namespace leap {
  class BoundedInputStream :
    public IInputStream
  {
  public:
    BoundedInputStream(std::unique_ptr<IInputStream>&& is, std::streamsize readLimit);
    ~BoundedInputStream(void);

    // Limit to the number of bytes that may be read
    std::streamsize ReadLimit = 0;

  private:
    std::unique_ptr<IInputStream> is;

    // Total number of bytes currently read
    std::streamsize m_totalRead = 0;

  public:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
    std::streamsize Length(void) override;
  };

  class BoundedOutputStream :
    public IOutputStream
  {
  public:
    BoundedOutputStream(std::unique_ptr<IOutputStream>&& os, std::streamsize writeLimit);

    // Limit to the number of bytes that may be written
    std::streamsize WriteLimit = 0;

  private:
    std::unique_ptr<IOutputStream> os;
    std::streamsize m_totalWritten = 0;

  public:
    bool Write(const void* pBuf, std::streamsize ncb) override;
    void Flush(void) override;
  };
}
