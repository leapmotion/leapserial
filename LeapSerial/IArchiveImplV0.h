// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IArchiveImpl.h"

namespace leap {
  class IArchiveImplV0:
    public IArchiveImpl
  {
  public:
    IArchiveImplV0(IInputStream& is);
    IArchiveImplV0(std::istream& is);

  public:
    void ReadArray(IArrayAppender&& ary) override;
  };
}
