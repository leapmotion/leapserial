#pragma once
#include "Archive.h"
#include <vector>
#include <stack>

namespace leap {

  class OArchiveFlatbuffer :
    public OArchiveRegistry
  {
    
  };

  class IArchiveFlatbuffer : 
    public IArchiveRegistry
  {
  public:
    IArchiveFlatbuffer(std::istream& is);
    ~IArchiveFlatbuffer();

    void ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) override;

    ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) override;

    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const { return 0; }

    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadByteArray(void* pBuf, uint64_t ncb) override;
    void ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) override;
    bool ReadBool() override;
    uint64_t ReadInteger(uint8_t ncb) override;
    void ReadFloat(float& value) override;
    void ReadFloat(double& value) override;
    void ReadArray(std::function<void(uint64_t)> sizeBufferFn, const field_serializer& t_serializer, std::function<void*()> enumerator, uint64_t n) override;
    void ReadDictionary(const field_serializer& keyDesc,
      void* key,
      const field_serializer& valueDesc,
      void* value,
      std::function<void(const void* key, const void* value)> inserter
      ) override;

    void* ReadObjectReference(const create_delete& cd, const field_serializer& desc) override;


  private:
    std::vector<uint8_t> m_data;
    
    uint32_t m_offset = 0;

    //data read helpers:
    template<typename T>
    T GetValue(uint32_t off) { return *reinterpret_cast<T*>(&m_data[off]); }
  };

}