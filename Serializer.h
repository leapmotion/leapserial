#pragma once
#include "Allocation.h"
#include "Archive.h"
#include "IArchiveImpl.h"
#include "OArchiveImpl.h"
#include "Descriptor.h"
#include "field_serializer.h"
#include <memory>
#include <istream>

namespace leap {
  struct descriptor;

  template<typename T, typename>
  struct field_serializer_t;

  template<class stream_t, class T>
  void Serialize(stream_t&& os, const T& obj) {
    static_assert(!std::is_pointer<T>::value, "Do not serialize a pointer to an object, serialize a reference");
    static_assert(
      !std::is_base_of<std::false_type, field_serializer_t<T, void>>::value,
      "serial_traits is not specialized on the specified type, and this type also doesn't provide a GetDescriptor routine"
    );

    OArchiveImpl ar(os);
    ar.RegisterObject(
      field_serializer_t<T, void>::GetDescriptor(),
      &obj
    );
    ar.Process();
  }

  /// <summary>
  /// Deserialization routine that returns and std::shared_ptr for memory allocation maintenance
  /// </summary>
  template<class T, class stream_t>
  std::shared_ptr<T> Deserialize(stream_t&& is) {
    auto retVal = std::make_shared<leap::internal::Allocation<T>>();
    T* pObj = retVal.get();
    
    // Initialize the archive with work to be done:
    IArchiveImpl ar(is, pObj);
    ar.Process(
      IArchiveImpl::deserialization_task(
        &field_serializer_t<T, void>::GetDescriptor(),
        0,
        pObj
      )
    );

    ar.Transfer(*retVal);
    return retVal;
  }

  /// <summary>
  /// Deserialization routine that returns and std::shared_ptr for memory allocation maintenance
  /// </summary>
  template<class T>
  void Deserialize(std::istream& is, T& obj) {
    // Initialize the archive with work to be done:
    IArchiveImpl ar(is, &obj);
    ar.Process(
      IArchiveImpl::deserialization_task(
        &field_serializer_t<T, void>::GetDescriptor(),
        0,
        &obj
      )
    );

    // If objects exist that require transferrence, then we have an error
    if (ar.ClearObjectTable())
      throw std::runtime_error(
        "Attempted to perform an allocator-free deserialization on a stream whose types are not completely responsible for their own cleanup"
      );
  }

  template<class T>
  void Deserialize(std::istream&& is, T& obj) {
    Deserialize<T>(is, obj);
  }

  // Fill a collection with objects serialized to 'is'
  // Returns one past the end of the container
  template<class T>
  std::vector<T> DeserializeToVector(std::istream& is) {
    std::vector<T> collection;
    while (is.peek() != std::char_traits<char>::eof()) {
      collection.emplace_back();
      Deserialize<T>(is, collection.back());
    }
    return collection;
  }
  
  template<class T>
  std::vector<T> DeserializeToVector(std::istream&& is) {
    return DeserializeToVector<T>(is);
  }
}
