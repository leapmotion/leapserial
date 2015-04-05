#pragma once
#include "Archive.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class OArchiveImpl:
    public OArchiveRegistry
  {
  public:
    OArchiveImpl(std::ostream& os);
    ~OArchiveImpl(void);

  private:
    // Underlying output stream
    std::ostream& os;

    // The last ID issued for any object, or 0 if no IDs have been issued yet.  The 0 identifier
    // is reserved for the root object.
    uint32_t lastID;

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
  public:
    using OArchive::WriteInteger;
    using OArchive::SizeInteger;

    // OArchive overrides:
    void WriteObject(const field_serializer& serializer, const void* pObj) override;
    void WriteObjectReference(const field_serializer& serializer, const void* pObj) override;

    void WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize = false) override;
    void WriteString(const void* pbuf, uint64_t charCount, uint8_t charSize) override;
    void WriteBool(bool value) override;
    void WriteInteger(int64_t value, size_t ncb) override;
    void WriteArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) override;
    void WriteDictionary(uint64_t n, const field_serializer& keyDesc, std::function<const void*()> keyEnumerator, const field_serializer& valueDesc, std::function<const void*()> valueEnumerator) override;
    
    uint64_t SizeObject(const field_serializer& serializer, const void* pObj) const override;
    uint64_t SizeObjectReference(const field_serializer&, const void*) const override { return sizeof(uint32_t); }
    uint64_t SizeArray(const field_serializer& desc, uint64_t n, std::function<const void*()> enumerator) const override;
    uint64_t SizeDictionary(uint64_t n, const field_serializer& keyDesc, std::function<const void*()> keyEnumerator, const field_serializer& valueDesc, std::function<const void*()> valueEnumerator) const override;
    
    inline size_t SizeString(const void* pBuf, uint64_t charCount, size_t charSize) const override { return sizeof(uint32_t) + (charCount*charSize); }
    size_t SizeInteger(int64_t value, size_t ncb) const override { return VarintSize(value); }
    size_t SizeFloat(float value) const override { return sizeof(float); }
    size_t SizeFloat(double value) const override { return sizeof(double); }
    size_t SizeBool(bool) const override { return 1; }

    /// <returns>
    /// The number of bytes that would be required to serialize the specified integer with varint encoding
    /// </returns>
    static uint16_t VarintSize(int64_t value);

    /// <summary>
    /// Processes objects on the internal queue until the queue is empty
    /// </summary>
    void Process(void);
  };
}
