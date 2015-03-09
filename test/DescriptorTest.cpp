#include "stdafx.h"
#include "Serializer.h"
#include <gtest/gtest.h>
#include <sstream>
#include <type_traits>

class SerializationTest:
  public testing::Test
{};

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
  std::ostringstream oss;

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

    leap::Serialize(oss, myStruct);
  }

  // Now deserialize
  auto allocation = leap::Deserialize<MySimpleStructure>(std::istringstream(oss.str()));

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
  std::ostringstream oss;

  {
    SimpleStruct ss;
    ss.myMap[0] = 555;
    ss.myMap[232] = 12;
    ss.myUnorderedMap[29] = 44;
    ss.myUnorderedMap[30] = 4014;
    leap::Serialize(oss, ss);
  }

  // Deserialize, make sure map entries are in the right place
  auto x = leap::Deserialize<SimpleStruct>(std::istringstream(oss.str()));
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
  std::ostringstream oss;

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

    leap::Serialize(oss, head);
  }

  auto ll = leap::Deserialize<MyLinkedList<int>>(std::istringstream(oss.str()));
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
  std::ostringstream oss;
  {
    MyLinkedList<int> ll;
    ll.next = &ll;
    ll.prev = nullptr;
    ll.value = 192;

    leap::Serialize(oss, ll);
  }

  // Deserialize, ensure we get null back
  {
    auto ll = leap::Deserialize<MyLinkedList<int>>(std::istringstream(oss.str()));
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
  std::ostringstream os;
  leap::Serialize(os, root);

  auto swef = leap::Deserialize<StructureWithEmbeddedField>(std::istringstream(os.str()));

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

TEST_F(SerializationTest, ExpectedByteCount) {
  MySimpleStructure mss;
  mss.a = 909;
  mss.b = -1;
  mss.myString = "Hello world";
  mss.myWString = L"Hello world again!";
  mss.selfReference = nullptr;
  mss.x = nullptr;
  mss.y = nullptr;

  // Predict the number of bytes required to serialize:
  leap::descriptor desc = MySimpleStructure::GetDescriptor();
  uint64_t ncb = desc.size(&mss);

  // Serialize first
  std::ostringstream os;
  leap::Serialize(os, mss);

  // Verify size equivalence.  We have exactly two bytes of slack because the outermost object
  // is serialized with a type, identifier, and length field, and this takes up two bytes.
  std::string buf = os.str();
  ASSERT_EQ(buf.size(), ncb + 2);
}

TEST_F(SerializationTest, VarintDoubleCheck) {
  std::ostringstream os;
  leap::OArchiveImpl oarch(os);
  oarch.WriteVarint(150);
  
  std::string buf = os.str();
  ASSERT_STREQ("\x96\x01", buf.c_str()) << "Varint serialization is incorrect";

  // Read operation should result in the same value
  std::istringstream is(buf);
  leap::internal::Allocation<std::string> alloc;
  leap::IArchiveImpl iarch(is, alloc);
  ASSERT_EQ(150, iarch.ReadVarint()) << "Read of a varint didn't return the original value";
}

TEST_F(SerializationTest, VarintSizeExpectationCheck) {
#ifndef _MSC_VER
  ASSERT_EQ(
    29,
    __builtin_clzll(0x7FFFFFFFF)
  );
  ASSERT_EQ(
    28,
    __builtin_clzll(0x800000000)
  );
#endif

  ASSERT_EQ(
    10,
    leap::OArchive::VarintSize(-1)
  ) << "-1 should have maxed out the varint size requirement";

  ASSERT_EQ(
    2,
    leap::OArchive::VarintSize(128)
  ) << "Boundary case failure";

  ASSERT_EQ(
    6,
    leap::OArchive::VarintSize(0x800000000ULL)
  ) << "Boundary case failure";

  ASSERT_EQ(
    5,
    leap::OArchive::VarintSize(0x7FFFFFFFFULL)
  ) << "Boundary case failure";
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

