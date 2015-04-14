#pragma once
#include "Archive.h"
#include "field_serializer.h"
#include "serial_traits.h"
#include <mutex>
#include <unordered_map>

namespace leap {
  template<typename T, typename = void>
  struct field_serializer_t {
    static_assert(std::is_same<T, void>::value, "An attempt was made to serialize type T, which does not provide a serial_traits entry or GetDescriptor function");
  };

  template<typename T>
  struct serial_traits;

  // Objects that provide serial_traits should use those traits externally
  template<typename T>
  struct field_serializer_t<T, typename std::enable_if<!std::is_base_of<std::false_type, serial_traits<T>>::value>::type> :
    field_serializer
  {
    bool allocates(void) const override {
      return serializer_is_irresponsible<T>::value;
    }

    serial_primitive type(void) const override {
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

    static const field_serializer& GetDescriptor(void) {
      static const field_serializer_t m{};
      return m;
    }
  };

  template<typename T>
  struct field_serializer_t<void(T::*)()>:
    field_serializer
  {
    void(T::*pfn)();

    bool allocates(void) const override { return false; }

    serial_primitive type(void) const override {
      return serial_primitive::ignored;
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

    struct hash {
      size_t operator()(void(T::*pfn)()) const {
        return *reinterpret_cast<size_t*>(&pfn);
      }
    };

    static const field_serializer& GetDescriptor(void(T::*pfn)()) {
      static std::unordered_map<void(T::*)(), field_serializer_t, hash> mp;
      static std::mutex lock;

      std::lock_guard<std::mutex> lk(lock);
      auto& entry = mp[pfn];
      entry.pfn = pfn;
      return entry;
    }
  };
}