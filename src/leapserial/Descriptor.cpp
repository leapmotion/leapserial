#include "stdafx.h"
#include "Descriptor.h"
#include "serial_traits.h"
#include <sstream>

using namespace leap;

leap::descriptor::descriptor(const field_descriptor* begin, const field_descriptor* end) {
  for (auto cur = begin; cur != end; cur++) {
    const auto& field_descriptor = *cur;
    if (field_descriptor.identifier)
      this->identified_descriptors.insert(std::make_pair(field_descriptor.identifier, field_descriptor));
    else
      this->field_descriptors.push_back(field_descriptor);

    // Intentional boolean optimization
    m_allocates = m_allocates || field_descriptor.serializer.allocates();
  }
}

uint64_t leap::descriptor::size(const OArchiveRegistry& ar, const void* pObj) const {
  return ar.SizeDescriptor(*this, pObj);
}

void leap::descriptor::serialize(OArchiveRegistry& ar, const void* pObj) const {
  ar.WriteDescriptor(*this, pObj);
}

void leap::descriptor::deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const {
  ar.ReadDescriptor(*this, pObj, ncb);
}
