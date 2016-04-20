// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "TestProtobufLS.hpp"
#include <LeapSerial/LeapSerial.h>
#include <LeapSerial/IArchiveProtobuf.h>
#include <LeapSerial/OArchiveProtobuf.h>
#include <gtest/gtest.h>
#include <sstream>

// This is required on Windows due to the way protobuf uses std::copy
#define _SCL_SECURE_NO_WARNINGS
#include "TestProtobuf.pb.h"
#undef _SCL_SECURE_NO_WARNINGS

static std::string ToProtobuf(const Person& in) {
  // Write the protobuf version first:
  leap::test::Person person;
  person.set_name(in.name);
  person.set_id(in.id);
  person.set_email(in.email);

  for (const auto& pn : in.phone) {
    auto* phoneNumber = person.add_phone();
    phoneNumber->set_number(pn.number);
    phoneNumber->set_type(static_cast<leap::test::Person_PhoneType>(pn.type));
  }
  for (const auto& entry : in.pets) {
    leap::test::PetFieldEntry* petEntry = person.add_pet();
    petEntry->set_key(entry.first);

    leap::test::Pet* pet = petEntry->mutable_value();
    pet->set_name(entry.second.name);
    pet->set_species((leap::test::Pet_Species)entry.second.species);
  }
  return person.SerializeAsString();
}

static Person MakeDefaultPerson(void) {
  Person person;
  person.name = "John";
  person.id = 100;
  person.email = "john@johnshouse.com";
  person.phone.resize(1);
  person.phone[0].number = "210-555-9294";
  person.phone[0].type = PhoneNumber::HOME;
  Pet& snake = person.pets["snake"];
  snake.name = "Snake";
  snake.species = Pet::Species::DOG;
  return person;
}

TEST(ArchiveProtobufTest, LeapSerialToProtobuf) {
  const Person defaultPerson = MakeDefaultPerson();

  std::stringstream ss;
  leap::Serialize<leap::OArchiveProtobuf>(leap::OutputStreamAdapter(ss), defaultPerson);
  std::string val = ss.str();

  leap::test::Person person;
  ASSERT_TRUE(person.ParseFromString(val)) << "Failed to deserialize a LeapSerial-formatted reference message";
}

TEST(ArchiveProtobufTest, ProtobufToLeapSerial) {
  const Person defaultPerson = MakeDefaultPerson();
  std::string s = ToProtobuf(defaultPerson);
  std::stringstream ss(s);

  Person person;
  leap::Deserialize<leap::IArchiveProtobuf>(leap::InputStreamAdapter(ss), person);
  ASSERT_EQ(person, defaultPerson);
}
