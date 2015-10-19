// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "LeapSerial.h"
#include <gtest/gtest.h>
#include <array>
#include <sstream>
#include <type_traits>

static_assert(leap::serializer_is_irresponsible<int*>::value, "Irresponsible allocator was not correctly inferred");

struct DummyStruct {};
static_assert(!leap::has_serializer<std::unique_ptr<DummyStruct>>::value, "A unique pointer to a dummy structure was incorrectly classified as having a valid serializer");

struct HasThreeSharedPointers {
  std::shared_ptr<int> a;
  std::shared_ptr<int> b;
  std::shared_ptr<int> c;

  static leap::descriptor GetDescriptor(void) {
    return{
      &HasThreeSharedPointers::a,
      &HasThreeSharedPointers::b,
      &HasThreeSharedPointers::c
    };
  }
};

TEST(PointerTest, CanSerializeSharedPtrs) {
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

TEST(PointerTest, SharedPtrOfSharedPtrs) {
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

TEST(PointerTest, VectorOfSharedPointers) {
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
    return{
      &MyLinkedList::value,
      &MyLinkedList::prev,
      &MyLinkedList::next
    };
  }
};

TEST(PointerTest, VerifyDoublyLinkedList) {
  std::stringstream ss;

  {
    MyLinkedList<int> head{ 1 };
    MyLinkedList<int> middle{ 2 };
    MyLinkedList<int> tail{ 3 };
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


TEST(PointerTest, NullPointerDereferenceCheck) {
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
    return{
      &StructureWithEmbeddedField::embedded
    };
  }
};

TEST(PointerTest, EmbeddedFieldCheck) {
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