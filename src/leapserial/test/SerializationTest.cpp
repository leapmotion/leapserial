// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include <leapserial/LeapSerial.h>
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include <sstream>
#include <type_traits>

class SerializationTest:
  public testing::Test
{};

// Need to ensure that all builtins are correctly recognized
static_assert(leap::has_serializer<std::string>::value, "std::string");
static_assert(leap::has_serializer<std::vector<int>>::value, "Vector of ints");
static_assert(leap::has_serializer<std::vector<std::string>>::value, "Vector of strings");
static_assert(leap::serializer_is_irresponsible<std::vector<int*>>::value, "Irresponsible allocator was not correctly inferred");
static_assert(!leap::serializer_is_irresponsible<std::vector<int>>::value, "Irresponsibility incorrectly inferred in a responsible allocator");

struct DummyStruct {};
static_assert(!leap::has_serializer<DummyStruct>::value, "A dummy structure was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<DummyStruct[2]>::value, "An arry of dummy structures was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::vector<DummyStruct>>::value, "A vector of dummy structures was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::vector<DummyStruct>>::value, "A vector of DummyStruct was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::unique_ptr<DummyStruct>>::value, "A vector of DummyStruct was incorrectly classified as having a valid serializer");

struct MyRepeatedStructure {
  int* pv;

  static leap::descriptor GetDescriptor(void) {
    return {
      &MyRepeatedStructure::pv
    };
  }
};

struct MySimpleStructure {
  int a;
  int b;

  MySimpleStructure* selfReference;
  MyRepeatedStructure* x;
  MyRepeatedStructure* y;

  std::vector<int> someIntegers;
  std::vector<MyRepeatedStructure*> vectorOfChildren;
  std::string myString;
  std::wstring myWString;

  static leap::descriptor GetDescriptor(void) {
    return {
      &MySimpleStructure::a,
      &MySimpleStructure::b,
      &MySimpleStructure::selfReference,
      &MySimpleStructure::x,
      &MySimpleStructure::y,
      &MySimpleStructure::someIntegers,
      &MySimpleStructure::vectorOfChildren,
      &MySimpleStructure::myString,
      &MySimpleStructure::myWString
    };
  }
};

static_assert(
  leap::serializer_is_irresponsible<MySimpleStructure>::value,
  "A field which trivally requires allocation was not statically detected to require such a thing"
);

static_assert(
  std::is_same<
    std::result_of<decltype(&MySimpleStructure::GetDescriptor) ()>::type,
    leap::descriptor
  >::value,
  "Could not infer GetDescriptor return type"
);
static_assert(leap::has_getdescriptor<MySimpleStructure>::value, "GetDescriptor not detected on MySimpleStructure");

TEST_F(SerializationTest, VerifyTrivialDescriptor) {
  // Obtain the descriptor for the simple structure first, verify all properties
  leap::descriptor desc = MySimpleStructure::GetDescriptor();

  ASSERT_TRUE(desc.allocates()) << "MySimpleStructure is an allocating type and was not correctly identified as such";

  ASSERT_EQ(
    &desc.field_descriptors[0].serializer,
    &desc.field_descriptors[1].serializer
  ) << "Descriptors for two fields were not the same pointer even though they described identical types";
  ASSERT_EQ(
    &desc.field_descriptors[3].serializer,
    &desc.field_descriptors[4].serializer
  ) << "Descriptors for two fields were not the same pointer even though they described identical types";
}

