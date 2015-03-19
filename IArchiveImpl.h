#pragma once
#include "Archive.h"
#include "Allocation.h"
#include <queue>
#include <unordered_map>

namespace leap {
  struct create_delete;

  class IArchiveImpl:
    public IArchiveRegistry
  {
  public:
    /// <summary>
    /// Constructs an archive implementation around the specified stream operator
    /// </summary>
    /// <param name="is">The underlying input stream</param>
    /// <param name="pRootObj">The root object about which deserialization takes place</param>
    IArchiveImpl(std::istream& is, void* pRootObj);
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

    struct entry {
      void* pObj;
      void(*pfnFree)(void*);
    };

    // Map of objects (as we encounter them) to their identifiers.  We use this
    // to reconcile cycles
    std::unordered_map<uint32_t, entry> objMap;

    // Identifiers remaining to be deserialized:
    std::queue<deserialization_task> work;

  public:
    // IArchive overrides:
    void* Lookup(const create_delete& cd, const field_serializer& serializer, uint32_t objId) override;
    void* Release(void* (*pfnAlloc)(), const field_serializer& serializer, uint32_t objId) override;
    std::istream& GetStream() const override;
    void Read(void* pBuf, uint64_t ncb) override;
    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const override { return m_count; }

    /// <summary>
    /// Moves ownership of all deletable entities to the specified allocator type
    /// </summary>
    /// <remarks>
    /// The internal object map is cleared as a result of this operation
    /// </remarks>
    void Transfer(internal::AllocationBase& alloc);

    /// <remarks>
    /// Destroys all objects that define deleters and clears the object map
    /// </remarks>
    /// <returns>The number of objects destroyed</returns>
    size_t ClearObjectTable(void);

    /// <summary>
    /// Recursively processes deserialization tasks, starting with the one passed, until none are left
    /// </summary>
    void Process(const deserialization_task& task);
  };
}
