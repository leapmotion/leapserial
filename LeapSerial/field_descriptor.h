// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "base.h"
#include "field_serializer_t.h"

namespace leap {
  template<typename T, typename>
  struct field_serializer_t;

  template<typename T>
  struct has_serializer;

  template<typename T, typename U, typename V>
  struct field_getter_setter;

  template<typename T, typename U, typename V>
  struct field_getter_setter_extern;

  template<typename MemFn>
  struct decompose {
    static const bool is_getter = false;
    static const bool is_setter = false;
  };

  // Getter functor signature
  template<typename T, typename RetVal, typename Base>
  struct decompose<RetVal(T::*)(const Base&) const> {
    typedef RetVal ret;
    typedef Base base;
    static const bool is_getter = true;
    static const bool is_setter = false;
  };

  // Setter functor signature
  template<typename T, typename Base, typename Arg>
  struct decompose<void(T::*)(Base, Arg) const> {
    typedef void ret;
    typedef Base base;
    typedef Arg arg;
    static const bool is_getter = false;
    static const bool is_setter = true;
  };

  /// <summary>
  /// Extractor for obtaining traits about a curried candidate accessor functor
  /// </summary>
  template<typename Fn, typename = void>
  struct getter {
    static const bool value = false;
  };

  // Function object
  template<typename Fn>
  struct getter<Fn, typename std::enable_if<std::is_class<Fn>::value>::type> :
    decompose<decltype(&Fn::operator())>
  {
    typedef decltype(&Fn::operator()) t_fnType;
    typedef int(*signature)(const typename decompose<t_fnType>::base&);
    static const bool value = true;
  };

  // Function pointer proper
  template<typename Ret, typename Base>
  struct getter<Ret(*)(const Base&), void>
  {
    typedef Ret ret;
    typedef Base base;
    typedef Ret(*signature)(const Base&);
    static const bool value = true;
  };

  // Function reference
  template<typename Ret, typename Base>
  struct getter<Ret(&)(const Base&), void>:
    getter<Ret(*)(const Base&), void>
  {};

  // Reference to a function pointer
  template<typename Ret, typename Base>
  struct getter<Ret(*&)(const Base&), void> :
    getter<Ret(*)(const Base&), void>
  {};

  /// <summary>
  /// Extractor for obtaining traits about an uncurried mutator functor
  /// </summary>
  template<typename Fn, typename = void>
  struct setter {
    static const bool value = false;
  };

  // Function object
  template<typename Fn>
  struct setter<Fn, typename std::enable_if<std::is_class<Fn>::value>::type> :
    decompose<decltype(&Fn::operator())>
  {
    typedef decltype(&Fn::operator()) t_fnType;
    typedef void(*signature)(typename decompose<t_fnType>::base, int);
    static const bool value = decompose<t_fnType>::is_setter;
  };

  // Function pointer proper
  template<typename Base, typename Arg>
  struct setter<void (*)(Base, Arg), void>
  {
    typedef Base base;
    typedef Arg arg;
    typedef void(*signature)(base, int);
    static const bool value = true;
  };

  // Function reference
  template<typename Base, typename Arg>
  struct setter<void(&)(Base, Arg), void>:
    setter<void(*)(Base, Arg), void>
  {};

  // Reference to a function pointer
  template<typename Base, typename Arg>
  struct setter<void(*&)(Base, Arg), void>:
    setter<void(*)(Base, Arg), void>
  {};

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
    {
      // Instantiate serial_traits on the type object itself.  If T::GetDescriptor is provided,
      // this has the effect of ultimately instantiating descriptor_entry_t<T>.  Sometimes, though,
      // this member is not provided--for instance, when a descriptor includes a member of a base
      // type and that base type is not itself serializable.
      (void)&serial_traits<T>::type;
      static_assert(has_serializer<U>::value, "An attempt was made to serialize type U, which does not provide a serial_traits entry or GetDescriptor function");
    }

    template<typename T>
    field_descriptor(void (T::*pMemfn)()) :
      identifier(0),
      name(nullptr),
      offset(0),
      serializer(field_serializer_t<void(T::*)(), void>::GetDescriptor(pMemfn))
    {}

    //Member getter/setters
    template<typename T, typename U, typename V>
    field_descriptor(int identifier, const char* name, U(T::*pGetFn)() const, void(T::*pSetFn)(V)) :
      identifier(identifier),
      name(name),
      offset(0),
      serializer(field_serializer_t<field_getter_setter<T,U,V>, void>::GetDescriptor(pGetFn, pSetFn))
    {}

    /// <summary>
    /// A descriptor for a field with nonmember getter and setters
    /// </summary>
    /// <param name="getter">A getter lambda of the form fieldType(classType)</param>
    template<typename T, typename U>
    field_descriptor(
      int identifier,
      const char* name,
      T&& get,
      U&& set
    ) :
      identifier(identifier),
      name(name),
      offset(0),
      serializer(
        field_serializer_t<
          field_getter_setter_extern<
            typename getter<T>::base,
            typename getter<T>::ret,
            typename setter<U>::arg
          >,
          void
        >::GetDescriptor(get, set)
      )
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

    field_descriptor(const field_serializer& serializer, const char* name, int identifier, size_t offset) :
      serializer(serializer),
      name(name),
      identifier(identifier),
      offset(offset)
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