TEST_F(SerializationTest, VerifyRoundTrip) {
  std::stringstream ss;

  // Serialize to an output stream
  {
    int v = 55;

    MyRepeatedStructure repeated;
    repeated.pv = &v;

    MyRepeatedStructure repeated2;
    repeated2.pv = &v;

    MySimpleStructure myStruct;
    myStruct.a = 101;
    myStruct.b = 102;
    myStruct.selfReference = &myStruct;
    myStruct.x = &repeated;
    myStruct.y = &repeated;
    myStruct.someIntegers.assign({1, 2, 3, 111});
    myStruct.vectorOfChildren.assign({&repeated, &repeated2});
    myStruct.myString = "Hello world!";
    myStruct.myWString = L"World hello!";

    leap::Serialize(ss, myStruct);
  }

  // Now deserialize
  auto allocation = leap::Deserialize<MySimpleStructure>(ss);

  // Validate everything came back:
  ASSERT_EQ(101, allocation->a) << "First serialized field did not deserialize properly";
  ASSERT_EQ(102, allocation->b) << "Second serialized field did not deserialize properly";
  ASSERT_EQ(allocation.get(), allocation->selfReference) << "Self-reference did not initialize as anticipated";
  ASSERT_EQ(allocation->x, allocation->y) << "Duplicated field did not appear as expected";
  ASSERT_EQ(4, allocation->someIntegers.size()) << "Expected number of integers were not deserialized";

  ASSERT_EQ(1, allocation->someIntegers[0]) << "Expected number of integers were not deserialized";
  ASSERT_EQ(2, allocation->someIntegers[1]) << "Expected number of integers were not deserialized";
  ASSERT_EQ(3, allocation->someIntegers[2]) << "Expected number of integers were not deserialized";
  ASSERT_EQ(111, allocation->someIntegers[3]) << "Expected number of integers were not deserialized";

  ASSERT_EQ(2UL, allocation->vectorOfChildren.size()) << "Expected two child nodes";
  ASSERT_EQ(allocation->x, allocation->vectorOfChildren[0]) << "First vector of children entry should have been equal to the established value x";
  ASSERT_NE(allocation->vectorOfChildren[0], allocation->vectorOfChildren[1]) << "Expected two unique items, but got one item instead";

  ASSERT_STREQ("Hello world!", allocation->myString.c_str()) << "Standard string round trip serialization did not work";
  ASSERT_STREQ(L"World hello!", allocation->myWString.c_str()) << "Standard string round trip serialization did not work";
}

struct HasOneMember {
  int foo = 10009;

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasOneMember::foo
    };
  }
};

TEST_F(SerializationTest, VectorOfFields) {
  std::vector<HasOneMember> a;
  a.resize(20);

  std::ostringstream os;
  leap::Serialize(os, a);

  auto b = leap::Deserialize<std::vector<HasOneMember>>(std::istringstream(os.str()));
  ASSERT_EQ(20UL, b->size()) << "Unexpected number of elements deserialized from a vector";
  for (auto& cur : *b)
    ASSERT_EQ(10009, cur.foo);
}

struct StructureVersionA {
  int base = 99;
  int fooA = 100;
  int fooB = 101;

  static leap::descriptor GetDescriptor(void) {
    return{
      &StructureVersionA::base,
      {1, &StructureVersionA::fooA},
      {2, &StructureVersionA::fooB}
    };
  }
};

struct StructureVersionB {
  int base = 199;
  int fooA = 200;
  int fooC = 201;

  static leap::descriptor GetDescriptor(void) {
    return {
      &StructureVersionB::base,
      {1, &StructureVersionB::fooA},
      {3, &StructureVersionB::fooC}
    };
  }
};

TEST_F(SerializationTest, NamedFieldCheck) {
  // Serialize structure A, deserialize as structure B
  StructureVersionA a;

  std::ostringstream os;
  leap::Serialize(os, a);

  auto b = leap::Deserialize<StructureVersionB>(std::istringstream(os.str()));

  // This value should have come over as a positional value from the original structure
  ASSERT_EQ(99, b->base);

  // We expect this value to have been carried over as a named value
  ASSERT_EQ(100, b->fooA);

  // And this value should have been left as a default because it was absent from the stream
  ASSERT_EQ(201, b->fooC);
}

struct HasArrayOfPointers {
  MyRepeatedStructure* mrs[4];

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasArrayOfPointers::mrs
    };
  }
};

TEST_F(SerializationTest, ArrayOfPointersTest) {
  std::ostringstream os;

  {
    int v = 999;

    MyRepeatedStructure mrs[2];
    mrs[0].pv = &v;
    mrs[1].pv = &v;

    HasArrayOfPointers aop;
    aop.mrs[0] = &mrs[0];
    aop.mrs[1] = &mrs[1];
    aop.mrs[2] = &mrs[1];
    aop.mrs[3] = nullptr;

    // Round-trip serialize:
    leap::Serialize(os, aop);
  }

  auto obj = leap::Deserialize<HasArrayOfPointers>(std::istringstream(os.str()));

  ASSERT_NE(obj->mrs[0], obj->mrs[1]) << "Object identity incorrectly aliased";
  ASSERT_EQ(obj->mrs[1], obj->mrs[2]) << "Object identity not correctly detected in array serialization";
  ASSERT_EQ(nullptr, obj->mrs[3]) << "Null entry not correctly deserialized in an array";
  ASSERT_EQ(obj->mrs[0]->pv, obj->mrs[1]->pv) << "Object identity not correctly detected in array serialization";
  ASSERT_EQ(999, *obj->mrs[0]->pv) << "Array object not correctly deserialized to the value it holds";
}

