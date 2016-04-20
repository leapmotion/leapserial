// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <LeapSerial/LeapSerial.h>
#include <gtest/gtest.h>

struct HasPostInitRoutine {
  int foo = 101;
  int bar = 102;
  std::string helloWorld = "Hello World";

  void PostInit(void) {
    bar = foo;
  }

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasPostInitRoutine::foo,
      {1, &HasPostInitRoutine::helloWorld},
      &HasPostInitRoutine::PostInit
    };
  }
};

TEST(SerialCallbackTest, PostInitTest) {
  std::ostringstream os;
  {
    HasPostInitRoutine hpit;
    hpit.foo = 12991;

    leap::Serialize(os, hpit);
  }

  auto read = leap::Deserialize<HasPostInitRoutine>(std::istringstream(os.str()));
  ASSERT_EQ("Hello World", read->helloWorld) << "Identified field not correctly deserialized";
  ASSERT_EQ(12991, read->foo) << "Deserialization of primitve member did not correctly occur";
  ASSERT_EQ(12991, read->bar) << "Post initialization routine did not run as expected";
}

struct MyAccessorStruct {
  int a, b, c;

  int GetA() const { return a * 5; }
  const int& GetB() const { return b; }

  void SetA(int v) { a = v / 5; }
  void SetB(const int& v) { b = v; }

  static leap::descriptor GetDescriptor() {
    return
    {
      { 0, "field_a", &MyAccessorStruct::GetA, &MyAccessorStruct::SetA },
      { "field_b", &MyAccessorStruct::GetB, &MyAccessorStruct::SetB }
    };
  }
};


TEST(SerialCallbackTest, AccessorMethodTest) {
  MyAccessorStruct st{ 10, 20 };

  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  leap::Serialize(ss, st);

  MyAccessorStruct stIn;
  leap::Deserialize(ss, stIn);

  ASSERT_EQ(st.a, stIn.a) << "Structure was not serialized correctly";
  ASSERT_EQ(st.b, stIn.b) << "Structure was not serialized correctly";
}

static int ExternalGetA(const MyAccessorStruct& st) { return st.a; }
static void ExternalSetA(MyAccessorStruct& st, int v) { st.a = v; }
static int ExternalGetC(const MyAccessorStruct& st) { return st.c; }

TEST(SerialCallbackTest, GetStaticAsserts)
{
  // Validate traits on getter lambdas:
  auto getter = [](const MyAccessorStruct& s) { return s.b; };
  typedef leap::decompose<decltype(&decltype(getter)::operator())> t_decompose;
  static_assert(t_decompose::is_getter, "Getter decomposition trait not detected");
  static_assert(leap::getter<decltype(getter)>::value, "Getter trait not correctly detected");

  static_assert(leap::getter<int(*)(const MyAccessorStruct&)>::value, "Getter trait not correctly detected on getter function");

  static_assert(
    std::is_convertible<
      decltype(getter),
      leap::getter<decltype(getter)>::signature
    >::value,
    "Getter did not coerce correctly to the inferred setter signature"
  );
}

TEST(SerialCallbackTest, SetStaticAsserts)
{
  auto setter = [](MyAccessorStruct& s, int v) {};

  typedef leap::decompose<decltype(&decltype(setter)::operator())> t_decompose;
  static_assert(t_decompose::is_setter, "Setter decomposition trait not detected");
  static_assert(leap::setter<decltype(setter)>::value, "Setter trait not correctly detected on setter");

  static_assert(leap::setter<void(*)(MyAccessorStruct&, int)>::value, "Setter trait not correctly detected on setter function");

  static_assert(
    std::is_convertible<
      decltype(setter),
      leap::setter<decltype(setter)>::signature
    >::value,
    "Setter did not coerce correctly to the inferred setter signature"
  );
}

TEST(SerialCallbackTest, LambdaMethodTest) {
  MyAccessorStruct st{ 10, 20 };
  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);

  leap::descriptor desc =
  {
    leap::field_descriptor{0,"field_a", &ExternalGetA, &ExternalSetA},
    leap::field_descriptor{
      0,
      "field_b",
      [](const MyAccessorStruct& s) { return s.b; },
      [](MyAccessorStruct& s, int v) { s.b = v; }
    },
    leap::field_descriptor{
      0,
      "field_c",
      ExternalGetC,
      [](MyAccessorStruct& s, int v) { s.c = v; }
    }
  };

  leap::OutputStreamAdapter ssao{ ss };
  leap::OArchiveLeapSerial(ssao).WriteObject(desc, &st);

  MyAccessorStruct stIn;

  leap::InputStreamAdapter ssai{ ss };
  leap::IArchiveLeapSerial(ssai).ReadObject(desc, &stIn, nullptr);

  ASSERT_EQ(st.a, stIn.a) << "Field A was not serialized correctly";
  ASSERT_EQ(st.b, stIn.b) << "Field B was not serialized correctly";
  ASSERT_EQ(st.c, stIn.c) << "Field C was not serialized correctly";
}
