// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Allocation.h"
#include "Archive.h"
#include "Descriptor.h"
#include "field_serializer.h"
#include "IArchiveImpl.h"
#include "IArchiveProtobuf.h"
#include "OArchiveImpl.h"
#include "OArchiveProtobuf.h"
#include "SchemaWriterProtobuf.h"
#include <memory>
#include <istream>

namespace leap {
  struct descriptor;

  template<typename T, typename>
  struct field_serializer_t;

  struct leapserial {
    typedef IArchiveImpl iarchive;
    typedef OArchiveImpl oarchive;
  };

  struct protobuf_v1 {
    typedef IArchiveProtobuf iarchive;
    typedef OArchiveProtobuf oarchive;
    typedef SchemaWriterProtobuf schema;
  };

  struct protobuf_v2 {
    typedef IArchiveProtobuf iarchive;
    typedef OArchiveProtobuf oarchive;
    typedef SchemaWriterProtobuf2 schema;
  };

  template<typename archive_t, typename T>
  void SerializeWithArchive(archive_t& ar, const T& obj) {
    static_assert(!std::is_pointer<T>::value, "Do not serialize a pointer to an object, serialize a reference");
    static_assert(
      !std::is_base_of<std::false_type, field_serializer_t<T, void>>::value,
      "serial_traits is not specialized on the specified type, and this type also doesn't provide a GetDescriptor routine"
    );

    ar.WriteObject(
      field_serializer_t<T, void>::GetDescriptor(),
      &obj
    );
  }

  template<typename archive_t, typename stream_t>
  void Serialize(stream_t& os, const leap::descriptor& desc) {
    typename archive_t::schema s{ desc };
    s.Write(os);
  }
  template<typename archive_t, typename T>
  void Serialize(std::ostream& os, const T& obj) {
    leap::OutputStreamAdapter osa{ os };
    archive_t ar(osa);
    SerializeWithArchive<archive_t, T>(ar, obj);
  }
  template<typename archive_t, typename T>
  void Serialize(IOutputStream& os, const T& obj) {
    archive_t ar(os);
    SerializeWithArchive(ar, obj);
  }
  template<typename T>
  void Serialize(std::ostream& os, const T& obj) {
    leap::OutputStreamAdapter osa{ os };
    leap::OArchiveImpl ar(osa);
    SerializeWithArchive(ar, obj);
  }
  template<typename T>
  void Serialize(IOutputStream& os, const T& obj) {
    leap::OArchiveImpl ar(os);
    SerializeWithArchive(ar, obj);
  }

  ///I/OArchiveImpl defaulted versions:
  template<typename archive_t, class stream_t, class T>
  void Serialize(stream_t&& os, const T& obj) {
    Serialize<archive_t, T>(os, obj);
  }

  /// <summary>
  /// Deserialization routine that returns an std::shared_ptr for memory allocation maintenance
  /// </summary>
  template<class T = void, class archive_t = IArchiveImpl, class stream_t = std::istream>
  std::shared_ptr<T> Deserialize(stream_t&& is) {
    auto retVal = std::make_shared<leap::internal::Allocation<T>>();
    T* pObj = &retVal->val;

    // Initialize the archive with work to be done:
    archive_t ar(is);
    ar.ReadObject(field_serializer_t<T, void>::GetDescriptor(), pObj, retVal.get() );

    // Aliasing ctor, because T is a member of Allocation<T> and not a base
    return { retVal, pObj };
  }

  /// <summary>
  /// Deserialization routine that modifies object in place
  /// </summary>
  template<class archive_t = IArchiveImpl, class T = void, class stream_t = std::istream>
  void Deserialize(stream_t&& is, T& obj) {
    // Initialize the archive with work to be done:
    archive_t ar(is);
    ar.ReadObject(field_serializer_t<T, void>::GetDescriptor(), &obj, nullptr);
  }

  // Fill a collection with objects serialized to 'is'
  // Returns one past the end of the container
  template<class T, class archive_t = IArchiveImpl, class stream_t = std::istream>
  std::vector<T> DeserializeToVector(stream_t&& is) {
    std::vector<T> collection;
    while (is.peek() != std::char_traits<char>::eof()) {
      collection.emplace_back();
      Deserialize<archive_t, T>(is, collection.back());
    }
    return collection;
  }

  // Utility method until everyone moves to C++14
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args) {
    return std::unique_ptr<T>{new T{ std::forward<Args&&>(args)... }};
  }
}
