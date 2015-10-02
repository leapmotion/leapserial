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
    IArchiveImpl(IInputStream& is);
    IArchiveImpl(std::istream& is);
    ~IArchiveImpl(void);

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
    void ReadFloat(float& value) override { ReadByteArray(&value, sizeof(float)); }
    void ReadFloat(double& value) override { ReadByteArray(&value, sizeof(double)); }
    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadArray(std::function<void(uint64_t)> sizeBufferFn, const field_serializer& t_serializer, std::function<void*()> enumerator, uint64_t n) override;
    void ReadDictionary(const field_serializer& keyDesc,
                        void* key,
                        const field_serializer& valueDesc,
                        void* value,
                        std::function<void(const void* key, const void* value)> insertionFn
                        ) override;

    


  };
}
