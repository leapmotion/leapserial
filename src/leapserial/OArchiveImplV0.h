// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include "OArchiveImpl.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class OArchiveImplV0:
    public OArchiveImpl
  {
  public:
    OArchiveImplV0(IOutputStream& os);

  private:
    void WriteArray(IArrayReader&& ary) override;
    uint64_t SizeArray(IArrayReader&& ary) const override;
  };
}
