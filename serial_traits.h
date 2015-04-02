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
    static uint64_t size(const OArchiveRegistry& ar, const T& obj) {
      return GetDescriptor().size(ar, &obj);
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
    static uint64_t size(const OArchive& ar, T val) {
      return ar.SizeFloat(val);
    }

    static void serialize(OArchive& ar, const T& obj) {
      ar.WriteFloat(obj);
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
    static uint64_t size(const OArchive& ar, T val) {
      return ar.SizeInteger(val, sizeof(T));
    }

    static void serialize(OArchive& ar, T val) {
      ar.WriteInteger(val, sizeof(T));
    }

    static void deserialize(IArchive& ar, T& val, uint64_t ncb) {
      val = static_cast<T>(ar.ReadVarint());
    }
  };

  template<typename T>
  struct primitive_serial_traits<T, typename std::enable_if<std::is_enum<T>::value>::type>
  {
    // Trivial serialization/deserialization operations
    static uint64_t size(const OArchive& ar, T val) {
      return ar.SizeInteger(val, sizeof(T));
    }

    static void serialize(OArchive& ar, T val) {
      ar.WriteInteger(val, sizeof(T));
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
    static uint64_t size(const OArchiveRegistry& ar, const T*const pObj) { 
      return ar.SizeObjectReference(field_serializer_t<T, void>(), pObj);
    }

    static void serialize(OArchiveRegistry& ar, const T* pObj) {
      ar.WriteObjectReference(
        field_serializer_t<T, void>::GetDescriptor(),
        pObj
      );
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

  // Const pointer serialization/deserialization rules are the same as pointer serialization rules,
  // but we act as though the type itself is non-const for the purpose of deserialization
  template<typename T>
  struct serial_traits<const T*>:
    serial_traits<T*>
  {
    static void deserialize(IArchiveRegistry& ar, const T*& pObj, uint64_t ncb) {
      T* ptr;
      serial_traits<T*>::deserialize(ar, ptr, ncb);
      pObj = ptr;
    }
  };

  template<>
  struct serial_traits<bool>
  {
    static uint64_t size(const OArchive& ar, bool val) {
      return ar.SizeBool(val);
    }

    static void serialize(OArchive& ar, bool val) {
      ar.WriteBool(val);
    }

    static void deserialize(IArchive& ar, bool& val, uint64_t ncb) {
      val = ar.ReadVarint() ? true : false;
    }
  };
  
  template<typename T, size_t N>
  struct serial_traits<T[N]>
  {
    static uint64_t size(const OArchiveRegistry& ar, const T* pObj) {
      size_t i = 0;
      return ar.SizeArray(field_serializer_t<T,void>(), N, [&] {return &pObj[i++]; });
    }

    static void serialize(OArchiveRegistry& ar, const T* pObj) {
      size_t i = 0;
      ar.WriteArray(field_serializer_t<T,void>(), N, [&]{ return &pObj[i++]; });
    }

    static void deserialize(IArchiveRegistry& ar, T* pObj, uint64_t ncb) {
      // Read the number of entries first:
      uint32_t nEntries;
      ar.Read(&nEntries, sizeof(nEntries));
      if (nEntries != N)
        // We expected to get N entries, but got back some other value.  Something is wrong.
        throw std::runtime_error("Attempted to deserialize a non-matching number of entries into a fixed-size space");

      // Now loop until we get the desired number of entries from the stream
      for (size_t i = 0; i < N; i++)
        serial_traits<T>::deserialize(ar, pObj[i], 0);
    }
  };
  
  template<typename T, size_t N>
  struct serial_traits<std::array<T,N>>
  {
    static uint64_t size(const OArchiveRegistry& ar, const std::array<T, N>& v) {
      return serial_traits<T[N]>::size(ar, &v.front());
    }
    
    static void serialize(OArchiveRegistry& ar, const std::array<T, N>& v) {
      return serial_traits<T[N]>::serialize(ar, &v.front());
    }
    
    static void deserialize(IArchiveRegistry& ar, std::array<T, N>& v, uint64_t ncb) {
      return serial_traits<T[N]>::deserialize(ar, &v.front(), ncb);
    }
  };

  // Convenience specialization for std::vector
  template<typename T, typename Alloc>
  struct serial_traits<std::vector<T, Alloc>>
  {
    static_assert(
      !std::is_base_of<std::false_type, serial_traits<T>>::value,
      "Attempted to serialize a vector of type T, but T is not serializable"
    );

    // We are irresponsible if and only if T is irresponsible
    static const bool is_irresponsible = serializer_is_irresponsible<T>::value;

    // If we're irresponsible, we need a more powerful archive registry as an input
    typedef typename std::conditional<is_irresponsible, IArchiveRegistry, IArchive>::type iarchive;

    static uint64_t size(const OArchiveRegistry& ar, const std::vector<T, Alloc>& v) {
      size_t i = 0;
      return ar.SizeArray(field_serializer_t<T,void>(), v.size(), [&] { return &v[i++]; });
    }

    static void serialize(OArchiveRegistry& ar, const std::vector<T, Alloc>& v) {
      size_t i = 0;
      ar.WriteArray(field_serializer_t<T,void>(),
        static_cast<uint64_t>(v.size()),
        [&] { return &v[i++]; }
      );
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

    static uint64_t size(const OArchive& ar, const type& pObj) {
      return ar.SizeString(pObj.data(), pObj.size() * sizeof(T), sizeof(T));
    }

    static void serialize(OArchive& ar, const type& obj) {
      ar.WriteString(obj.data(), obj.size()*sizeof(T), sizeof(T));
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
    typedef typename Container::key_type key_type;
    typedef typename Container::mapped_type mapped_type;
    typedef serial_traits<key_type> key_traits;
    typedef serial_traits<mapped_type> mapped_traits;
    
    static_assert(
      !std::is_base_of<std::false_type, serial_traits<key_type>>::value,
      "Attempted to serialize a map, but the map's key type is not serializable"
    );
    static_assert(
      !std::is_base_of<std::false_type, serial_traits<mapped_type>>::value,
      "Attempted to serialize a map, but the map's mapped type is not serializable"
    );

    // Convenience static.  We need to make allocations if either our key or value type
    // needs to make allocations.
    static const bool sc_needsAllocation =
      serializer_is_irresponsible<typename Container::key_type>::value ||
      serializer_is_irresponsible<typename Container::mapped_type>::value;

    // We need to know if either the key type or the mapped type is irresponsible.
    // If it is, we're going to need to ensure we accept the correct input archive
    // type in order to ensure values are correctly propagated.
    typedef typename std::conditional<sc_needsAllocation, IArchiveRegistry, IArchive>::type iarchive;

    static uint64_t size(const OArchiveRegistry& ar, const Container& obj) {
      auto iKey = obj.begin();
      auto iValue = obj.begin();
      return ar.SizeDictionary(obj.size(),
        field_serializer_t<typename Container::key_type, void>(), [&]{ return &(iKey++)->first; },
        field_serializer_t<typename Container::mapped_type, void>(), [&]{ return &(iValue++)->second; }
      );
    }

    static void serialize(OArchiveRegistry& ar, const Container& obj) {
      auto iKey = obj.begin();
      auto iValue = obj.begin();
      return ar.WriteDictionary(obj.size(),
        field_serializer_t<typename Container::key_type, void>(), [&]{ return &((iKey++)->first); },
        field_serializer_t<typename Container::mapped_type, void>(), [&]{ return &((iValue++)->second); }
      );
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
  
  // Associative array types:
  template<typename Key, typename Value>
  struct serial_traits<std::map<Key, Value>> :
    serial_traits_map_t<std::map<Key, Value>>
  {};

  template<typename Key, typename Value>
  struct serial_traits<std::unordered_map<Key, Value>> :
    serial_traits_map_t<std::unordered_map<Key, Value>>
  {};

  template<typename T>
  struct serial_traits<std::unique_ptr<T>> {
    typedef std::unique_ptr<T> type;

    static uint64_t size(const OArchiveRegistry& ar, const type& pObj) {
      return ar.SizeObjectReference(field_serializer_t<T,void>(), pObj.get());
    }

    static void serialize(OArchiveRegistry& ar, const type& obj) {
      ar.WriteObjectReference(
        field_serializer_t<T, void>::GetDescriptor(),
        obj.get()
      );
    }

    static void deserialize(IArchive& ar, type& obj, uint64_t ncb) {
      // Object ID, then directed registration:
      uint32_t objId;
      ar.Read(&objId, sizeof(objId));

      // Verify no unique pointer aliases:
      if (ar.IsReleased(objId))
        throw std::runtime_error("Attempted to map the same object into two distinct unique pointers");

      // Now we just perform a lookup into our archive and store the result here
      auto released = ar.Release(
        [] {
          return IArchive::ReleasedMemory{new T, nullptr};
        },
        field_serializer_t<T, void>::GetDescriptor(),
        objId
      );
      obj.reset(static_cast<T*>(released.pObject));
    }
  };

  template<typename T>
  struct serial_traits<std::shared_ptr<T>> {
    typedef std::shared_ptr<T> type;

    static uint64_t size(const OArchiveRegistry& ar, const type& pObj) {
      return ar.SizeObjectReference(field_serializer_t<T,void>(), pObj.get());
    }

    static void serialize(OArchiveRegistry& ar, const type& obj) {
      ar.WriteObjectReference(
        field_serializer_t<T, void>::GetDescriptor(),
        obj.get()
      );
    }

    static void deserialize(IArchive& ar, type& obj, uint64_t ncb) {
      // Object ID, then directed registration:
      uint32_t objId;
      ar.Read(&objId, sizeof(objId));

      // We're using the shared pointer directly as our context field
      auto released = ar.Release(
        [] {
          std::shared_ptr<T> retVal = std::make_shared<T>();
          return IArchive::ReleasedMemory{retVal.get(), retVal};
        },
        field_serializer_t<T, void>::GetDescriptor(),
        objId
      );
      obj = std::static_pointer_cast<T>(released.pContext);
    }
  };

  
}