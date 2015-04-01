#pragma once

#include "serial_traits.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Test {
namespace Native {

  enum TestEnum {
    VALUE_ONE = 32,
    VALUE_TWO,
    VALUE_THREE
  };

  class TestObject {
  public:
    std::string unserialized;

    bool a;
    char b;
    int32_t c;
    uint64_t d;
    std::string e;
    std::vector<std::string> f;
    TestEnum g;

    static leap::descriptor GetDescriptor(void) {
      return{
        &TestObject::a,
        { 3, &TestObject::b },
        { 4, &TestObject::c },
        { 5, &TestObject::d },
        { 6, &TestObject::e },
        { 7, &TestObject::f },
        { 2, &TestObject::g },
      };
    }
  };

}
}