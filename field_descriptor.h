#pragma once
#include "base.h"
#include "field_serializer_t.h"

namespace leap {
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

    template<typename Base, typename Derived>
    field_descriptor(base<Base, Derived>):
      identifier(0),
      offset(
        static_cast<int>(
          reinterpret_cast<uint64_t>(
            static_cast<Base*>(reinterpret_cast<Derived*>(1))
          ) - 1
        )
      ),
      serializer(field_serializer_t<Base, void>::GetDescriptor())
    {}

    // Serializer interface, actually implements our serialization operation
    const field_serializer& serializer;

    // A numeric identifier for this field.  If this value is zero, the field is
    // required and strictly positional.
    int identifier;

    // The offset in type U where this field is located
    size_t offset;
  };
}