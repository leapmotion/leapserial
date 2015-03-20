#pragma once
#include "Archive.h"
#include "Descriptor.h"
#include "field_serializer_t.h"
#include <array>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace leap {
  template<typename T>
  struct serial_traits;

  template<typename T, typename>
  struct field_serializer_t;

  template<typename T>
  struct serializer_is_irresponsible;

  template<typename T, typename = void>
  struct primitive_serial_traits:
    std::false_type
  {};

  // Create/delete structure used to describe how to allocate and free a memory block
  struct create_delete {
    void* (*pfnAlloc)();
    void(*pfnFree)(void*);
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

    static void serialize(OArchiveRegistry& ar, const T& obj) {
      GetDescriptor().serialize(ar, &obj);
    }

    static void deserialize(IArchiveRegistry& ar, T& obj, uint64_t ncb) {
      GetDescriptor().deserialize(ar, &obj, ncb);
    }

    // GetDescriptor is defined for our type, we can invoke it
    static const field_serializer& GetDescriptor(void) {
      static const auto desc = T::GetDescriptor();
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
  struct primitive_serial_traits<T, typename std::enable_if<std::is_enum<T>::value>::type>
  {
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
  {
  };

  // If we have a pointer to another object, we have to write an ID out and not the entire object
  // The object will be serialized at some other time
  template<typename T>
  struct serial_traits<T*>
  {
    static const size_t constant_size = sizeof(uint32_t);
    static uint64_t size(const T*const) { return sizeof(uint32_t); }

    static void serialize(OArchiveRegistry& ar, const T* pObj) {
      // We need an ID for this object.  The RegisterObject method provides us with a way to do
      // this, but we also need to give this method the descriptor to be used to serialize the
      // object that we're pointed to right now.
      uint32_t objId = ar.RegisterObject(
        field_serializer_t<T, void>::GetDescriptor(),
        pObj
      );
      ar.Write(&objId, sizeof(objId));
    }

    static void deserialize(IArchiveRegistry& ar, T*& pObj, uint64_t ncb) {
      // We expect to find an ID in the intput stream
      uint32_t objId;
      ar.Read(&objId, sizeof(objId));

      // Now we just perform a lookup into our archive and store the result here
      pObj = (T*)ar.Lookup(
        {
          []() -> void* { return new T; },
          [](void* ptr) { delete (T*) ptr; }
        },
        field_serializer_t<T, void>::GetDescriptor(),
        objId
      );
    }
  };

  template<typename T, size_t N>
  struct serial_traits<T[N]>
  {
    static uint64_t size(const T* pObj) {
      return 0;
    }

    static void serialize(OArchiveRegistry& ar, const T* pObj) {
      for (int i = 0; i < N; i++)
        serial_traits<T>::serialize(ar, pObj[i]);
    }

    static void deserialize(IArchiveRegistry& ar, T* pObj, uint64_t ncb) {
      for (int i = 0; i < N; i++)
        serial_traits<T>::deserialize(ar, pObj[i], 0);
    }
  };
  
  template<typename T, size_t N>
  struct serial_traits<std::array<T,N>>
  {
    static uint64_t size(const std::array<T, N>& v) {
      if (internal::is_constant_size<T>::value)
        // Constant-sized element types have a simple equation to compute total size
        return sizeof(uint32_t) + v.size() * sizeof(T);
      
      // More complex types require summation on a per-element basis
      uint64_t retVal = sizeof(uint32_t);
      for (const auto& cur : v)
        retVal += serial_traits<T>::size(cur);
      return retVal;
    }
    
    static void serialize(OArchiveRegistry& ar, const std::array<T, N>& v) {
      // Write the number of entries first:
      uint32_t nEntries = static_cast<uint32_t>(v.size());
      ar.Write(&nEntries, sizeof(nEntries));
      
      // Write each entry out, one at a time, using the preferred serializer
      for (const auto& cur : v)
        serial_traits<T>::serialize(ar, cur);
    }
    
    static void deserialize(IArchiveRegistry& ar, std::array<T, N>& v, uint64_t ncb) {
      // Read the number of entries first:
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));
      
      // Now loop until we get the desired number of entries from the stream
      for (auto& cur : v)
        serial_traits<T>::deserialize(ar, cur, 0);
    }
  };

  // Convenience specialization for std::vector
  template<typename T, typename Alloc>
  struct serial_traits<std::vector<T, Alloc>>
  {
    // We are irresponsible if and only if T is irresponsible
    static const bool is_irresponsible = serializer_is_irresponsible<T>::value;

    // If we're irresponsible, we need a more powerful archive registry as an input
    typedef typename std::conditional<is_irresponsible, IArchiveRegistry, IArchive>::type iarchive;

    static uint64_t size(const std::vector<T, Alloc>& v) {
      if (internal::is_constant_size<T>::value)
        // Constant-sized element types have a simple equation to compute total size
        return sizeof(uint32_t) + v.size() * sizeof(T);

      // More complex types require summation on a per-element basis
      uint64_t retVal = sizeof(uint32_t);
      for (const auto& cur : v)
        retVal += serial_traits<T>::size(cur);
      return retVal;
    }

    static void serialize(OArchiveRegistry& ar, const std::vector<T, Alloc>& v) {
      // Write the number of entries first:
      uint32_t nEntries = static_cast<uint32_t>(v.size());
      ar.Write(&nEntries, sizeof(nEntries));

      // Write each entry out, one at a time, using the preferred serializer
      for (const auto& cur : v)
        serial_traits<T>::serialize(ar, cur);
    }

    static void deserialize(iarchive& ar, std::vector<T, Alloc>& v, uint64_t ncb) {
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
    typedef serial_traits<typename Container::key_type> key_traits;
    typedef serial_traits<typename Container::mapped_type> mapped_traits;

    // Convenience static.  We need to make allocations if either our key or value type
    // needs to make allocations.
    static const bool sc_needsAllocation =
      serializer_is_irresponsible<typename Container::key_type>::value ||
      serializer_is_irresponsible<typename Container::mapped_type>::value;

    // We need to know if either the key type or the mapped type is irresponsible.
    // If it is, we're going to need to ensure we accept the correct input archive
    // type in order to ensure values are correctly propagated.
    typedef typename std::conditional<sc_needsAllocation, IArchiveRegistry, IArchive>::type iarchive;

    static uint64_t size(const Container& obj) {
      // Sum up all sizes for all child objects
      uint64_t retVal = sizeof(uint32_t);
      for (const auto& cur : obj)
        retVal +=
          serial_traits<typename Container::key_type>::size(cur.first) +
          serial_traits<typename Container::mapped_type>::size(cur.second);
      return retVal;
    }

    static void serialize(OArchiveRegistry& ar, const Container& obj) {
      // Write the number of entries first:
      uint32_t nEntries = static_cast<uint32_t>(obj.size());
      ar.Write(&nEntries, sizeof(nEntries));

      // Write out each key/value pair, one at a time
      for (const auto& cur : obj) {
        key_traits::serialize(ar, cur.first);
        mapped_traits::serialize(ar, cur.second);
      }
    }

    static void deserialize(iarchive& ar, Container& obj, uint64_t ncb) {
      // Read the number of entries first:
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));

      // Now read in all values:
      while (nEntries--) {
        typename Container::key_type key;
        key_traits::deserialize(ar, key, 0);
        mapped_traits::deserialize(ar, obj[key], 0);
      }
    }
  };

  template<typename T>
  struct serial_traits<std::unique_ptr<T>> {
    typedef std::unique_ptr<T> type;

    static uint64_t size(const type& pObj) {
      return sizeof(uint32_t);
    }

    static void serialize(OArchiveRegistry& ar, const type& obj) {
      // Identical implementation to serial_traits<T*>
      uint32_t objId = ar.RegisterObject(
        field_serializer_t<T, void>::GetDescriptor(),
        obj.get()
      );
      ar.Write(&objId, sizeof(objId));
    }

    static void deserialize(IArchive& ar, type& obj, uint64_t ncb) {
      // Object ID, then directed registration:
      uint32_t objId;
      ar.Read(&objId, sizeof(objId));

      // Now we just perform a lookup into our archive and store the result here
      obj.reset(
        (T*)ar.Release(
          []() -> void* { return new T; },
          field_serializer_t<T, void>::GetDescriptor(),
          objId
        )
      );
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