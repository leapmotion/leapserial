// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include "ArchiveLeapSerial.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class IArchiveImplV0 :
    public IArchiveLeapSerial
  {
  public:
    IArchiveImplV0(IInputStream& is);
    IArchiveImplV0(std::istream& is);

  public:
    void ReadArray(IArrayAppender&& ary) override;
  };

  class ArchiveLeapSerialV0:
    public OArchiveLeapSerial
  {
  public:
    ArchiveLeapSerialV0(IOutputStream& os);

  private:
    void WriteArray(IArrayReader&& ary) override;
    uint64_t SizeArray(IArrayReader&& ary) const override;
  };
}