struct MyInlineType {
  int foo = 101;
  int bar = 102;
  std::string helloWorld = "Hello World!";

  static leap::descriptor GetDescriptor(void) {
    return{
      &MyInlineType::foo,
      &MyInlineType::bar,
      &MyInlineType::helloWorld
    };
  }
};

TEST_F(SerializationTest, InlineDeserializationTest) {
  std::stringstream ss;
  leap::Serialize(ss, MyInlineType());

  ASSERT_FALSE(MyInlineType::GetDescriptor().allocates()) << "Inline type improperly indicated that it requires an allocator";

  // Trivial short syntax check:
  MyInlineType houp;
  leap::Deserialize(ss, houp);

  ASSERT_EQ(101, houp.foo);
  ASSERT_EQ(102, houp.bar);
  ASSERT_EQ("Hello World!", houp.helloWorld);
}

TEST_F(SerializationTest, StlArray) {
  std::stringstream ss;
  leap::Serialize(ss, std::array<int, 5>{ { 1, 2, 3, 4, 5 } });

  std::array<int, 5> ary;
  leap::Deserialize(ss, ary);
  ASSERT_EQ(1, ary[0]);
  ASSERT_EQ(2, ary[1]);
  ASSERT_EQ(3, ary[2]);
}

TEST_F(SerializationTest, NativeArray) {
  //C-Array
  std::stringstream os;
  {
    int ary [] = {1, 2, 3, 4, 5};
    leap::Serialize(os, ary);
  }

  int ary[5];
  leap::Deserialize(os, ary);

  ASSERT_EQ(1, ary[0]);
  ASSERT_EQ(2, ary[1]);
  ASSERT_EQ(3, ary[2]);
}

struct HasEmbeddedArrayOfString {
public:
  std::string ary[5];

  static leap::descriptor GetDescriptor(void) {
    return{&HasEmbeddedArrayOfString::ary};
  }
};

TEST_F(SerializationTest, EmbeddedNativeArray) {
  HasEmbeddedArrayOfString heaos;
  heaos.ary[0] = "A";
  heaos.ary[1] = "B";
  heaos.ary[2] = "C";
  heaos.ary[3] = "D";
  heaos.ary[4] = "E";

  std::stringstream ss;
  leap::Serialize(ss, heaos);

  HasEmbeddedArrayOfString ret;
  leap::Deserialize(ss, ret);

  ASSERT_EQ("A", ret.ary[0]);
  ASSERT_EQ("B", ret.ary[1]);
  ASSERT_EQ("C", ret.ary[2]);
  ASSERT_EQ("D", ret.ary[3]);
  ASSERT_EQ("E", ret.ary[4]);
}

struct CountsTotalInstances {
  CountsTotalInstances(int value = 299):
    value(value)
  {
    s_count++;
  }
  ~CountsTotalInstances(void) {
    s_count--;
  }

  int value;

  static int s_count;

  static leap::descriptor GetDescriptor(void) {
    return{
      &CountsTotalInstances::value
    };
  }
};

int CountsTotalInstances::s_count = 0;

struct HasUniqueAndDumbPointer {
  CountsTotalInstances* dumb;
  std::unique_ptr<CountsTotalInstances> unique;

  static leap::descriptor GetDescriptor(void) {
    return {
      &HasUniqueAndDumbPointer::dumb,
      &HasUniqueAndDumbPointer::unique
    };
  }
};

TEST_F(SerializationTest, UniquePtrTest) {
  std::stringstream ss;
  {
    HasUniqueAndDumbPointer huadp;

    huadp.unique.reset(new CountsTotalInstances);
    huadp.dumb = huadp.unique.get();

    leap::Serialize(ss, huadp);
  }

  std::unique_ptr<CountsTotalInstances> cti;
  {
    // Recover value:
    auto deserialized = leap::Deserialize<HasUniqueAndDumbPointer>(ss);
    ASSERT_EQ(deserialized->dumb, deserialized->unique.get()) << "Unique pointer was not deserialized equivalently to a dumb pointer";

    // Values should be equivalent:
    ASSERT_EQ(deserialized->dumb, deserialized->unique.get()) << "Dumb pointer did not correctly deserialize to an identical unique pointer";

    // Now ensure that we can strip the unique_ptr correctly and that the deserialized instance is valid:
    cti = std::move(deserialized->unique);
  }

  // Ensure that the object was not deleted--should still have one count
  ASSERT_EQ(1UL, CountsTotalInstances::s_count) << "Expected one instance to escape deletion";
}

