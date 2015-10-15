// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "IArray.h"
#include "IDictionary.h"
#include <cstdint>
#include <functional>
#include <ios>
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

  namespace internal {
    class AllocationBase;

    // Utility type for maintaining stack-based state
    template<typename T>
    struct Pusher {
      Pusher(Pusher&&) = delete;
      Pusher(const Pusher&) = delete;

      Pusher(T& cur) :
        cur(cur),
        prior(cur)
      {}
      ~Pusher(void) { cur = prior; }

      T& cur;
      T prior;
    };

    template<typename T>
    Pusher<T> MakePusher(T& cur) { return Pusher<T>{cur}; }
  }

  /// <summary>
  /// Stream adaptor interface for use with the Archive type
  /// </summary>
  class IInputStream {
  public:
    virtual ~IInputStream(void) {}

    /// <returns>
    /// False if the next call to Read might succeed
    /// </returns>
    virtual bool IsEof(void) const = 0;

    /// <summary>
    /// Reads exactly the specified number of bytes from the input stream
    /// </summary>
    /// <returns>
    /// The number of bytes actually read, -1 if there was an error.  The number of bytes read
    /// may be less than the number of bytes requested if the end of the file was encountered
    /// before the operation completed.
    /// </returns>
    virtual std::streamsize Read(void* pBuf, std::streamsize ncb) = 0;

    /// <summary>
    /// Discards the specified number of bytes from the input stream
    /// </summary>
    virtual std::streamsize Skip(std::streamsize ncb) = 0;
  };

  /// <summary>
  /// Mapping adaptor, allows input streams to be wrapped to support Archive operations
  /// </summary>
  class InputStreamAdapter:
    public IInputStream 
  {
  public:
    InputStreamAdapter(std::istream& is) :
      is(is)
    {}

  private:
    std::istream& is;

  public:
    // IInputStream overrides:
    bool IsEof(void) const override;
    std::streamsize Read(void* pBuf, std::streamsize ncb) override;
    std::streamsize Skip(std::streamsize ncb) override;
  };

  /// <summary>
  /// Stream adaptor interface for use with the Archive type
  /// </summary>
  class IOutputStream {
  public:
    virtual ~IOutputStream(void) {}

    /// <summary>
    /// Writes all of the specified bytes to the output stream
    /// </summary>
    virtual bool Write(const void* pBuf, std::streamsize ncb) = 0;
  };

  class OutputStreamAdapter:
    public IOutputStream 
  {
  public:
    OutputStreamAdapter(std::ostream& os) :
      os(os)
    {}

    // IOutputStream overrides:
    bool Write(const void* pBuf, std::streamsize ncb) override;

  private:
    std::ostream& os;
  };

  /// <summary>
  /// An enumeration of the different basic types that are handled directly by the archiver
  /// </summary>
  /// <remarks>
  /// 'Atom' refers to a basic unit of serializablity.  Most are straightforward, but some 
  /// require documentation.
  /// Reference - Any type of pointer to another object
  /// Descriptor - A combination of one or more other atoms.  Has a degree of support for adding fields
  /// Finalized Descriptor - A descriptor with a fixed and un-changable set of fields.
  /// </remarks>
  enum class serial_atom {
    ignored = -1,
    boolean = 0,
    i8,
    i16,
    i32,
    i64,
    f32,
    f64,
    reference,
    array,
    string,
    map,
    descriptor, //A combination type
    finalized_descriptor //a combination type which may never change
  };

  const char* ToString(serial_atom atom);

  /// <summary>
  /// Represents a stateful serialization archive structure
  /// </summary>
  class OArchive {
  public:
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
    /// The number of bytes maximum that are set in value
    /// </param>
    /// <remarks>
    /// It is an error for (value & ~(1 << (ncb * 8))) to be nonzero.
    /// </remarks>
    virtual void WriteInteger(int64_t value, uint8_t ncb) = 0;

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
    
    virtual uint64_t SizeInteger(int64_t value, uint8_t ncb) const = 0;
    virtual uint64_t SizeFloat(float value) const = 0;
    virtual uint64_t SizeFloat(double value) const = 0;
    virtual uint64_t SizeBool(bool value) const = 0;
    virtual uint64_t SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const = 0;

    template<class T>
    uint64_t SizeInteger(T value) {
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

    /// <summary>
    /// Writes an object directly to the stream - used as the root call to serialize an object.
    /// </summary>
    /// <remarks>
    /// This method MUST NOT BE CALLED by any internal serialization routines.  Is intended exclusively for
    /// root objects--IE, objects that have no context and therefore no identifiers.
    /// </remarks>
    virtual void WriteObject(const field_serializer& serializer, const void* pObj) = 0;
    
    /// <summary>
    /// Writes a descriptor to the stream
    /// </summary>
    virtual void WriteDescriptor(const descriptor& descriptor, const void* pObj) = 0;
    
    /// <summary>
    /// Writes out an array of entries
    /// </summary>
    virtual void WriteArray(IArrayReader&& ary) = 0;
    
    /// <summary>
    /// Writes out a dictionary of entries
    /// </summary>
    virtual void WriteDictionary(IDictionaryReader&& dictionary) = 0;

    virtual uint64_t SizeObjectReference(const field_serializer& serializer, const void* pObj) const = 0;
    virtual uint64_t SizeDescriptor(const descriptor& descriptor, const void* pObj) const = 0;
    virtual uint64_t SizeArray(IArrayReader&& ary) const = 0;
    virtual uint64_t SizeDictionary(IDictionaryReader&& dictionary) const = 0;
  };

  class IArchive {
  public:
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
    /// Reads an object into the specified memory
    /// </summary>
    virtual void ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) = 0;

    /// <summary>
    /// Identical to IArchiveRegistry::ReadObjectReference, except this prevents the IArchive from delegating delete responsibilities to the allocation
    /// </summary>
    /// <param name="pfnAlloc">The routine to be used for allocation</param>
    /// <param name="serializer">The descriptor to use to deserialize this object, if necessary</param>
    /// <returns>A ReleasedMemory structure</returns>
    /// <remarks>
    /// This method is permitted in the IArchive class because it does not require that the archive take responsibility
    /// for the proffered object, and in fact can allow the IArchive to abdicate responsibility for a type for which it
    /// may have been formerly responsible.
    /// </remarks>
    virtual ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) = 0;

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
    /// <param name="charSize"> The number of bytes per character </param>
    /// <param name="getBufferFn"> Function to get the buffer to write to, takes the size of the string as an argument </param>
    virtual void ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) = 0;

    /// <summary>
    /// Reads out a length delimited array of an arbitrary type into the specified buffer.
    /// </summary>
    virtual void ReadArray(IArrayAppender&& ary) = 0;

    /// <summary>
    /// Reads the specified boolean value to the output stream
    /// </sumamry>
    virtual bool ReadBool() = 0;
    
    /// <summary>
    /// Reads the specified integer from the input stream
    /// </summary>
    /// <param name="value">The actual integer to be written</param>
    /// <param name="ncb">
    /// The number of bytes maximum that are set in value
    /// </param>
    /// <remarks>
    /// It is an error for (value & ~(1 << (ncb * 8))) to be nonzero.
    /// </remarks>
    virtual uint64_t ReadInteger(uint8_t ncb) = 0;
    virtual void ReadFloat(float& value) { ReadByteArray(&value, sizeof(float)); }
    virtual void ReadFloat(double& value) { ReadByteArray(&value, sizeof(double)); }
    virtual void ReadDictionary(IDictionaryInserter&& dictionary) = 0;
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
    /// <param name="cd">The allocation and deletion routine pair, to be invoked if the object isn't found</param>
    /// <param name="desc">The descriptor that will be used to deserialize the object, if needed</param>
    /// <returns>A pointer to the object</returns>
    /// <remarks>
    /// While this method does always return a valid pointer, and the pointed-to object is guaranteed to
    /// be at a minimum default constructed, the returned object may nevertheless have yet to be deserialized.
    /// </remarks>
    virtual void* ReadObjectReference(const create_delete& cd, const field_serializer& desc) = 0;
  };
}
