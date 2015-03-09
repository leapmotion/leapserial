#pragma once
#include "Archive.h"
#include "Allocation.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class IArchiveImpl:
    public IArchive
  {
  public:
    IArchiveImpl(std::istream& is, internal::AllocationBase& alloc);
    ~IArchiveImpl(void);

    struct deserialization_task {
      deserialization_task(
        const field_serializer* serializer,
        uint32_t id,
        void* pObj
      ) :
        serializer(serializer),
        id(id),
        pObj(pObj)
      {}

      // Descriptor, identifier, and object pointer
      const field_serializer* serializer;
      uint32_t id;
      void* pObj;
    };

  private:
    // Underlying input stream
    std::istream& is;

    // Number of bytes read so far:
    uint64_t m_count = 0;

    // Allocation return structure
    internal::AllocationBase& alloc;

    // Map of objects (as we encounter them) to their identifiers.  We use this
    // to reconcile cycles
    std::unordered_map<uint32_t, void*> objMap;

    // Identifiers remaining to be deserialized:
    std::queue<deserialization_task> work;

  public:
    // IArchive overrides:
    void* Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) override;
    void Read(void* pBuf, uint64_t ncb) override;
    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const override { return m_count; }

    /// <summary>
    /// Recursively processes deserialization tasks, starting with the one passed, until none are left
    /// </summary>
    void Process(const deserialization_task& task);
  };
}
