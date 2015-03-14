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
    descriptor(std::initializer_list<field_descriptor> field_descriptors) :
      descriptor(field_descriptors.begin(), field_descriptors.end())
    {}

    descriptor(const field_descriptor* begin, const field_descriptor* end);

    // Allocation flag
    bool m_allocates = false;

    // Required field descriptors
    std::vector<field_descriptor> field_descriptors;

    // Identified field descriptors
    std::unordered_map<uint64_t, field_descriptor> identified_descriptors;

    // field_serializer overrides:
    bool allocates(void) const { return m_allocates; }
    serial_type type(void) const override { return serial_type::string; }
    uint64_t size(const void* pObj) const override;
    void serialize(OArchiveRegistry& ar, const void* pObj) const override;
    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override;
  };
}
