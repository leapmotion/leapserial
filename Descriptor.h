#pragma once
#include "Archive.h"
#include "field_serializer.h"
#include <initializer_list>
#include <iosfwd>
#include <unordered_map>
#include <vector>

namespace leap {
  struct descriptor;

  template<typename T, typename>
  struct field_serializer_t;

  /// <summary>
  /// A descriptor which describes serialization operations for a single datatype
  /// </summary>
  /// <remarks>
  /// If this descriptor describes an object, it may potentially be recursive
  /// </remarks>
  struct field_descriptor {
    template<typename U, typename T>
    field_descriptor(int identifier, T U::*val) :
      identifier(identifier),
      offset(reinterpret_cast<size_t>(&(static_cast<U*>(nullptr)->*val))),
      serializer(field_serializer_t<T, void>::GetDescriptor())
    {}

    template<typename U>
    field_descriptor(void (U::*pMemfn)()) :
      identifier(0),
      offset(0),
      serializer(field_serializer_t<void(U::*)(), void>::GetDescriptor(pMemfn))
    {
    }

    template<typename U, typename T>
    field_descriptor(T U::*val) :
      field_descriptor(0, val)
    {}

    // Serializer interface, actually implements our serialization operation
    const field_serializer& serializer;

    // A numeric identifier for this field.  If this value is zero, the field is
    // required and strictly positional.
    int identifier;

    // The offset in type U where this field is located
    size_t offset;
  };

  struct descriptor:
    field_serializer
  {
    descriptor(std::initializer_list<field_descriptor> field_descriptors);

    // Required field descriptors
    std::vector<field_descriptor> field_descriptors;

    // Identified field descriptors
    std::unordered_map<uint64_t, field_descriptor> identified_descriptors;

    // field_serializer overrides:
    serial_type type(void) const override { return serial_type::string; }
    uint64_t size(const void* pObj) const override;
    void serialize(OArchive& ar, const void* pObj) const override;
    void deserialize(IArchive& ar, void* pObj, uint64_t ncb) const override;
  };
}
