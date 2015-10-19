// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <cstddef>

namespace leap {
  struct field_serializer;

  /// <summary>
  /// An interface to describe read operations related to a dictionary
  /// </summary>
  struct IDictionaryReader {
    IDictionaryReader(const field_serializer& key_serializer, const field_serializer& value_serializer) :
      key_serializer(key_serializer),
      value_serializer(value_serializer)
    {}

    /// <summary>
    /// The serializer for keys in this dictionary
    /// </summary>
    const field_serializer& key_serializer;

    /// <returns>
    /// The serializer for values in this dictionary
    /// </returns>
    const field_serializer& value_serializer;

    /// <returns>
    /// The size of the passed dictionary object
    /// </returns>
    virtual size_t size(void) const = 0;

    /// <summary>
    /// Advances to the next entry in the dictionary.
    /// </summary>
    /// <returns>False if there were no other elements</returns>
    /// <remarks>
    /// This method must be called before the first call to `key` or `value`
    /// </remarks>
    virtual bool next(void) = 0;

    /// <returns>The currently enumerated key</returns>
    virtual const void* key(void) const = 0;

    /// <returns>The currently enumerated value</returns>
    virtual const void* value(void) const = 0;
  };

  /// <summary>
  /// An interface to describe read operations related to a dictionary
  /// </summary>
  struct IDictionaryInserter {
    /// <returns>
    /// </returns>
    IDictionaryInserter(const field_serializer& key_serializer, const field_serializer& value_serializer) :
      key_serializer(key_serializer),
      value_serializer(value_serializer)
    {}

    /// <summary>
    /// The serializer for keys in this dictionary
    /// </summary>
    const field_serializer& key_serializer;

    /// <returns>
    /// The serializer for values in this dictionary
    /// </returns>
    const field_serializer& value_serializer;

    /// <summary>
    /// Retrieves a space that may be used to insert a key
    /// </summary>
    virtual void* key(void) = 0;

    /// <summary>
    /// Creates a space for the specified key in the dictionary
    /// </summary>
    /// <returns>The value counterpart for the key</returns>
    /// <remarks>
    /// The key space is reset on this insert call.
    /// </remarks>
    virtual void* insert(void) = 0;
  };
}
