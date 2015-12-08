// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once

#include "serial_traits.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Test {
namespace Native {

  enum TestEnum : short {
    VALUE_ONE = 32,
    VALUE_TWO,
    VALUE_THREE
  };

  struct TestStructure {
    int x;
    int y;
    static leap::descriptor GetDescriptor(){
      return{
        { &TestStructure::x },
        { &TestStructure::y }
      };
    }
  };

  struct SubObject {
    char value;
    static leap::descriptor GetDescriptor() {
      return{ { 1, &SubObject::value } };
    }
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
    TestStructure h;
    SubObject i;
    SubObject j;

    static leap::descriptor GetDescriptor(void) {
      return{
        &TestObject::a,
        { 2, &TestObject::b },
        { 3, &TestObject::c },
        { 4, &TestObject::d },
        { 5, &TestObject::e },
        { 6, &TestObject::f },
        { 1, &TestObject::g },
        { 7, &TestObject::h },
        { 8, &TestObject::i },
        { 9, &TestObject::j }
      };
    }
  };

}
}
