// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class OArchiveImpl:
    public OArchiveRegistry
  {
  public:
    OArchiveImpl(IOutputStream& os);
    virtual ~OArchiveImpl(void);

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
}
