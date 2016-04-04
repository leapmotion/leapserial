#include "stdafx.h"
#include <leapserial/LeapSerial.h>
#include <gtest/gtest.h>

class PolymorphicClassTest:
  public testing::Test
{};

namespace {
  class Base {
  public:
    virtual ~Base(void) {};

    static leap::descriptor GetDescriptor(void) {
      return {};
    }
  };

  class DerivedA :
    Base
  {
  public:
    std::string foo;

    static leap::descriptor GetDescriptor(void) {
      return{
        &DerivedA::foo
      };
    }
  };

  class DerivedB :
    Base
  {
  public:
    int x;

    static leap::descriptor GetDescriptor(void) {
      return{
        &DerivedB::x
      };
    }
  };

  class HasMember {
  public:
    std::unique_ptr<Base> ptr;

    static leap::descriptor GetDescriptor(void) {
      return{
        &HasMember::ptr
      };
    }
  };
}

TEST_F(PolymorphicClassTest, PolymorphicMember) {

}
