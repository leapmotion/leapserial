#pragma once

namespace leap {
  class IArchive;
  class IArchiveRegistry;
  class OArchive;
  class OArchiveRegistry;

  /// <summary>
  /// Interface used for general purpose field serialization
  /// </summary>
  struct field_serializer {
    /// <returns>
    /// True if this serializer or any child serializer requires memory management
    /// </returns>
    virtual bool allocates(void) const = 0;

    /// <returns>
    /// The field type, used for serial operations
    /// </returns>
    virtual serial_type type(void) const = 0;

    /// <returns>
    /// The exact number of bytes that will be required in the serialize operation
    /// </returns>
    virtual uint64_t size(const void* pObj) const = 0;

    /// <summary>
    /// Serializes the object into the specified buffer
    /// </summary>
    virtual void serialize(OArchiveRegistry& ar, const void* pObj) const = 0;

    /// <summary>
    /// Deserializes the object from the specified archive
    /// </summary>
    /// <param name="ar">The input archive</param>
    /// <param name="pObj">A pointer to the memory receiving the deserialized object</param>
    /// <param name="ncb">The maximum number of bytes to be read</param>
    /// <remarks>
    /// The "ncb" field is only valid if the type of this object is serial_type::string.
    /// </remarks>
    virtual void deserialize(IArchiveRegistry& ar, void* pObj, uint64_t ncb) const = 0;
  };

}