#pragma once
#include "Allocation.h"
#include "Archive.h"
#include "IArchiveImpl.h"
#include "OArchiveImpl.h"
#include "Descriptor.h"
#include "field_serializer.h"
#include <memory>

namespace leap {
  struct descriptor;

  template<typename T, typename>
  struct field_serializer_t;

  template<class T>
  void Serialize(std::ostream& os, const T& obj) {
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

  template<class T>
  std::shared_ptr<T> Deserialize(std::istream& is) {
    auto retVal = std::make_shared<leap::internal::Allocation<T>>();
    
    // Initialize the archive with work to be done:
    IArchiveImpl ar(is, *retVal);
    ar.Process(
      IArchiveImpl::deserialization_task(
        &field_serializer_t<T, void>::GetDescriptor(),
        0,
        static_cast<T*>(retVal.get())
      )
    );

    return retVal;
  }

  template<class T>
  std::shared_ptr<T> Deserialize(std::istream&& is) {
    return Deserialize<T>(is);
  }
}