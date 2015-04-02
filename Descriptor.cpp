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
  uint64_t retVal = 0;
  for (const auto& field_descriptor : field_descriptors)
    retVal += field_descriptor.serializer.size(ar,
      static_cast<const char*>(pObj) +field_descriptor.offset
    );
  for (const auto& cur : identified_descriptors) {
    const auto& identified_descriptor = cur.second;
    // Need the type of the child object and its size proper
    serial_type type = identified_descriptor.serializer.type();
    uint64_t ncbChild = 
      identified_descriptor.serializer.size(ar,
        static_cast<const char*>(pObj) + identified_descriptor.offset
      );

    // Add the size required to encode type information and identity information to the
    // size proper of the child object
    retVal +=
      leap::serial_traits<uint32_t>::size(ar,
        (identified_descriptor.identifier << 3) |
        static_cast<int>(type)
      ) +
      ncbChild;

    if (type == serial_type::string)
      // Need to know the size-of-the-size
      retVal += leap::serial_traits<uint64_t>::size(ar,ncbChild);
  }
  return retVal;
}

void leap::descriptor::serialize(OArchiveRegistry& ar, const void* pObj) const {
  // Stationary descriptors first:
  for (const auto& field_descriptor : field_descriptors)
    field_descriptor.serializer.serialize(
      ar,
      static_cast<const char*>(pObj) + field_descriptor.offset
    );

  // Then variable descriptors:
  for (const auto& cur : identified_descriptors) {
    const auto& identified_descriptor = cur.second;
    const void* pChildObj = static_cast<const char*>(pObj) + identified_descriptor.offset;

    // Has identifier, need to write out the ID with the type and then the payload
    auto type = identified_descriptor.serializer.type();
    ar.WriteInteger(
      (identified_descriptor.identifier << 3) |
      static_cast<int>(type),
      sizeof(int)
    );

    // Decide whether this is a counted sequence or not:
    switch (type) {
    case serial_type::string:
      // Counted string, write the size first
      ar.WriteInteger((int64_t)identified_descriptor.serializer.size(ar,pChildObj), sizeof(uint64_t));
      break;
    default:
      // Nothing else requires that the size be written
      break;
    }

    // Now handoff to serialization proper
    identified_descriptor.serializer.serialize(ar, pChildObj);
  }
}

void leap::descriptor::deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const {
  uint64_t countLimit = ar.Count() + ncb;
  for (const auto& field_descriptor : field_descriptors)
    field_descriptor.serializer.deserialize(
      ar,
      static_cast<char*>(pObj) + field_descriptor.offset,
      0
    );

  if (!ncb)
    // Impossible for there to be more fields, we don't have a sizer
    return;

  // Identified fields, read them in
  while (ar.Count() < countLimit) {
    // Ident/type field first
    uint64_t ident = ar.ReadVarint();
    uint64_t ncbChild;
    switch ((serial_type)(ident & 7)) {
    case serial_type::string:
      ncbChild = ar.ReadVarint();
      break;
    case serial_type::b64:
      ncbChild = 8;
      break;
    case serial_type::b32:
      ncbChild = 4;
      break;
    case serial_type::varint:
      ncbChild = 0;
      break;
    default:
      throw std::runtime_error("Unexpected type field encountered");
    }

    // See if we can find the descriptor for this field:
    auto q = identified_descriptors.find(ident >> 3);
    if (q == identified_descriptors.end())
      // Unrecognized field, need to skip
      if (static_cast<serial_type>(ident & 7) == serial_type::varint)
        // Just read a varint in that we discard right away
        ar.ReadVarint();
      else
        // Skip the requisite number of bytes
        ar.Skip(static_cast<size_t>(ncbChild));
    else
      // Hand off to child class
      q->second.serializer.deserialize(
        ar,
        static_cast<char*>(pObj) + q->second.offset,
        static_cast<size_t>(ncbChild)
      );
  }

  if (ar.Count() > countLimit) {
    std::ostringstream os;
    os << "Deserialization error, read " << (ar.Count() - countLimit + ncb) << " bytes and expected to read " << ncb << " bytes";
    throw std::runtime_error(os.str());
  }
}
