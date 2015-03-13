#pragma once
#include "Archive.h"
#include <map>
#include <string>
#include <unordered_map>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace leap {
  template<typename T, typename>
  struct field_serializer_t;

  template<typename T, typename = void>
  struct primitive_serial_traits:
    std::false_type
  {};

  // Create/delete structure used to describe how to allocate and free a memory block
  struct create_delete {
    void* (*pfnAlloc)();
    void (*pfnFree)(void*);
  };

  namespace internal {
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
    /// Holds true if the type "T" will always serialize to the same size
    /// </summary>
    /// <remarks>
    /// This method consults the serial traits for the specified type in order to come to its conclusion.
    /// For a given type T, if serial_traits<T>::constant_size is defined and nonzero, the corresponding
    /// type is inferred to have a constant size.
    /// </remarks>
    template<typename T, typename = void>
    struct is_constant_size:
      std::false_type
    {
      static const size_t size = ~0;
    };

    template<typename T>
    struct is_constant_size<T, typename std::enable_if<T::constant_size>::type>:
      std::true_type
    {
      static const size_t size = T::constant_size;
    };
  }

  // Embedded object types should use their corresponding descriptors
  template<typename T>
  struct primitive_serial_traits<T, typename std::enable_if<internal::has_getdescriptor<T>::value>::type>
  {
    // Trivial serialization/deserialization operations
    static uint64_t size(const T& obj) {
      return GetDescriptor().size(&obj);
    }

    static void serialize(OArchive& ar, const T& obj) {
      GetDescriptor().serialize(ar, &obj);
    }

    static void deserialize(IArchive& ar, T& obj, uint64_t ncb) {
      GetDescriptor().deserialize(ar, &obj, ncb);
    }

    // GetDescriptor is defined for our type, we can invoke it
    static const descriptor& GetDescriptor(void) {
      static const descriptor desc = T::GetDescriptor();
      return desc;
    }
  };

  // Specialization for anything that is a floating-point type.  These can be written directly to disk,
  // so we don't have to perform any kind of translation.  On big-endian systems, we will need to
  // perform byte order conversions.
  template<typename T>
  struct primitive_serial_traits<T, typename std::enable_if<std::is_floating_point<T>::value>::type>
  {
    static const size_t constant_size = sizeof(T);

    // Trivial serialization/deserialization operations
    static uint64_t size(T) {
      return sizeof(T);
    }

    static void serialize(OArchive& ar, const T& obj) {
      ar.Write(&obj, sizeof(T));
    }

    static void deserialize(IArchive& ar, T& obj, uint64_t ncb) {
      ar.Read(&obj, sizeof(T));
    }
  };

  // Specialization for anything that is integral.  This is already platform-independent and
  // makes use of the varint facility in the Archive type.
  template<typename T>
  struct primitive_serial_traits<T, typename std::enable_if<std::is_integral<T>::value>::type>
  {
    static const size_t constant_size = sizeof(T);

    // Trivial serialization/deserialization operations
    static uint64_t size(T val) {
      return OArchive::VarintSize(val);
    }

    static void serialize(OArchive& ar, T val) {
      ar.WriteVarint(val);
    }

    static void deserialize(IArchive& ar, T& val, uint64_t ncb) {
      val = static_cast<T>(ar.ReadVarint());
    }
  };

  template<typename T>
  struct serial_traits:
    primitive_serial_traits<T, void>
  {};

  // If we have a pointer to another object, we have to write an ID out and not the entire object
  // The object will be serialized at some other time
  template<typename T>
  struct serial_traits<T*>
  {
    static const size_t constant_size = sizeof(uint32_t);
    static uint64_t size(const T*const) { return sizeof(uint32_t); }

    static void serialize(OArchive& ar, const T* pObj) {
      // We need an ID for this object.  The RegisterObject method provides us with a way to do
      // this, but we also need to give this method the descriptor to be used to serialize the
      // object that we're pointed to right now.
      uint32_t objId = ar.RegisterObject(
        field_serializer_t<T, void>::GetDescriptor(),
        pObj
      );
      ar.Write(&objId, sizeof(objId));
    }

    static void deserialize(IArchive& ar, T*& pObj, uint64_t ncb) {
      static const create_delete cd {
        []() -> void* { return new T; },
        [](void* ptr) { delete (T*) ptr; }
      };

      // We expect to find an ID in the intput stream
      uint32_t objId;
      ar.Read(&objId, sizeof(objId));

      // Now we just perform a lookup into our archive and store the result here
      pObj = (T*)ar.Lookup(
        cd,
        field_serializer_t<T, void>::GetDescriptor(),
        objId
      );
    }
  };

  template<typename T, int N>
  struct serial_traits<T[N]>
  {
    static uint64_t size(const T* pObj) {
      return 0;
    }

    static void serialize(OArchive& ar, const T* pObj) {
      for (int i = 0; i < N; i++)
        serial_traits<T>::serialize(ar, pObj[i]);
    }

    static void deserialize(IArchive& ar, T* pObj, uint64_t ncb) {
      for (int i = 0; i < N; i++)
        serial_traits<T>::deserialize(ar, pObj[i], 0);
    }
  };

  // Convenience specialization for std::vector
  template<typename T, typename Alloc>
  struct serial_traits<std::vector<T, Alloc>>
  {
    static uint64_t size(const std::vector<T>& v) {
      if (internal::is_constant_size<T>::value)
        // Constant-sized element types have a simple equation to compute total size
        return sizeof(uint32_t) + v.size() * sizeof(T);

      // More complex types require summation on a per-element basis
      uint64_t retVal = sizeof(uint32_t);
      for (const auto& cur : v)
        retVal += serial_traits<T>::size(cur);
      return retVal;
    }

    static void serialize(OArchive& ar, const std::vector<T>& v) {
      // Write the number of entries first:
      uint32_t nEntries = static_cast<uint32_t>(v.size());
      ar.Write(&nEntries, sizeof(nEntries));

      // Write each entry out, one at a time, using the preferred serializer
      for (const auto& cur : v)
        serial_traits<T>::serialize(ar, cur);
    }

    static void deserialize(IArchive& ar, std::vector<T>& v, uint64_t ncb) {
      // Read the number of entries first:
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));

      // Now loop until we get the desired number of entries from the stream
      v.resize(nEntries);
      for (auto& cur : v)
        serial_traits<T>::deserialize(ar, cur, 0);
    }
  };

  template<typename T>
  struct serial_traits<std::basic_string<T, std::char_traits<T>, std::allocator<T>>>
  {
    typedef std::basic_string<T, std::char_traits<T>, std::allocator<T>> type;

    static uint64_t size(const type& pObj) {
      return sizeof(uint32_t) + pObj.size() * sizeof(T);
    }

    static void serialize(OArchive& ar, const type& obj) {
      // String length, then the string proper
      uint32_t nEntries = static_cast<uint32_t>(obj.size());
      ar.Write(&nEntries, sizeof(nEntries));
      ar.Write(obj.c_str(), nEntries * sizeof(T));
    }

    static void deserialize(IArchive& ar, type& obj, uint64_t ncb) {
      // Same story--string length, then string proper
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));
      obj.resize(nEntries);
      ar.Read(&obj[0], nEntries * sizeof(T));
    }
  };

  /// <summary>
  /// Mix-in type for all key/value container types
  /// </summary>
  template<typename Container>
  struct serial_traits_map_t
  {
    static uint64_t size(const Container& obj) {
      // Sum up all sizes for all child objects
      uint64_t retVal = sizeof(uint32_t);
      for (const auto& cur : obj)
        retVal +=
          serial_traits<typename Container::key_type>::size(cur.first) +
          serial_traits<typename Container::mapped_type>::size(cur.second);
      return retVal;
    }

    static void serialize(OArchive& ar, const Container& obj) {
      // Write the number of entries first:
      uint32_t nEntries = static_cast<uint32_t>(obj.size());
      ar.Write(&nEntries, sizeof(nEntries));

      // Write out each key/value pair, one at a time
      for (const auto& cur : obj) {
        serial_traits<typename Container::key_type>::serialize(ar, cur.first);
        serial_traits<typename Container::mapped_type>::serialize(ar, cur.second);
      }
    }

    static void deserialize(IArchive& ar, Container& obj, uint64_t ncb) {
      // Read the number of entries first:
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));

      // Now read in all values:
      while (nEntries--) {
        typename Container::key_type key;
        serial_traits<typename Container::key_type>::deserialize(ar, key, 0);
        serial_traits<typename Container::mapped_type>::deserialize(ar, obj[key], 0);
      }
    }
  };

  // Associative array types:
  template<typename Key, typename Value>
  struct serial_traits<std::map<Key, Value>> :
    serial_traits_map_t<std::map<Key, Value>>
  {};

  template<typename Key, typename Value>
  struct serial_traits<std::unordered_map<Key, Value>> :
    serial_traits_map_t<std::unordered_map<Key, Value>>
  {};
}