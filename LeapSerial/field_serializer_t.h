// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include "field_serializer.h"
#include "serial_traits.h"
#include <mutex>
#include <unordered_set>

namespace leap {
  template<typename T, typename = void>
  struct field_serializer_t {
    static_assert(std::is_same<T, void>::value, "An attempt was made to serialize type T, which does not provide a serial_traits entry or GetDescriptor function");
  };

  template<typename T>
  struct serial_traits;

  template<typename T>
  struct has_serializer;

  template<typename T, typename = void>
  struct field_serializer_base_t :
    field_serializer
  {};

  template<typename T>
  struct field_serializer_base_t<T, typename std::enable_if<serial_traits<T>::is_array>::type> :
    field_serializer_array
  {
    const field_serializer& element(void) const override { return field_serializer_t<typename serial_traits<T>::value_type>::GetDescriptor(); }
  };

  template<typename T>
  struct field_serializer_base_t<T, typename std::enable_if<serial_traits<T>::is_map>::type> :
    field_serializer_map
  {
    const field_serializer& key(void) const override { return field_serializer_t<typename serial_traits<T>::key_type>::GetDescriptor(); }
    const field_serializer& mapped(void) const override { return field_serializer_t<typename serial_traits<T>::mapped_type>::GetDescriptor(); }
  };

  template<typename T>
  struct field_serializer_base_t<T, typename std::enable_if<serial_traits<T>::is_object>::type> :
    field_serializer_object
  {
    const descriptor& object(void) const override { return serial_traits<T>::get_descriptor(); }
  };

  // Objects that provide serial_traits should use those traits externally
  template<typename T>
  struct field_serializer_t<T, typename std::enable_if<!std::is_base_of<std::false_type, serial_traits<T>>::value>::type> :
    field_serializer_base_t<T, void>
  {
  private:
    // Singleton pattern, do not use this routine, use GetDescriptor instead
    field_serializer_t(void) {}

  public:
    bool allocates(void) const override {
      return serializer_is_irresponsible<T>::value;
    }

    serial_atom type(void) const override {
      return serial_traits<T>::type();
    }

    uint64_t size(const OArchiveRegistry& ar, const void* pObj) const override {
      return serial_traits<T>::size(ar, *static_cast<const T*>(pObj));
    }

    void serialize(OArchiveRegistry& ar, const void* pObj) const override {
      serial_traits<T>::serialize(ar, *static_cast<const T*>(pObj));
    }

    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override {
      serial_traits<T>::deserialize(ar, *static_cast<T*>(pObj), ncb);
    }

    bool is_optional(void) const override { return leap::is_optional<T>::value; }

    static const field_serializer& GetDescriptor(void) {
      static const field_serializer_t m{};
      return m;
    }
  };

  template<typename T>
  struct mem_hash {
    size_t operator()(const T& obj) const {
      size_t retVal = 0;
      for (size_t i = 0; i < sizeof(obj); i++)
        retVal += retVal * 101 + reinterpret_cast<const char*>(&obj)[i];
      return retVal;
    }
  };

  template<typename T>
  struct field_serializer_t<void(T::*)()>:
    field_serializer
  {
    field_serializer_t(void(T::*pfn)()):
      pfn(pfn)
    {}

    void(T::*pfn)();

    bool allocates(void) const override { return false; }

    serial_atom type(void) const override {
      return serial_atom::ignored;
    }

    uint64_t size(const OArchiveRegistry& ar, const void* pObj) const override {
      return 0;
    }

    void serialize(OArchiveRegistry& ar, const void* pObj) const override {
      // Does nothing
    }

    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override {
      (static_cast<T*>(pObj)->*pfn)();
    }

    bool is_optional(void) const override { return true; }

    bool operator==(const field_serializer_t& rhs) const {
      return pfn == rhs.pfn;
    }

    static const field_serializer& GetDescriptor(void(T::*pfn)()) {
      static std::unordered_set<field_serializer_t, mem_hash<field_serializer_t>> st;
      static std::mutex lock;

      std::lock_guard<std::mutex> lk(lock);
      field_serializer_t key{pfn};
      auto q = st.find(key);
      if (q == st.end())
        q = st.insert(q, key);
      return *q;
    }
  };

