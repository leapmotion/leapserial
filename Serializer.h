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

  template<class archive_t, class stream_t, class T>
  void Serialize(stream_t&& os, const T& obj) {
    static_assert(!std::is_pointer<T>::value, "Do not serialize a pointer to an object, serialize a reference");
    static_assert(
      !std::is_base_of<std::false_type, field_serializer_t<T, void>>::value,
      "serial_traits is not specialized on the specified type, and this type also doesn't provide a GetDescriptor routine"
    );

    archive_t ar(os);
    ar.WriteObject(
      field_serializer_t<T, void>::GetDescriptor(),
      &obj
    );
  }

  /// <summary>
  /// Deserialization routine that returns an std::shared_ptr for memory allocation maintenance
  /// </summary>
  template<class archive_t, class T, class stream_t>
  std::shared_ptr<T> Deserialize(stream_t&& is) {
    auto retVal = std::make_shared<leap::internal::Allocation<T>>();
    T* pObj = retVal.get();
    
    // Initialize the archive with work to be done:
    archive_t ar(is);
    ar.ReadObject(field_serializer_t<T, void>::GetDescriptor(), pObj, retVal.get() );
    
    return retVal;
  }

  /// <summary>
  /// Deserialization routine that modifies object in place
  /// </summary>
  template<class archive_t, class T>
  void Deserialize(std::istream& is, T& obj) {
    // Initialize the archive with work to be done:
    archive_t ar(is);
    ar.ReadObject(field_serializer_t<T, void>::GetDescriptor(), &obj, nullptr);
  }

  template<class archive_t, class T>
  void Deserialize(std::istream&& is, T& obj) {
    Deserialize<archive_t, T>(is, obj);
  }

  // Fill a collection with objects serialized to 'is'
  // Returns one past the end of the container
  template<class T, class archive_t = IArchiveImpl>
  std::vector<T> DeserializeToVector(std::istream& is) {
    std::vector<T> collection;
    while (is.peek() != std::char_traits<char>::eof()) {
      collection.emplace_back();
      Deserialize<archive_t, T>(is, collection.back());
    }
    return collection;
  }
  
  template<class T, class archive_t = IArchiveImpl>
  std::vector<T> DeserializeToVector(std::istream&& is) {
    return DeserializeToVector<T, archive_t>(is);
  }

  ///I/OArchiveImpl defaulted versions:
  template<class stream_t, class T>
  void Serialize(stream_t&& os, const T& obj) {
    Serialize<OArchiveImpl, stream_t, T>(std::move(os), obj);
  }

  template<class T, class stream_t>
  std::shared_ptr<T> Deserialize(stream_t&& is) {
    return Deserialize<IArchiveImpl, T, stream_t&>(is);
  }

  template<class T>
  void Deserialize(std::istream& is, T& obj) {
    Deserialize<IArchiveImpl, T>(is, obj);
  }

  template<class T>
  void Deserialize(std::istream&& is, T& obj) {
    Deserialize<IArchiveImpl, T>(is, obj);
  }
}
