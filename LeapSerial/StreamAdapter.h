// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IInputStream.h"
#include "IOutputStream.h"
#include <iosfwd>
#include <memory>

namespace leap {
  /// <summary>
  /// Mapping adaptor, allows input streams to be wrapped to support Archive operations
  /// </summary>
  class InputStreamAdapter :
    public IInputStream
  {
  public:
    InputStreamAdapter(std::istream& is) :
      is(is)
    {}
    InputStreamAdapter(std::unique_ptr<std::istream> pis) :
      is(*pis),
      pis(std::move(pis))
    {}

    InputStreamAdapter(const InputStreamAdapter& rhs) :
      is(rhs.is)
    {}

  private:
    std::istream& is;

    // Non-null if we have taken ownership
    const std::unique_ptr<std::istream> pis;

  public:
    // IInputStream overrides:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;

    /// <summary>
    /// Sets the offset on the underlying input stream
    /// </summary>
    /// <returns>this</returns>
    InputStreamAdapter* Seek(std::streamsize off);
  };

  class OutputStreamAdapter :
    public IOutputStream
  {
  public:
    OutputStreamAdapter(std::ostream& os) :
      os(os)
    {}

    OutputStreamAdapter(std::unique_ptr<std::ostream> pos) :
      os(*pos),
      pos(std::move(pos))
    {}

    OutputStreamAdapter(const OutputStreamAdapter& rhs) :
      os(rhs.os)
    {}

    // IOutputStream overrides:
    bool Write(const void* pBuf, std::streamsize ncb) override;

  private:
    std::ostream& os;

    // Non-null if we have taken ownership
    const std::unique_ptr<std::ostream> pos;
  };
}
