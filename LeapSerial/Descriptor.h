// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "field_descriptor.h"
#include "field_serializer.h"
#include <initializer_list>
#include <unordered_map>
#include <vector>

namespace leap {
  struct descriptor;
  class IArchiveRegistry;
  class OArchiveRegistry;

  template<typename T, typename>
  struct field_serializer_t;

  /// <summary>
  /// Provides a generic way to describe serializable fields in a class
  /// </summary>
  struct descriptor:
    field_serializer
  {
    descriptor(const char* name, std::initializer_list<field_descriptor> field_descriptors) :
      descriptor(name, field_descriptors.begin(), field_descriptors.end())
    {}
    descriptor(std::initializer_list<field_descriptor> field_descriptors) :
      descriptor(nullptr, field_descriptors.begin(), field_descriptors.end())
    {}
    descriptor(const field_descriptor* begin, const field_descriptor* end) :
      descriptor(nullptr, begin, end)
    {}

    descriptor(const char* name, const field_descriptor* begin, const field_descriptor* end);

    // Allocation flag
    bool m_allocates = false;

    // Holds the descriptor's symbolic name, if one has been provided
    const char* const name = nullptr;

    // Required field descriptors
    std::vector<field_descriptor> field_descriptors;

    // Identified field descriptors
    std::unordered_map<uint64_t, field_descriptor> identified_descriptors;

    // field_serializer overrides:
    bool allocates(void) const override { return m_allocates; }
    serial_atom type(void) const override { return identified_descriptors.empty() ? serial_atom::finalized_descriptor : serial_atom::descriptor; }
    uint64_t size(const OArchiveRegistry& ar, const void* pObj) const override;
    void serialize(OArchiveRegistry& ar, const void* pObj) const override;
    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override;
  };
}
