// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>
#include <array>
#include <sstream>
#include <type_traits>

class SerializationTest:
  public testing::Test
{};

// Need to ensure that all builtins are correctly recognized
static_assert(leap::has_serializer<std::string>::value, "std::string");
static_assert(leap::has_serializer<std::vector<int>>::value, "Vector of ints");
static_assert(leap::has_serializer<std::vector<std::string>>::value, "Vector of strings");
static_assert(leap::has_serializer<std::map<int, int>>::value, "Map of primitives");
static_assert(leap::has_serializer<std::map<std::string, std::string>>::value, "Map of nonprimitives");
static_assert(leap::has_serializer<std::unordered_map<int, int>>::value, "Unordered map of primitives");
static_assert(leap::has_serializer<std::unordered_map<std::string, std::string>>::value, "Unordered map of nonprimitives");

struct DummyStruct {};
static_assert(!leap::has_serializer<DummyStruct>::value, "A dummy structure was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<DummyStruct[2]>::value, "An arry of dummy structures was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::vector<DummyStruct>>::value, "A vector of dummy structures was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::unique_ptr<DummyStruct>>::value, "A unique pointer to a dummy structure was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::map<int, DummyStruct>>::value, "A map of DummyStruct was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::unordered_map<int, DummyStruct>>::value, "An unordered_map of DummyStruct was incorrectly classified as having a valid serializer");
static_assert(!leap::has_serializer<std::vector<DummyStruct>>::value, "A vector of DummyStruct was incorrectly classified as having a valid serializer");

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
static_assert(leap::internal::has_getdescriptor<MySimpleStructure>::value, "GetDescriptor not detected on MySimpleStructure");

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

struct SimpleStruct {
  std::map<int, int> myMap;
  std::unordered_map<int, int> myUnorderedMap;

  static leap::descriptor GetDescriptor(void) {
    return {
      &SimpleStruct::myMap,
      &SimpleStruct::myUnorderedMap
    };
  }
};

TEST_F(SerializationTest, CanSerializeMaps) {
  std::stringstream ss;

  {
    SimpleStruct x;
    x.myMap[0] = 555;
    x.myMap[232] = 12;
    x.myUnorderedMap[29] = 44;
    x.myUnorderedMap[30] = 4014;
    leap::Serialize(ss, x);
  }

  // Deserialize, make sure map entries are in the right place
  auto x = leap::Deserialize<SimpleStruct>(ss);
  ASSERT_EQ(2UL, x->myMap.size()) << "Map has an incorrect number of elements";
  ASSERT_EQ(555, x->myMap[0]);
  ASSERT_EQ(12, x->myMap[232]);

  // Same for unordered map
  ASSERT_EQ(2UL, x->myUnorderedMap.size()) << "Unordered map has an incorrect number of elements";
  ASSERT_EQ(44, x->myUnorderedMap[29]);
  ASSERT_EQ(4014, x->myUnorderedMap[30]);
}

template<typename T>
struct MyLinkedList {
  MyLinkedList(const T& v = T()) : value(v), prev(nullptr), next(nullptr) {}
  void InsertAfter(MyLinkedList* last) {
    if (!last)
      return;
    prev = last;
    next = prev->next;
    if (next) {
      next->prev = this;
    }
    prev->next = this;
  }

  T value;
  MyLinkedList* prev;
  MyLinkedList* next;

  static leap::descriptor GetDescriptor(void) {
    return {
      &MyLinkedList::value,
      &MyLinkedList::prev,
      &MyLinkedList::next
    };
  }
};

TEST_F(SerializationTest, VerifyDoublyLinkedList) {
  std::stringstream ss;

  {
    MyLinkedList<int> head{1};
    MyLinkedList<int> middle{2};
    MyLinkedList<int> tail{3};
    middle.InsertAfter(&head);
    tail.InsertAfter(&middle);
    // Doesn't support null right now, so add loops
    head.prev = &tail;
    tail.next = &head;

    // Verfiy that we are creating what we think we are creating:
    ASSERT_EQ(1, head.value);
    ASSERT_TRUE(head.prev != nullptr);
    ASSERT_TRUE(head.next != nullptr);
    ASSERT_EQ(3, head.prev->value);
    ASSERT_EQ(2, head.next->value);
    ASSERT_EQ(2, middle.value);
    ASSERT_TRUE(middle.prev != nullptr);
    ASSERT_TRUE(middle.next != nullptr);
    ASSERT_EQ(3, middle.next->value);
    ASSERT_EQ(1, middle.prev->value);
    ASSERT_TRUE(tail.prev != nullptr);
    ASSERT_TRUE(tail.next != nullptr);
    ASSERT_EQ(3, tail.value);
    ASSERT_EQ(1, tail.next->value);
    ASSERT_EQ(2, tail.prev->value);

    leap::Serialize(ss, head);
  }

  auto ll = leap::Deserialize<MyLinkedList<int>>(ss);
  MyLinkedList<int>& head = *ll;
  ASSERT_TRUE(head.next != nullptr);
  MyLinkedList<int>& middle = *head.next;
  ASSERT_TRUE(middle.next != nullptr);
  MyLinkedList<int>& tail = *middle.next;

  ASSERT_EQ(1, head.value);
  ASSERT_TRUE(head.next != nullptr);
  ASSERT_TRUE(head.prev != nullptr);
  ASSERT_EQ(3, head.prev->value);
  ASSERT_EQ(2, head.next->value);
  ASSERT_EQ(2, middle.value);
  ASSERT_TRUE(middle.prev != nullptr);
  ASSERT_TRUE(middle.next != nullptr);
  ASSERT_EQ(3, middle.next->value);
  ASSERT_EQ(1, middle.prev->value);
  ASSERT_EQ(3, tail.value);
  ASSERT_TRUE(tail.prev != nullptr);
  ASSERT_TRUE(tail.next != nullptr);
  ASSERT_EQ(1, tail.next->value);
  ASSERT_EQ(2, tail.prev->value);
}

TEST_F(SerializationTest, NullPointerDereferenceCheck) {
  std::stringstream ss;
  {
    MyLinkedList<int> ll;
    ll.next = &ll;
    ll.prev = nullptr;
    ll.value = 192;

    leap::Serialize(ss, ll);
  }

  // Deserialize, ensure we get null back
  {
    auto ll = leap::Deserialize<MyLinkedList<int>>(ss);
    ASSERT_EQ(192, ll->value);
    ASSERT_EQ(ll.get(), ll->next);
    ASSERT_EQ(nullptr, ll->prev);
  }
}

struct StructureWithEmbeddedField {
  MyLinkedList<int> embedded;

  static leap::descriptor GetDescriptor(void) {
    return {
      &StructureWithEmbeddedField::embedded
    };
  }
};

TEST_F(SerializationTest, EmbeddedFieldCheck) {
  StructureWithEmbeddedField root;
  MyLinkedList<int> top;

  root.embedded.value = 1029;
  root.embedded.prev = &root.embedded;
  root.embedded.next = &top;

  top.value = 292;
  top.prev = &root.embedded;
  top.next = nullptr;

  // Round-trip serialization:
  std::stringstream ss;
  leap::Serialize(ss, root);

  auto swef = leap::Deserialize<StructureWithEmbeddedField>(ss);

  ASSERT_EQ(1029, swef->embedded.value);
  ASSERT_EQ(292, swef->embedded.next->value);
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

TEST_F(SerializationTest, PostInitTest) {
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
  leap::Serialize(ss, std::array<int, 5>{1, 2, 3, 4, 5});

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

static_assert(leap::serializer_is_irresponsible<int*>::value, "Irresponsible allocator was not correctly inferred");
static_assert(leap::serializer_is_irresponsible<std::vector<int*>>::value, "Irresponsible allocator was not correctly inferred");
static_assert(leap::serializer_is_irresponsible<std::map<std::string, std::vector<int*>>>::value, "Irresponsible allocator was not correctly inferred");
static_assert(!leap::serializer_is_irresponsible<std::vector<int>>::value, "Irresponsibility incorrectly inferred in a responsible allocator");
static_assert(!leap::serializer_is_irresponsible<std::map<std::string, std::vector<int>>>::value, "Irresponsibility incorrectly inferred in a responsible allocator");

TEST_F(SerializationTest, ComplexMapOfStringToVectorTest) {
  std::stringstream ss;

  // Serialize the complex map type first
  {
    std::map<std::string, std::vector<int>> mm;
    mm["a"] = std::vector<int> {1, 2, 3};
    mm["b"] = std::vector<int> {4, 5, 6};
    mm["c"] = std::vector<int> {7, 8, 9};

    leap::Serialize(ss, mm);
  }

  // Try to get it back:
  std::map<std::string, std::vector<int>> mm;
  leap::Deserialize(ss, mm);

  ASSERT_EQ(1UL, mm.count("a")) << "Deserialized map was missing an essential member";
  ASSERT_EQ(1UL, mm.count("b")) << "Deserialized map was missing an essential member";
  ASSERT_EQ(1UL, mm.count("c")) << "Deserialized map was missing an essential member";

  const auto& a = mm["a"];
  ASSERT_EQ(3UL, a.size()) << "Vector was sized incorrectly";

  const auto& c = mm["c"];
  ASSERT_EQ(3UL, c.size()) << "Vector was sized incorrectly";
  ASSERT_EQ(7, c[0]);
  ASSERT_EQ(8, c[1]);
  ASSERT_EQ(9, c[2]);
}

struct HasThreeSharedPointers {
  std::shared_ptr<int> a;
  std::shared_ptr<int> b;
  std::shared_ptr<int> c;

  static leap::descriptor GetDescriptor(void) {
    return {
      &HasThreeSharedPointers::a,
      &HasThreeSharedPointers::b,
      &HasThreeSharedPointers::c
    };
  }
};

TEST_F(SerializationTest, CanSerializeSharedPtrs) {
  HasThreeSharedPointers htsp;
  htsp.a = std::make_shared<int>(101);
  htsp.b = std::make_shared<int>(199);
  htsp.c = htsp.a;

  // Round trip
  std::stringstream ss;
  leap::Serialize(ss, htsp);

  HasThreeSharedPointers res;
  leap::Deserialize(ss, res);
  ASSERT_EQ(res.a, res.c) << "Aliased shared pointer was not correctly detected in a second pass";
  ASSERT_EQ(101, *res.a) << "Aliased shared pointer was not correctly stored";
  ASSERT_EQ(199, *res.b) << "Ordinary shared pointer was not correctly stored";

  ASSERT_EQ(2UL, res.a.use_count()) << "Incorrect use count detected";
}

TEST_F(SerializationTest, SharedPtrOfSharedPtrs) {
  std::shared_ptr<HasThreeSharedPointers> sphtsp = std::make_shared<HasThreeSharedPointers>();
  sphtsp->a = std::make_shared<int>(101);
  sphtsp->b = std::make_shared<int>(199);
  sphtsp->c = sphtsp->a;

  // Round trip
  std::stringstream ss;
  leap::Serialize(ss, sphtsp);

  std::shared_ptr<HasThreeSharedPointers> res;
  leap::Deserialize(ss, res);
  ASSERT_EQ(res->a, res->c) << "Aliased shared pointer was not correctly detected in a second pass";
  ASSERT_EQ(101, *res->a) << "Aliased shared pointer was not correctly stored";
  ASSERT_EQ(199, *res->b) << "Ordinary shared pointer was not correctly stored";

  ASSERT_EQ(2UL, res->a.use_count()) << "Incorrect use count detected";
}

TEST_F(SerializationTest, VectorOfSharedPointers) {
  std::vector<std::shared_ptr<int>> vosp;

  // Fill the vector with originals:
  for (int i = 10; i--;)
    vosp.push_back(std::make_shared<int>(i));

  // Create some duplicates:
  vosp.resize(14);
  vosp[10] = vosp[4];
  vosp[11] = vosp[4];
  vosp[12] = vosp[5];
  vosp[13] = vosp[9];

  // Round trip, as per usual
  std::stringstream ss;
  leap::Serialize(ss, vosp);

  std::vector<std::shared_ptr<int>> ret;
  leap::Deserialize(ss, ret);
  ASSERT_EQ(14, ret.size()) << "Incorrect number of deserialized entries";

  const char* msg = "Aliased entries did not correctly match when deserialized";
  ASSERT_EQ(ret[4], ret[10]) << msg;
  ASSERT_EQ(ret[4], ret[11]) << msg;
  ASSERT_EQ(ret[5], ret[12]) << msg;
  ASSERT_EQ(ret[9], ret[13]) << msg;

  ASSERT_EQ(3UL, ret[4].use_count()) << "Expected three aliases when deserializing";
  ASSERT_EQ(2UL, ret[5].use_count()) << "Expected two aliases when deserializing";
  ASSERT_EQ(2UL, ret[9].use_count()) << "Expected two aliases when deserializing";
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

TEST_F(SerializationTest, InheritanceTest) {
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

TEST_F(SerializationTest, DiamondInheritanceTest) {
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
      leap::OArchiveImpl oArch(ss);
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

TEST_F(SerializationTest, AccessorMethodTest) {
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

TEST_F(SerializationTest, GetStaticAsserts)
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

TEST_F(SerializationTest, SetStaticAsserts)
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

TEST_F(SerializationTest, LambdaMethodTest) {
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

  leap::OArchiveImpl(ss).WriteObject(desc, &st);

  MyAccessorStruct stIn;

  leap::IArchiveImpl(ss).ReadObject(desc, &stIn, nullptr);

  ASSERT_EQ(st.a, stIn.a) << "Field A was not serialized correctly";
  ASSERT_EQ(st.b, stIn.b) << "Field B was not serialized correctly";
  ASSERT_EQ(st.c, stIn.c) << "Field C was not serialized correctly";
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

struct HasChronoMembers {
  std::chrono::seconds one{ 4929 };
  std::chrono::nanoseconds two{ 4929 };
  std::chrono::duration<short, std::ratio<24 * 60 * 60, 1>> three{ 2 };
  std::chrono::duration<float> four{ 1.2f };

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasChronoMembers::one,
      &HasChronoMembers::two,
      &HasChronoMembers::three,
      &HasChronoMembers::four
    };
  }
};

TEST_F(SerializationTest, DurationMembers) {
  const HasChronoMembers hcm{};

  std::stringstream ss;
  leap::Serialize(ss, hcm);
  auto re = leap::Deserialize<HasChronoMembers>(ss);

  ASSERT_EQ(hcm.one, re->one) << "Seconds did not deserialize properly";
  ASSERT_EQ(hcm.two, re->two) << "Nanoseconds did not deserialize properly";
  ASSERT_EQ(hcm.three, re->three) << "Custom days duration did not deserialize properly";
  ASSERT_EQ(hcm.four, re->four) << "Floating point duration did not deserialize properly";
}
