// Copyright (C) 2012-2018 Leap Motion, Inc. All rights reserved.
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
    InputStreamAdapter(std::istream& is);
    InputStreamAdapter(std::unique_ptr<std::istream> pis);
    InputStreamAdapter(const InputStreamAdapter& rhs);

    ~InputStreamAdapter(void);

  private:
    std::istream& is;

    // Non-null if we have taken ownership
    const std::unique_ptr<std::istream> pis;

  public:
    // IInputStream overrides:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
    std::streamsize Length(void) override;
    std::streampos Tell(void) override;
    void Clear(void) override;

    /// <summary>
    /// Sets the offset on the underlying input stream
    /// </summary>
    /// <returns>this</returns>
    InputStreamAdapter* Seek(std::streampos off) override;
  };

  class OutputStreamAdapter :
    public IOutputStream
  {
  public:
    OutputStreamAdapter(std::ostream& os);
    OutputStreamAdapter(std::unique_ptr<std::ostream> pos);
    OutputStreamAdapter(const OutputStreamAdapter& rhs);
    ~OutputStreamAdapter(void);

  private:
    std::ostream& os;

    // Non-null if we have taken ownership
    const std::unique_ptr<std::ostream> pos;

  public:
    // Accessor methods:
    std::ostream& GetStdStream(void) const { return os; }

    // IOutputStream overrides:
    bool Write(const void* pBuf, std::streamsize ncb) override;
  };
}
