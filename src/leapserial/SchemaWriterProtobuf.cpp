// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "SchemaWriterProtobuf.h"
#include "Descriptor.h"
#include "ProtobufUtil.hpp"
#include <sstream>

using namespace leap;

SchemaWriterProtobuf::SchemaWriterProtobuf(const descriptor& desc, protobuf::Version version) :
  Desc(desc),
  Version(version)
{}

struct SchemaWriterProtobuf::indent {
  indent(size_t level) : level(level) {}
  const size_t level;
  friend std::ostream& operator<<(std::ostream& os, const indent& rhs) {
    for (size_t i = rhs.level; i--;)
      os << "  ";
    return os;
  }
};

struct SchemaWriterProtobuf::name {
  name(const descriptor& desc) : desc(desc) {}
  const descriptor& desc;
  friend std::ostream& operator<<(std::ostream& os, const name& rhs) {
    if (rhs.desc.name)
      os << rhs.desc.name;
    else
      // Need to derive a locally valid name
      os << "msg_" << std::hex << std::setfill('0') << std::setw(8) << reinterpret_cast<uint32_t>(&rhs.desc);
    return os;
  }
};

struct SchemaWriterProtobuf::type {
  type(SchemaWriterProtobuf& parent, const field_serializer& ser, bool print_req = true) :
    parent(parent),
    ser(ser),
    print_req(print_req)
  {}
  
  SchemaWriterProtobuf& parent;
  const field_serializer& ser;
  bool print_req = true;

  const char* signifier(void) const {
    return
      !print_req ?
      "" :
      ser.is_optional() ?
      "optional " :
      "required ";
  }

  std::ostream& print(std::ostream& os) const {
    switch (auto atom = ser.type()) {
    case serial_atom::array:
      // "repeated" field, entry type knows more
      return os << "repeated " << type{ parent, dynamic_cast<const field_serializer_array&>(ser).element(), false };

    case serial_atom::map:
      {
        const auto& ser_map = dynamic_cast<const field_serializer_map&>(ser);
        switch(parent.Version) {
        case protobuf::Version::Proto1:
          {
            parent.synthetic.emplace_back(nullptr);
            std::unique_ptr<leap::descriptor>& pDesc = parent.synthetic.back();

            std::string* pSyntheticName;
            {
              std::stringstream ss;
              ss << "MapEntry_" << std::hex << std::setw(8) << std::setfill('0') << reinterpret_cast<uint32_t>(&pDesc);
              parent.syntheticNames.emplace_back(ss.str());
              pSyntheticName = &parent.syntheticNames.back();
            }
            pDesc = std::unique_ptr<leap::descriptor> {
              new leap::descriptor{
                pSyntheticName->c_str(),
                {
                  leap::field_descriptor { ser_map.key(), "key", 1, ~0UL },
                  leap::field_descriptor { ser_map.mapped(), "value", 2, ~0UL }
                }
              }
            };
            parent.awaiting.push_back(pDesc.get());
            os << "repeated " << *pSyntheticName;
          }
          break;
        case protobuf::Version::Proto2:
        case protobuf::Version::Proto3:
          return os
            << signifier()
            << "map<"
            << type{ parent, ser_map.key() }
            << ','
            << type{ parent, ser_map.mapped() }
            << '>';
        }
      }
      break;
    case serial_atom::descriptor:
    case serial_atom::finalized_descriptor:
      // Need to generate a reference to this type by name:
      {
        const auto& f_ser_obj = dynamic_cast<const field_serializer_object&>(ser);
        const auto& ser_obj = f_ser_obj.object();
        parent.awaiting.push_back(&ser_obj);
        return os << signifier() << name{ ser_obj };
      }
    default:
      // Type is fundamental
      return os
        << signifier()
        << leap::internal::protobuf::ToProtobufField(atom);
    }
    return os;
  }
  friend std::ostream& operator<<(std::ostream& os, const type& rhs) { return rhs.print(os); }
};

void SchemaWriterProtobuf::Write(IOutputStream& os) {
  std::stringstream ss;
  Write(ss);
  auto str = ss.str();
  os.Write(str.data(), str.size());
}

void SchemaWriterProtobuf::Write(std::ostream& os) {
  awaiting.push_back(&Desc);
  encountered.clear();
  synthetic.clear();
  syntheticNames.clear();

  while (!awaiting.empty()) {
    const descriptor& cur = *awaiting.back();
    awaiting.pop_back();

    if (encountered.count(&cur))
      continue;
    encountered.insert(&cur);

    os << "message " << name(cur) << " {" << std::endl;
    for (auto& e : cur.identified_descriptors) {
      os << indent(tabLevel + 1) << type(*this, e.second.serializer);
      os << ' ';
      if (e.second.name)
        os << e.second.name;
      else
        os << "field_" << std::dec << e.first;

      os << " = " << e.first << ";" << std::endl;
    }
    os << '}' << std::endl << std::endl;
  }
}
