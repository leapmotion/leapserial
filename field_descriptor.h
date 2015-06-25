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
    template<typename T, typename U>
    field_descriptor(int identifier, const char* name, U T::*val) :
      identifier(identifier),
      name(name),
      offset(reinterpret_cast<size_t>(&(static_cast<T*>(nullptr)->*val))),
      serializer(field_serializer_t<U, void>::GetDescriptor())
    {}

    template<typename T>
    field_descriptor(void (T::*pMemfn)()) :
      identifier(0),
      name(nullptr),
      offset(0),
      serializer(field_serializer_t<void(T::*)(), void>::GetDescriptor(pMemfn))
    {
    }

    //Member getter/setters
    template<typename T, typename U, typename V>
    field_descriptor(int identifier, const char* name, U(T::*pGetFn)() const, void(T::*pSetFn)(V)) :
      identifier(identifier),
      name(name),
      offset(0),
      serializer(field_serializer_t<field_getter_setter<T,U,V>>::GetDescriptor(pGetFn, pSetFn))
    {}
    
    //Non-Member getter/setters
    template<typename T, typename U, typename V>
    field_descriptor(int identifier, const char* name, U(*pGetFn)(const T&), void(*pSetFn)(T&,V)) :
      identifier(identifier),
      name(name),
      offset(0),
      serializer(field_serializer_t<field_getter_setter_extern<T, U, V>>::GetDescriptor(pGetFn, pSetFn))
    {}

    template<typename T, typename U>
    field_descriptor(U T::*val) :
      field_descriptor(0, val)
    {}

    template<typename T, typename U>
    field_descriptor(int identifier, U T::*val) :
      field_descriptor(identifier, nullptr, val)
    {}
    
    template<typename T, typename U, typename V>
    field_descriptor(int identifier, U(T::*pGetFn)() const, void(T::*pSetFn)(V)) :
      field_descriptor(identifier, nullptr, pGetFn, pSetFn)
    {}

    template<typename T, typename U, typename V>
    field_descriptor(int identifier, U(*pGetFn)(const T&), void(*pSetFn)(T&, V)) :
      field_descriptor(identifier, nullptr, pGetFn, pSetFn)
    {}

    template<typename T, typename U>
    field_descriptor(const char* name, U T::*val) :
      field_descriptor(0, name, val)
    {}
    
    template<typename T, typename U, typename V>
    field_descriptor(const char* name, U(T::*pGetFn)() const, void(T::*pSetFn)(V)) :
      field_descriptor(0, name, pGetFn, pSetFn)
    {}

    template<typename T, typename U, typename V>
    field_descriptor(const char* name, U(*pGetFn)(const T&), void(*pSetFn)(T&, V)) :
      field_descriptor(0, name, pGetFn, pSetFn)
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

    // A string identifier for this field. Used for serialization to formats which require them
    const char* name;

    // A numeric identifier for this field.  If this value is zero, the field is
    // required and strictly positional.
    int identifier;

    // The offset in type T where this field is located
    size_t offset;
  };
}