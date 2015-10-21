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
  struct primitive_serial_traits;

  template<typename T, typename>
  struct field_serializer_t;

  /// <summary>
  /// Holds "true" if T::GetDescriptor exists
  /// </summary>
  template<class T>
  struct has_getdescriptor {
    template<class U>
    static std::true_type select(decltype(U::GetDescriptor)*);

    template<class U>
    static std::false_type select(...);

    static const bool value = decltype(select<T>(nullptr))::value;
  };

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

  // Embedded object types should use their corresponding descriptors
  template<typename T>
  struct primitive_serial_traits<T, typename std::enable_if<has_getdescriptor<T>::value>::type>
  {
    static const bool is_object = true;

    static ::leap::serial_atom type() {
      return get_descriptor().type();
    }

    // Trivial serialization/deserialization operations
    static uint64_t size(const OArchiveRegistry& ar, const T& obj) {
      return get_descriptor().size(ar, &obj);
    }

    static void serialize(OArchiveRegistry& ar, const T& obj) {
      get_descriptor().serialize(ar, &obj);
    }

    static void deserialize(IArchiveRegistry& ar, T& obj, uint64_t ncb) {
      get_descriptor().deserialize(ar, &obj, ncb);
    }

    // GetDescriptor is defined for our type, we can invoke it
    static const descriptor& get_descriptor(void) {
      static const descriptor desc = T::GetDescriptor();
      return desc;
    }
  };
}
