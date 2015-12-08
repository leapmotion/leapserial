// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <cstddef>

namespace leap {
  struct field_serializer;

  struct IArrayReader {
    IArrayReader(const field_serializer& serializer) :
      serializer(serializer)
    {}

    /// <summary>
    /// The serializer for elements of the array
    /// </summary>
    const field_serializer& serializer;

    /// <returns>
    /// The ith item in the specified array
    /// </returns>
    virtual const void* get(size_t i) const = 0;

    /// <returns>
    /// The size of the array object
    /// </returns>
    virtual size_t size(void) const = 0;
  };

  struct IArrayAppender {
    IArrayAppender(const field_serializer& serializer) :
      serializer(serializer)
    {}

    /// <summary>
    /// The serializer for elements of the array
    /// </summary>
    const field_serializer& serializer;

    /// <summary>
    /// Reserves the specified number of entries in the array
    /// </summary>
    virtual void reserve(size_t n) = 0;

    /// <summary>
    /// Allocates a space for a new entry in the array
    /// </summary>
    /// <remarks>
    /// The returned space must be default-constructed or otherwise valid
    /// </remarks>
    virtual void* allocate(void) = 0;
  };
}
