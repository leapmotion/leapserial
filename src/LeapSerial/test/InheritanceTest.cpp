// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <LeapSerial/LeapSerial.h>
#include <gtest/gtest.h>

class InheritanceTest:
  public testing::Test
{};

struct InheritanceTestBase
{
  int a = 1001;
  std::string name = "Benjamin";

  static leap::descriptor GetDescriptor() {
    return{
      &InheritanceTestBase::a,
      &InheritanceTestBase::name
    };
  }
};

struct InheritanceTestDerived:
  std::vector<int>,
  InheritanceTestBase
{
  virtual ~InheritanceTestDerived(void) {}

  std::string secondName = "Franklin";

  static leap::descriptor GetDescriptor(void) {
    return {
      leap::base<InheritanceTestBase, InheritanceTestDerived>(),
      &InheritanceTestDerived::secondName
    };
  }
};

TEST_F(InheritanceTest, SingleInheritanceTest) {
  InheritanceTestDerived x;
  x.a = 1002;
  x.name = "George";
  x.secondName = "Washington";

  std::stringstream ss;
  leap::Serialize(ss, x);
  auto retVal = leap::Deserialize<InheritanceTestDerived>(ss);

  ASSERT_EQ(1002, retVal->a) << "Trivial base member was not correctly serialized";
  ASSERT_EQ("George", retVal->name) << "Recursively serialized base member was not correctly deserialized";
  ASSERT_EQ("Washington", retVal->secondName) << "Derived member was not correctly deserialized";
}

struct DiamondBase {
  std::string val;

  static leap::descriptor GetDescriptor(void) {
    return{
      &DiamondBase::val
    };
  }
};

template<int I>
struct DiamondLeg:
  DiamondBase
{
  static leap::descriptor GetDescriptor(void) {
    return{
      leap::base<DiamondBase, DiamondLeg<I>>()
    };
  }
};

struct DiamondInheritance:
  DiamondLeg<0>,
  DiamondLeg<1>
{
  virtual ~DiamondInheritance(void) {}

  std::string myMember = "Standard";

  static leap::descriptor GetDescriptor(void) {
    return {
      leap::base<DiamondLeg<0>, DiamondInheritance>(),
      leap::base<DiamondLeg<1>, DiamondInheritance>(),
      &DiamondInheritance::myMember
    };
  }
};

TEST_F(InheritanceTest, DiamondInheritanceTest) {
  DiamondInheritance x;
  static_cast<DiamondLeg<0>&>(x).val = "Hello";
  static_cast<DiamondLeg<1>&>(x).val = "World";
  x.myMember = "VIP";

  std::stringstream ss;
  leap::Serialize(ss, x);
  auto retVal = leap::Deserialize<DiamondInheritance>(ss);

  ASSERT_EQ("VIP", retVal->myMember) << "Trivial base member was not correctly serialized";
  ASSERT_EQ(
    "Hello",
    static_cast<DiamondLeg<0>&>(*retVal).val
  ) << "Left-hand side of the diamond did not deserialize correctly";
  ASSERT_EQ(
    "World",
    static_cast<DiamondLeg<1>&>(*retVal).val
  ) << "Right-hand side of the diamond did not deserialize correctly";
}