struct HasOnlyUniquePtrs {
  std::unique_ptr<CountsTotalInstances> u1;
  std::unique_ptr<CountsTotalInstances> u2;

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasOnlyUniquePtrs::u1,
      &HasOnlyUniquePtrs::u2
    };
  }
};

TEST_F(SerializationTest, CoerceUniquePtr) {
  std::stringstream ss;
  {
    HasOnlyUniquePtrs houp;
    houp.u1 = std::unique_ptr<CountsTotalInstances>(new CountsTotalInstances(23));
    houp.u2 = std::unique_ptr<CountsTotalInstances>(new CountsTotalInstances(244));
    leap::Serialize(ss, houp);
  }

  // Verify that the short syntax works:
  HasOnlyUniquePtrs houp;
  leap::Deserialize(ss, houp);

  // Now ensure that our unique pointers came back properly:
  ASSERT_EQ(23, houp.u1->value);
  ASSERT_EQ(244, houp.u2->value);
}

enum eMySimpleEnum {
  eMySimpleEnum_First,
  eMySimpleEnum_Second = 995
};

class HasAnEnumMember {
public:
  eMySimpleEnum member = eMySimpleEnum_Second;

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasAnEnumMember::member
    };
  }
};

TEST_F(SerializationTest, CanSerializeEnumsTest) {
  std::stringstream ss;
  leap::Serialize(ss, HasAnEnumMember());

  ASSERT_FALSE(MyInlineType::GetDescriptor().allocates()) << "Inline type improperly indicated that it requires an allocator";

  // Trivial short syntax check:
  HasAnEnumMember hem;
  leap::Deserialize(ss, hem);

  ASSERT_EQ(eMySimpleEnum_Second, hem.member) << "Deserialized enum-type member did not come back correctly";
}
TEST_F(SerializationTest, CanSerializeBoolsTest) {
  for (int i=0; i<10; i++) {
    bool condition = (i%2) == 0;
    std::stringstream ss;
    leap::Serialize(ss, condition);
    bool outCondition;
    leap::Deserialize(ss, outCondition);
    ASSERT_EQ(condition, outCondition);
  }
}

struct SimpleObject {
  SimpleObject(int val):
    i(val)
  {}

  SimpleObject(void){}

  static leap::descriptor GetDescriptor(void) {
    return {
      &SimpleObject::i
    };
  }

  int i;
};

TEST_F(SerializationTest, DeserializeToVector) {
  std::stringstream ss;

  for (int i=0; i<5; i++) {
    SimpleObject obj(i);
    leap::Serialize(ss, obj);
  }

  auto collection = leap::DeserializeToVector<SimpleObject>(ss);

  ASSERT_EQ(5, collection.size()) << "Didn't deserialize the same number of objects that were serialized";

  int counter = 0;
  for (const SimpleObject& obj : collection) {
    ASSERT_EQ(counter, obj.i) << "Didn't deserialize correctly";
    counter++;
  }
}

struct ConstMember {
  ConstMember(){}
  ConstMember(int* val):
    value(val)
  {}

  static leap::descriptor GetDescriptor(void) {
    return {
      &ConstMember::value
    };
  }

  const int* value;
};

TEST_F(SerializationTest, ConstSerialize) {
  int value = 42;
  std::stringstream ss;

  ConstMember mem(&value);
  leap::Serialize(ss, mem);

  auto otherMem = leap::Deserialize<ConstMember>(ss);

  ASSERT_EQ(42, *otherMem->value) << "Didn't deserialize const pointer member correctly";
}

TEST_F(SerializationTest, AlternateDescriptor) {
  leap::descriptor desc = {
    {&MySimpleStructure::a},
    {&MySimpleStructure::b}
  };

  std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
  {
    MySimpleStructure mss;
    mss.a = 1;
    mss.b = 2;
    mss.myString = "This will not be sent";
    {
      leap::OutputStreamAdapter ssa{ ss };
      leap::OArchiveImpl oArch(ssa);
      oArch.WriteObject(desc, &mss);
    }
  }

  {
    MySimpleStructure mss;
    leap::IArchiveImpl iArch(ss);
    iArch.ReadObject(desc, &mss, nullptr);
    ASSERT_EQ(mss.a, 1) << "Field a was not restored correctly";
    ASSERT_EQ(mss.b, 2) << "Field b was not restored correctly";
    ASSERT_TRUE(mss.myString.empty()) << "field myString was stored when it shouldn't have been!";
  }
}

struct MyNamedStructure {
  int a;
  int b;

