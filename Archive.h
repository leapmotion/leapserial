#pragma once
#include <stdint.h>
#include <iosfwd>

namespace leap {
  struct create_delete;
  struct descriptor;
  struct field_serializer;
  class IArchive;
  class IArchiveRegistry;
  class OArchive;
  class OArchiveRegistry;

  enum class serial_type {
    // Null type, this type is never serialized
    ignored = -1,

    varint = 0,
    b64 = 1,
    string = 2,
    b32 = 5
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
    /// Get  a stream interface to the archive
    /// </summary>
    virtual std::ostream& GetStream() const = 0;
    
    /// <summary>
    /// Writes the specified bytes to the output stream
    /// </summary>
    virtual void Write(const void* pBuf, uint64_t ncb) const = 0;

    /// <returns>
    /// The number of bytes that would be required to serialize the specified integer with varint encoding
    /// </returns>
    static uint16_t VarintSize(int64_t value);

    /// <summary>
    /// Writes the specified integer as a varint to the output stream
    /// </sumamry>
    void WriteVarint(int64_t value) const;

    // Convenience overloads:
    void Write(int32_t value) const { return Write(&value, sizeof(value)); }
    void Write(uint32_t value) const { return Write(&value, sizeof(value)); }
    void Write(int64_t value) const { return Write(&value, sizeof(value)); }
    void Write(uint64_t value) const { return Write(&value, sizeof(value)); }
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
    virtual uint32_t RegisterObject(const field_serializer& serializer, const void* pObj) = 0;
  };

  class IArchive {
  public:
    /// <summary>
    /// Creates a new output archive based on the specified stream
    /// </summary>
    virtual ~IArchive(void) {}

    /// <summary>
    /// Get a stream interface to the archive
    /// </summary>
    virtual std::istream& GetStream() const = 0;

    /// <summary>
    /// Identical to Lookup, except this prevents the IArchive from delegating delete responsibilities to the allocation
    /// </summary>
    /// <param name="pfnAlloc">The routine to be used for allocation</param>
    /// <remarks>
    /// This method is permitted in the IArchive class because it does not require that the archive take responsibility
    /// for the proffered object, and in fact can allow the IArchive to abdicate responsibility for a type for which it
    /// may have been formerly responsible.
    /// </remarks>
    virtual void* Release(void* (*pfnAlloc)(), const field_serializer& serializer, uint32_t objId) = 0;

    /// <summary>
    /// Reads the specified number of bytes from the input stream
    /// </summary>
    virtual void Read(void* pBuf, uint64_t ncb) = 0;

    // Convenience overloads:
    void Read(int32_t& val) { Read(&val, sizeof(val)); }
    void Read(uint32_t& val) { Read(&val, sizeof(val)); }
    void Read(int64_t& val) { Read(&val, sizeof(val)); }
    void Read(uint64_t& val) { Read(&val, sizeof(val)); }

    /// <summary>
    /// Discards the specified number of bytes from the input stream
    /// </summary>
    virtual void Skip(uint64_t ncb) = 0;

    /// <returns>
    /// The total number of bytes read so far
    /// </returns>
    virtual uint64_t Count(void) const = 0;

    /// <summary>
    /// Interprets bytes from the output stream as a single varint
    /// </sumamry>
    int64_t ReadVarint(void);
  };

  /// <summary>
  /// "Regstry" interface for serialization operations that need to serialize foreign object references
  /// </summary>
  class IArchiveRegistry:
    public IArchive
  {
  public:
    virtual ~IArchiveRegistry(void) {}

    /// <summary>
    /// Registers an encountered identifier for later deserialization
    /// </summary>
    /// <param name="pfnAllocate">The allocation routine, to be invoked if the object isn't found</param>
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