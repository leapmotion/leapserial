#pragma once
#include <type_traits>

namespace leap {
  class IArchive;
  class IArchiveRegistry;
  class OArchive;
  class OArchiveRegistry;

  template<typename T>
  struct serial_traits;

  /// <summary>
  /// Holds true if the specified type is irresponsible during deserialization
  /// </summary>
  /// <remarks>
  /// So-called "irresponsible types" are types which do not properly clean up their own memory.
  /// The pointer type itself is the fundamental irresponsible type.  Irresponsibility is a
  /// transitive property, thus classes containing a pointer type or other irresponsible classes
  /// are themselves irresponsible.
  ///
  /// Irresponsibility is inferred by looking for an IArchiveRegistry as the needed input argument
  /// to the serial_traits::deserialize routine.  It is via this interface that allocation takes
  /// place, thus its existence indicates an intention to perform such an allocation.
  /// </remarks>
  template<typename T>
  struct serializer_is_irresponsible
  {
    template<typename U>
    static std::false_type select(
      decltype(
        serial_traits<U>::deserialize(
          *static_cast<IArchive*>(nullptr),
          *static_cast<U*>(nullptr),
          0
        )
      )*
    );

    template<typename>
    static std::true_type select(...);

    static const bool value = decltype(select<T>(nullptr))::value;
  };

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
    virtual serial_primitive type(void) const = 0;

    /// <returns>
    /// The exact number of bytes that will be required in the serialize operation
    /// </returns>
    virtual uint64_t size(const OArchiveRegistry& ar, const void* pObj) const = 0;

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