  static leap::descriptor GetDescriptor(void) {
    return{
      { "Unsuitable for JSON", &MyNamedStructure::a },
      { 1, "suitable_for_json", &MyNamedStructure::b },
    };
  }
};
TEST_F(SerializationTest, FieldNameTest) {
  leap::descriptor desc = MyNamedStructure::GetDescriptor();

  ASSERT_STREQ(desc.field_descriptors[0].name, "Unsuitable for JSON")
    << "Field names were not assigned correctly";
  ASSERT_STREQ(desc.identified_descriptors.at(1).name, "suitable_for_json")
    << "Field names were not assigned correctly";
}

enum class SimpleEnum {
  One,
  Two,
  Four = 0x4444
};

TEST_F(SerializationTest, EnumClassTest) {
  std::stringstream ss;
  leap::Serialize(ss, SimpleEnum::Four);

  auto x = leap::Deserialize<SimpleEnum>(ss);
  ASSERT_EQ(SimpleEnum::Four, *x) << "Round-trip serialization of an enum class yielded an incorrect result";
}

namespace {
  enum MyType {
    One,
    Two
  };

  struct StructA {
    std::string hello = "Hello";
    std::map<std::string, std::string> mymap;
    uint64_t member1 = 2;
    MyType member2 = MyType::One;

    static leap::descriptor GetDescriptor(void) {
      return{
        &StructA::hello,
        &StructA::mymap,
        &StructA::member1,
        &StructA::member2
      };
    }
  };
  struct StructB {
    std::string world = "World";
    std::map<std::string, std::string> mymap;
    uint64_t member1 = 99;
    MyType member2 = MyType::Two;
    int member3 = 229;

    static leap::descriptor GetDescriptor(void) {
      return {
        &StructB::world,
        &StructB::mymap,
        &StructB::member1,
        &StructB::member2,
        { 1, &StructB::member3 }
      };
    }
  };
}

TEST_F(SerializationTest, VectorRoundTrip) {
  std::vector<StructB> all{ 4 };
  all[0].world = "A";
  all[1].world = "B";
  all[2].world = "C";
  all[3].world = "D";

  std::stringstream ss;
  leap::Serialize(ss, all);

  std::vector<StructB> next;
  leap::Deserialize(ss, next);

  for (size_t i = 0; i < all.size(); i++)
    ASSERT_EQ(all[i].member3, next[i].member3);
}

TEST_F(SerializationTest, FixedSizeBackwardsCompatCheck) {
  // Write as A
  std::stringstream ss;
  StructA a;
  a.mymap = std::map<std::string, std::string>{ { "a", "b" },{ "c", "d" } };
  leap::Serialize(ss, a);

  StructB b;
  leap::Deserialize(ss, b);

  ASSERT_EQ(a.hello, b.world);
  ASSERT_EQ(a.mymap, b.mymap);
  ASSERT_EQ(a.member1, b.member1);
  ASSERT_EQ(a.member2, b.member2);
}

TEST_F(SerializationTest, FixedSizeForwardsCompatCheck) {
  // Write as B
  std::stringstream ss;
  StructB b;
  b.mymap = std::map<std::string, std::string>{ { "hello", "world" } };
  leap::Serialize(ss, b);

  StructA a;
  leap::Deserialize(ss, a);

  ASSERT_EQ(b.world, a.hello);
  ASSERT_EQ(b.mymap, a.mymap);
  ASSERT_EQ(b.member1, a.member1);
  ASSERT_EQ(b.member2, a.member2);
}

namespace {
  class HasManyFloats {
  public:
    float a = 99.0f;
    double b = 99.0;
    long double c = 99.0;

    static leap::descriptor GetDescriptor(void) {
      return{
        &HasManyFloats::a,
        &HasManyFloats::b,
        &HasManyFloats::c
      };
    }
  };
}

TEST_F(SerializationTest, FloatingTypesTest) {
  HasManyFloats hmf;
  hmf.a = std::nextafter(10000.0f, 10000.1f);
  hmf.b = std::nextafter(20000.0, 20000.1);
  hmf.c = std::nextafter(30000.0L, 30000.1L);

  std::stringstream ss;
  leap::Serialize(ss, hmf);
  auto deserialized = leap::Deserialize<HasManyFloats>(ss);
  ASSERT_EQ(hmf.a, deserialized->a) << "Float datatype not properly round-trip serialized";
  ASSERT_EQ(hmf.b, deserialized->b) << "Double datatype not properly round-trip serialized";
  ASSERT_EQ(hmf.c, deserialized->c) << "Long double datatype not properly round-trip serialized";
}