  template<typename T, typename U, typename V>
  struct field_getter_setter{};

  template<typename T, typename U, typename V>
  struct field_serializer_t <
    field_getter_setter<T, U, V>,
    typename std::enable_if<
      std::is_same<
        typename std::decay<U>::type,
        typename std::decay<V>::type
      >::value
    >::type
  > :
    field_serializer
  {
    typedef typename std::decay<U>::type field_type;

    field_serializer_t(U(T::*pfnGetter)() const, void(T::*pfnSetter)(V)):
      pfnGetter(pfnGetter),
      pfnSetter(pfnSetter)
    {}

    U(T::*pfnGetter)() const;
    void(T::*pfnSetter)(V);

    bool allocates(void) const override { return false; }

    serial_atom type(void) const override {
      return serial_traits<field_type>::type();
    }

    uint64_t size(const OArchiveRegistry& ar, const void* pObj) const override {
      return serial_traits<field_type>::size(ar, (static_cast<const T*>(pObj)->*pfnGetter)());
    }

    void serialize(OArchiveRegistry& ar, const void* pObj) const override {
      serial_traits<field_type>::serialize(ar, (static_cast<const T*>(pObj)->*pfnGetter)());
    }

    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override {
      field_type val;
      serial_traits<field_type>::deserialize(ar, val, ncb);
      (static_cast<T*>(pObj)->*pfnSetter)(val);
    }

    bool is_optional(void) const override { return true; }

    bool operator==(const field_serializer_t& rhs) const {
      return pfnGetter == rhs.pfnGetter && pfnSetter == rhs.pfnSetter;
    }

    static const field_serializer& GetDescriptor(U(T::*getter)() const, void(T::*setter)(V)) {
      static std::unordered_set<field_serializer_t, mem_hash<field_serializer_t>> st;
      static std::mutex lock;

      std::lock_guard<std::mutex> lk(lock);
      field_serializer_t key{getter, setter};
      auto q = st.find(key);
      if (q == st.end())
        q = st.insert(q, key);
      return *q;
    }
  };

  template<typename T, typename U, typename V>
  struct field_getter_setter_extern{};
 
  template<typename T, typename U, typename V>
  struct field_serializer_t <
    field_getter_setter_extern<T, U, V>,
    typename std::enable_if<
      std::is_same<
        typename std::decay<U>::type,
        typename std::decay<V>::type
      >::value
    >::type
  > :
    field_serializer
  {
    typedef typename std::decay<U>::type field_type;
 
    field_serializer_t(U(*pfnGetter)(const T&), void(*pfnSetter)(T&,V)) :
      pfnGetter(pfnGetter),
      pfnSetter(pfnSetter)
    {}
 
    U(*pfnGetter)(const T&);
    void(*pfnSetter)(T&, V);
 
    bool allocates(void) const override { return false; }
 
    serial_atom type(void) const override {
      return serial_traits<field_type>::type();
    }
 
    uint64_t size(const OArchiveRegistry& ar, const void* pObj) const override {
      return serial_traits<field_type>::size(ar, pfnGetter(*static_cast<const T*>(pObj)));
    }
 
    void serialize(OArchiveRegistry& ar, const void* pObj) const override {
      serial_traits<field_type>::serialize(ar, pfnGetter(*static_cast<const T*>(pObj)));
    }
 
    void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const override {
      field_type val;
      serial_traits<field_type>::deserialize(ar, val, ncb);
      pfnSetter(*static_cast<T*>(pObj),val);
    }

    bool is_optional(void) const override { return true; }
 
    bool operator==(const field_serializer_t& rhs) const {
      return pfnGetter == rhs.pfnGetter && pfnSetter == rhs.pfnSetter;
    }
 
    static const field_serializer& GetDescriptor(U(*getter)(const T&), void(*setter)(T&,V)) {
      static std::unordered_set<field_serializer_t, mem_hash<field_serializer_t>> st;
      static std::mutex lock;
 
      std::lock_guard<std::mutex> lk(lock);
      field_serializer_t key{ getter, setter };
      auto q = st.find(key);
      if (q == st.end())
        q = st.insert(q, key);
      return *q;
    }
  };
}