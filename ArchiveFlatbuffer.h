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
    uint64_t Count(void) const override { return m_count; }

    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadByteArray(void* pBuf, uint64_t ncb) override;
    void ReadString(void* pBuf, uint64_t charCount, uint8_t charSize) override;
    bool ReadBool() override;
    uint64_t ReadInteger(uint8_t ncb) override;
    void ReadFloat(float& value) override;
    void ReadFloat(double& value) override;
    void ReadArray(const field_serializer& sz, uint64_t n, std::function<void*()> enumerator) override;
    void ReadDictionary(const field_serializer& keyDesc,
      void* key,
      const field_serializer& valueDesc,
      void* value,
      std::function<void(const void* key, const void* value)> inserter
      ) override;

    void* ReadObjectReference(const create_delete& cd, const field_serializer& desc) override;


  private:
    std::vector<uint8_t> m_data;
    uint64_t m_count = 0;

    std::stack<uint32_t> m_currentTableOffset;
    std::stack<int32_t> m_currentVTableOffset;
    std::stack<uint16_t> m_currentVTableSize;
    std::stack<uint16_t> m_currentObjectSize;

    uint8_t m_currentVTableEntry = 0;

    //data read helpers:
    template<typename T>
    T GetValue(uint32_t off) { return *reinterpret_cast<T*>(&m_data[off]); }

    template<typename T>
    T GetCurrentTableValue() {
      const auto fieldOffset = m_currentTableOffset.top() + GetValue<uint16_t>(m_currentVTableOffset.top() + 4 + (2 * m_currentVTableEntry));
      return GetValue<T>(fieldOffset);
    }
  };

}