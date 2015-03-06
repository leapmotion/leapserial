#pragma once
#include "Archive.h"
#include <queue>
#include <unordered_map>

namespace leap {
  class OArchiveImpl:
    public OArchive
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

  public:
    // OArchive overrides:
    uint32_t RegisterObject(const field_serializer& serializer, const void* pObj) override;
    void Write(const void* pBuf, uint64_t ncb) const override;
    std::ostream& GetStream() const override;

    /// <summary>
    /// Processes objects on the internal queue until the queue is empty
    /// </summary>
    void Process(void);
  };
}
