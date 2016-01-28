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

    /// <summary>
    /// Reports whether the size of the element type is immutable
    /// </summary>
    /// <returns>The immutable size, or zero if no such size exists</returns>
    /// <remarks>
    /// If elements in the array have a immutable size derived from the type, this value is nonzero.
    /// Broadly speaking, all primitive types have a immutable size, and almost no user types have
    /// a immutable size.
    ///
    /// The cases where a user type is considered immutable are cases where the user type represents
    /// a fixed concept; in the case where more complex concepts are desired, the old type would be
    /// totally deprecated rather than being extended.  For instance, a type to represent a vector3
    /// might be given an immutable size by the user, because if the user desires a fourth entry,
    /// they would simply create a new vector4 type.
    ///
    /// Note that the concept of a immutable size refers to a property of the type itself, NOT
    /// the serialized representation of the type.  For instance, while an int32_t is a
    /// type that has a immutable_size, the size of an instance of this type on the wire depends
    /// on the archiver's encoding choices.
    /// </remarks>
    virtual size_t immutable_size(void) const = 0;

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
