#pragma once
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>

namespace leap {
  struct create_delete;
  struct descriptor;
  struct field_serializer;
  class IArchive;
  class IArchiveRegistry;
  class OArchive;
  class OArchiveRegistry;

  //This type is part of the interface, so is subject to change and therefore
  //should never be serialized directly.
  enum class serial_primitive {
    ignored = -1,
    boolean = 0,
    i8,
    i16,
    i32,
    i64,
    f32,
    f64,
    array,
    string,
    map
  };

  /// <summary>
  /// Represents a stateful serialization archive structure
  /// </summary>
  class OArchive {
  public:
    /// <summary>
    /// Creates a new output archive based on the specified stream
    /// </summary>
    virtual ~OArchive(void) {}

    /// <summary>
    /// Writes the specified bytes to the output stream, optionally prefixed by the size of the stream.
    /// </summary>
    virtual void WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize = false) = 0;
    
    /// <summary>
    /// Writes the specified bytes as a string.
    /// </summary>
    /// <param name="charCount"> The number of characters </param>
    /// <param name="charSize"> The number of bytes per character</param>
    virtual void WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) = 0;

    /// <summary>
    /// Writes the specified boolean value to the output stream
    /// </sumamry>
    virtual void WriteBool(bool value) = 0;

    /// <summary>
    /// Writes the specified integer to the output stream
    /// </summary>
    /// <param name="value">The actual integer to be written</param>
    /// <param name="ncb">
    /// The number of bytes maximumum that are set in value
    /// </param>
    /// <remarks>
    /// It is an error for (value & ~(1 << (ncb * 8))) to be nonzero.
    /// </remarks>
    virtual void WriteInteger(int64_t value, size_t ncb) = 0;

    // Convenience overloads:
    virtual void WriteInteger(int8_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(uint8_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(int16_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(uint16_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(int32_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(uint32_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(int64_t value) { WriteInteger(value, sizeof(value)); }
    virtual void WriteInteger(uint64_t value) { WriteInteger((int64_t)value, sizeof(value)); }
    
    virtual void WriteFloat(float value) { WriteByteArray(&value, sizeof(float)); }
    virtual void WriteFloat(double value) { WriteByteArray(&value, sizeof(double)); }
    
    /// <summary>
    /// Returns the number of bytes that will be used to write the specified integer
    /// </summary>
    virtual size_t SizeInteger(int64_t value, size_t ncb) const = 0;
    virtual size_t SizeFloat(float value) const = 0;
    virtual size_t SizeFloat(double value) const = 0;
    virtual size_t SizeBool(bool value) const = 0;
    virtual size_t SizeString(const void* pBuf, uint64_t ncb, size_t charSize) const = 0;

    template<class T>
    size_t SizeInteger(T value) {
      return SizeInteger(value, sizeof(T));
    }
  };

  /// <summary>
  /// "Regstry" interface for serialization operations that need to serialize foreign object references
  /// </summary>
  class OArchiveRegistry:
    public OArchive
  {
  public:
    virtual ~OArchiveRegistry(void) {}
    
    /// <summary>
    /// Registers an object for serialization, returning the ID that will be given to the object
    /// </summary>
    virtual void WriteObjectReference(const field_serializer& serializer, const void* pObj) = 0;
    virtual uint64_t SizeObjectReference(const field_serializer& serializer, const void* pObj) const = 0;

    /// <summary>
    /// Writes an object directly to the stream - used as the root call to serialize an object.
    /// </summary>
    virtual void WriteObject(const field_serializer& serializer, const void* pObj) = 0;
    
    /// <summary>
    /// Writes a descriptor to the stream
    /// </summary>
    virtual void WriteDescriptor(const descriptor& descriptor, const void* pObj) = 0;
    virtual uint64_t SizeDescriptor(const descriptor& descriptor, const void* pObj) const = 0;

    /// <summary>
    /// Writes out an array of entries
    /// </summary>
    virtual void WriteArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) = 0;
    virtual uint64_t SizeArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) const = 0;

    /// <summary>
    /// Writes out an array of entries
    /// </summary>
    virtual void WriteDictionary(
      uint64_t n,
      const field_serializer& keyDesc,
      std::function<const void*()> keyEnumerator,
      const field_serializer& valueDesc,
      std::function<const void*()> valueEnumerator
     ) = 0;

    virtual uint64_t SizeDictionary(
      uint64_t n,
      const field_serializer& keyDesc,
      std::function<const void*()> keyEnumerator,
      const field_serializer& valueDesc,
      std::function<const void*()> valueEnumerator
      ) const = 0;

  
  };

  class IArchive {
  public:
    /// <summary>
    /// Creates a new output archive based on the specified stream
    /// </summary>
    virtual ~IArchive(void) {}

    struct ReleasedMemory {
      ReleasedMemory(void* pObject, std::shared_ptr<void> pContext) :
        pObject(pObject),
        pContext(pContext)
      {}

      // The actually released object
      void* pObject;

      // A context block associated with the released object.  This context block must be a shared pointer
      // in order to ensure proper memory cleanup takes place.
      std::shared_ptr<void> pContext;
    };

    /// <summary>
    /// Identical to IArchiveRegistry::Lookup, except this prevents the IArchive from delegating delete responsibilities to the allocation
    /// </summary>
    /// <param name="pfnAlloc">The routine to be used for allocation</param>
    /// <param name="serializer">The descriptor to use to deserialize this object, if necessary</param>
    /// <returns>A ReleasedMemory structure</returns>
    /// <remarks>
    /// This method is permitted in the IArchive class because it does not require that the archive take responsibility
    /// for the proffered object, and in fact can allow the IArchive to abdicate responsibility for a type for which it
    /// may have been formerly responsible.
    /// </remarks>
    virtual ReleasedMemory Release(ReleasedMemory(*pfnAlloc)(), const field_serializer& serializer, uint32_t objId) = 0;

    /// <summary>
    /// Identical to ReadObjectReference, but does not claim responsiblity for cleaning up the allocated memory.
    /// </summary>
    virtual ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) = 0;

    /// <summary>
    /// Corrolary routine to Release
    /// </summary>
    /// <returns>True if the specified object identifier refers to a block of memory that has already been released</returns>
    virtual bool IsReleased(uint32_t objId) = 0;
    
    /// <summary>
    /// Discards the specified number of bytes from the input stream
    /// </summary>
    virtual void Skip(uint64_t ncb) = 0;
    
    /// <returns>
    /// The total number of bytes read so far
    /// </returns>
    virtual uint64_t Count(void) const = 0;
    
    /// <summary>
    /// Reads a described object from the stream.
    /// </summary>
    virtual void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) = 0;

    /// <summary>
    /// Reads the specified number of bytes from the input stream
    /// </summary>
    virtual void ReadByteArray(void* pBuf, uint64_t ncb) = 0;
    
    /// <summary>
    /// Reads out a string into the specified buffer
    /// </summary>
    /// <param name="charSize"> The number of bytes per character</param>
    virtual void ReadString(void* pBuf, size_t charCount, size_t charSize) = 0;
    
    /// <summary>
    /// Reads the specified boolean value to the output stream
    /// </sumamry>
    virtual bool ReadBool() = 0;
    
    /// <summary>
    /// Reads the specified integer from the input stream
    /// </summary>
    /// <param name="value">The actual integer to be written</param>
    /// <param name="ncb">
    /// The number of bytes maximumum that are set in value
    /// </param>
    /// <remarks>
    /// It is an error for (value & ~(1 << (ncb * 8))) to be nonzero.
    /// </remarks>
    
    virtual uint64_t ReadInteger(size_t ncb) = 0;
    virtual void ReadFloat(float& value) { ReadByteArray(&value, sizeof(float)); }
    virtual void ReadFloat(double& value) { ReadByteArray(&value, sizeof(double)); }
    virtual void ReadArray(const field_serializer& sz, uint64_t n, std::function<void*()> enumerator) = 0;
    virtual void ReadDictionary(const field_serializer& keyDesc,
                                void* key,
                                const field_serializer& valueDesc,
                                void* value,
                                std::function<void(const void* key, const void* value)> inserter
                                ) = 0;

  };

  /// <summary>
  /// "Regstry" interface for serialization operations that need to serialize foreign object references
  /// </summary>
  class IArchiveRegistry:
    public IArchive
  {
  public:
    virtual ~IArchiveRegistry(void) {}

    virtual void* ReadObjectReference(const create_delete& cd, const field_serializer& sz) = 0;

    /// <summary>
    /// Registers an encountered identifier for later deserialization
    /// </summary>
    /// <param name="cd">The allocation and deletion routine pair, to be invoked if the object isn't found</param>
    /// <param name="desc">The descriptor that will be used to deserialize the object, if needed</param>
    /// <param name="objId">The ID of the object that has been encountered</param>
    /// <returns>A pointer to the object</returns>
    /// <remarks>
    /// While this method does always return a valid pointer, and the pointed-to object is guaranteed to
    /// be at a minimum default constructed, the returned object may nevertheless have yet to be deserialized.
    /// </remarks>
    virtual void* Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) = 0;
  };
}
