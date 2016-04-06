// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include <queue>
#include <unordered_map>

namespace leap {
  struct create_delete;

  class OArchiveLeapSerial:
    public OArchiveRegistry
  {
  public:
    OArchiveLeapSerial(IOutputStream& os);
    virtual ~OArchiveLeapSerial(void);

  protected:
    // If any additional (flat) memory was required to construct the output stream reference, this is it
    void* const pOsMem = nullptr;
    void(*const pfnDtor)(void*) = nullptr;

    // The last ID issued for any object, or 0 if no IDs have been issued yet.  The 0 identifier
    // is reserved for the root object.
    uint32_t lastID = 0;

    struct work {
      work(
        uint32_t id,
        const field_serializer* serializer,
        const void* pObj
      ):
        id(id),
        serializer(serializer),
        pObj(pObj)
      {}

      uint32_t id;
      const field_serializer* serializer;
      const void* pObj;
    };

    // Map of objects (as we encounter them) to their identifiers.  We use this
    // to reconcile cycles
    std::unordered_map<const void*, uint32_t> objMap;

    // Queue of things waiting to be serialized.  Objects are serialized in order so that identifiers
    // will be sequential in the output stream.
    std::queue<work> deferred;

    void WriteSize(uint32_t sz);

    /// <summary>
    /// Translates from an object pointer to an object ID, and registers the pointer
    /// for later deserialization by Process() if it has not been encountered before
    /// </summary>
    uint32_t RegisterObject(const field_serializer& serializer, const void* pObj);

    /// <summary>
    /// Processes objects on the internal queue until the queue is empty
    /// </summary>
    void Process(void);

  public:
    using OArchive::WriteInteger;
    using OArchive::SizeInteger;

    // OArchive overrides:
    void WriteObject(const field_serializer& serializer, const void* pObj) override;
    void WriteObjectReference(const field_serializer& serializer, const void* pObj) override;
    void WriteDescriptor(const descriptor& descriptor, const void* pObj) override;

    void WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize = false) override;
    void WriteString(const void* pbuf, uint64_t charCount, uint8_t charSize) override;
    void WriteBool(bool value) override;
    void WriteInteger(int64_t value, uint8_t ncb) override;
    void WriteFloat(float value) override { WriteByteArray(&value, sizeof(value)); }
    void WriteFloat(double value) override { WriteByteArray(&value, sizeof(value)); }
    void WriteFloat(long double value) override { WriteByteArray(&value, sizeof(value)); }
    void WriteArray(IArrayReader&& ary) override;
    void WriteDictionary(IDictionaryReader&& dictionary) override;

    uint64_t SizeDescriptor(const descriptor& desc, const void* pObj) const override;
    uint64_t SizeObjectReference(const field_serializer&, const void*) const override { return sizeof(uint32_t); }
    uint64_t SizeArray(IArrayReader&& ary) const override;
    uint64_t SizeDictionary(IDictionaryReader&& dictionary) const override;

    inline uint64_t SizeString(const void* pBuf, uint64_t charCount, uint8_t charSize) const override { return (size_t)(sizeof(uint32_t) + (charCount*charSize)); }
    uint64_t SizeInteger(int64_t value, uint8_t ncb) const override;
    uint64_t SizeFloat(float value) const override { return sizeof(float); }
    uint64_t SizeFloat(double value) const override { return sizeof(double); }
    uint64_t SizeFloat(long double value) const override { return sizeof(long double); }
    uint64_t SizeBool(bool) const override { return 1; }
  };

  class IArchiveLeapSerial:
    public IArchiveRegistry
  {
  public:
    /// <summary>
    /// Constructs an archive implementation around the specified stream operator
    /// </summary>
    /// <param name="is">The underlying input stream</param>
    IArchiveLeapSerial(IInputStream& is);
    IArchiveLeapSerial(std::istream& is);
    virtual ~IArchiveLeapSerial(void);

    struct deserialization_task {
      deserialization_task(
        const field_serializer* serializer,
        uint32_t id,
        void* pObject
      ) :
        serializer(serializer),
        id(id),
        pObject(pObject)
      {}

      // Descriptor, identifier, and object pointer
      const field_serializer* serializer;
      uint32_t id;
      void* pObject;
    };

  private:
    // Underlying input stream
    IInputStream& is;

    // If any additional (flat) memory was required to construct the output stream reference, this is it
    void* const pIsMem = nullptr;
    void(*const pfnDtor)(void*) = nullptr;

    // Number of bytes read so far:
    uint64_t m_count = 0;

    struct entry {
      // A pointer to the raw object
      void* pObject;

      // Caller-specified context field, if Release was called, otherwise nullptr
      std::shared_ptr<void> pContext;

      // A pointer to the routine that will be used to clean up the object
      void(*pfnFree)(void*);
    };

    // Map of objects (as we encounter them) to their identifiers.  We use this
    // to reconcile cycles
    std::unordered_map<uint32_t, entry> objMap;

    // Identifiers remaining to be deserialized:
    std::queue<deserialization_task> work;

    ReleasedMemory Release(ReleasedMemory(*pfnAlloc)(), const field_serializer& serializer, uint32_t objId);
    void* Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId);

    bool IsReleased(uint32_t objId);

    /// <summary>
    /// Moves ownership of all deletable entities to the specified allocator type
    /// </summary>
    /// <remarks>
    /// The internal object map is cleared as a result of this operation
    /// </remarks>
    void Transfer(internal::AllocationBase& alloc);

    /// <summary>
    /// Destroys all objects that define deleters and clears the object map
    /// </summary>
    /// <returns>The number of objects destroyed</returns>
    size_t ClearObjectTable(void);

    /// <summary>
    /// Recursively processes deserialization tasks, starting with the one passed, until none are left
    /// </summary>
    void Process(const deserialization_task& task);
  public:

    void* ReadObjectReference(const create_delete& cd, const field_serializer& sz) override;

    // IArchive overrides:
    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const override { return m_count; }

    void ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) override;

    ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) override;
    void ReadByteArray(void* pBuf, uint64_t ncb) override;
    void ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) override;
    bool ReadBool() override;
    uint64_t ReadInteger(uint8_t ncb) override;
    void ReadFloat(float& value) override { ReadByteArray(&value, sizeof(value)); }
    void ReadFloat(double& value) override { ReadByteArray(&value, sizeof(value)); }
    void ReadFloat(long double& value) override { ReadByteArray(&value, sizeof(value)); }
    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadArray(IArrayAppender&& ary) override;
    void ReadDictionary(IDictionaryInserter&& dictionary) override;
  };
}
