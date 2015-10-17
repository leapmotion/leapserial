// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#include "stdafx.h"
#include "optional.h"
#include <gtest/gtest.h>
#include <sstream>

TEST(OptionalTest, Basics) {
  leap::optional<std::shared_ptr<int>> optional;
  ASSERT_FALSE(optional) << "Optional field was incorrectly initialized to a valid state";
  optional = std::make_shared<int>(55);
  ASSERT_EQ(55, **optional) << "Optional field did not hold an optional type properly";

  leap::optional<std::shared_ptr<int>> copy;
  copy = optional;
  ASSERT_EQ(55, **copy) << "Copy assignment did not copy over properly";
  copy = optional;
  ASSERT_EQ(55, **copy) << "Copy to an existing assignment did not copy over properly";
  ASSERT_EQ(2, copy->use_count()) << "Copy assignment did not properly invoke a destructor";
}

TEST(OptionalTest, Primitives) {
  leap::optional<int> optInt;
  leap::optional<float> optFloat;
  leap::optional<double> optDouble;

  optInt = 9;
  optFloat = 1.0f;
  optDouble = 929.4949;

  ASSERT_EQ(9, *optInt);
  ASSERT_FLOAT_EQ(1.0f, *optFloat);
  ASSERT_DOUBLE_EQ(929.4949, *optDouble);
}

TEST(OptionalTest, Pointers) {
  std::unique_ptr<int> data{ new int { 1001 } };
  leap::optional<int*> val = data.get();
  ASSERT_EQ(data.get(), *val);
  ASSERT_EQ(1001, **val);
}

TEST(OptionalTest, DtorTest) {
  auto val = std::make_shared<int>(292);
  leap::optional<std::shared_ptr<int>> optVal;
  optVal = val;

  ASSERT_EQ(2UL, val.use_count()) << "Value was not copied properly into an optional field";
  ASSERT_EQ(292, **optVal) << "Optional value did not point to the correct destination";
  optVal = {};
  ASSERT_TRUE(val.unique()) << "Optional value did not correctly release resources on destruction";
}

TEST(OptionalTest, MovementTest) {
  std::unique_ptr<int> val { new int{111} };
  leap::optional<std::unique_ptr<int>> optVal;
  optVal = std::move(val);
  ASSERT_EQ(111, **optVal) << "Optional value did not correctly receive a move-only value";
}

TEST(OptionalTest, PlacementConstruct) {
  leap::optional<std::shared_ptr<int>> optVal;
  optVal.emplace(new int{ 1001 });
  ASSERT_EQ(1001, **optVal) << "Optional value did not correctly placement construct a unique pointer";

  auto val = *optVal;
  ASSERT_EQ(2UL, val.use_count());
  optVal.emplace(new int{ 2001 });
  ASSERT_EQ(2001, **optVal) << "Optional value did not take on a new value properly in emplace while holding a value";
  ASSERT_TRUE(val.unique()) << "Optional value did not correctly release its existing value on emplace construction";
}

TEST(OptionalTest, Swap) {
  leap::optional<std::unique_ptr<int>> optValA{ std::unique_ptr<int>{ new int{ 7774 } } };
  leap::optional<std::unique_ptr<int>> optValB = std::move(optValA);
  ASSERT_FALSE(optValA) << "Moved optional not properly nullified";
  ASSERT_TRUE(optValB) << "R-value initialized optional was unexpectedly empty";

  optValB.swap(optValB);
  ASSERT_TRUE(optValB) << "Incorrect reflexive swap behavior";
  ASSERT_EQ(7774, **optValB) << "Incorrect reflexive swap behavior";

  leap::optional<std::unique_ptr<int>> optValC;
  std::swap(optValA, optValB);
  std::swap(optValA, optValC);
  ASSERT_FALSE(optValA);
  ASSERT_FALSE(optValB);
  ASSERT_TRUE(optValC);
}