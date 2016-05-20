// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include "ArchiveLeapSerial.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class IArchiveLeapSerialV0 :
    public IArchiveLeapSerial
  {
  public:
    IArchiveLeapSerialV0(IInputStream& is);
    IArchiveLeapSerialV0(std::istream& is);

  public:
    void ReadArray(IArrayAppender&& ary) override;
  };

  class OArchiveLeapSerialV0 :
    public OArchiveLeapSerial
  {
  public:
    OArchiveLeapSerialV0(IOutputStream& os);

  private:
    void WriteArray(IArrayReader&& ary) override;
    uint64_t SizeArray(IArrayReader&& ary) const override;
  };
}
