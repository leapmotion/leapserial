// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include <vector>
#include <utility>

namespace leap {
  struct create_delete;
}

namespace leap { namespace internal {
  class AllocationBase {
  public:
    AllocationBase(void);
    AllocationBase(AllocationBase&& rhs) = delete;
    void operator=(AllocationBase&& rhs) = delete;

    ~AllocationBase(void);

    // All of the allocated objects that will need to be garbage collected when we go out of scope,
    // together with the deleter for those types
    std::vector<std::pair<void*, void(*)(void*)>> garbageList;

    /// <summary>
    /// Retrieves a pointer to the so-called "root object," which should follow the allocation base immediately
    /// </summary>
    void* GetRoot(void);
  };

  /// <summary>
  /// Represents ownership over a block of memory.
  /// </summary>
  /// <remarks>
  /// During deserialization, a multitude of secondary allocations may be necessary to completely build
  /// out a general graph topology.  This structure is a collection of all of the allocations that were
  /// needed.  Callers to Deserialize must keep this structure resident until they are done with the
  /// deserialized data.
  /// </remarks>
  template<class T>
  class Allocation:
    public AllocationBase
  {
  public:
    Allocation(void) {}
    Allocation(Allocation&) = delete;
    Allocation(Allocation&& rhs) = delete;
    void operator=(Allocation&& rhs) = delete;

    T val;
  };
}}
