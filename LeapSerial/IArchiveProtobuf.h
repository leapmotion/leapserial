#pragma once
#include "Archive.h"

namespace leap {
  class IArchiveProtobuf :
    public IArchiveRegistry
  {
  public:
    IArchiveProtobuf(IInputStream& is);

    void ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) override;
    ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) override;

    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const override { return 0; }

    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadByteArray(void* pBuf, uint64_t ncb) override;
    void ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) override;
    bool ReadBool(void) override;
    uint64_t ReadInteger(uint8_t) override;
    void ReadFloat(float& value) override { is.Read(&value, sizeof(value)); }
    void ReadFloat(double& value) override { is.Read(&value, sizeof(value)); }
    void ReadArray(IArrayAppender&& ary) override;
    void ReadDictionary(IDictionaryInserter&& rdr) override;

    void* ReadObjectReference(const create_delete& cd, const field_serializer& desc) override;

  private:
    bool ReadSingle(const descriptor& descriptor, void* pObj);

    // Descriptor of current object being read, if any exist:
    const descriptor* m_pCurDesc = nullptr;

    // Stream traits:
    uint64_t m_count = 0;
    IInputStream& is;
  };
}
