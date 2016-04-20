// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"

namespace leap {

  class OArchiveJSON :
    public OArchiveRegistry
  {
  public:
    OArchiveJSON(leap::OutputStreamAdapter& osa, bool escapeSlashes = false);

    // Controls whether the forward slash character should be escaped
    const bool EscapeSlashes;

    // Flag which controls pretty printing of outputs
    bool PrettyPrint = false;

    // Number of tabs that will be placed on pretty-printed lines
    size_t TabLevel = 0;

    // Width of a tab character.  Zero prints an actual tab, any other number
    // prints that number of spaces
    size_t TabWidth = 0;

    // OArchiveRegistry overrides
    void WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize = false) override;
    void WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) override;
    void WriteBool(bool value) override;
    void WriteInteger(int64_t value, uint8_t ncb) override;
    void WriteInteger(int8_t value) override;
    void WriteInteger(uint8_t value) override;
    void WriteInteger(int16_t value) override;
    void WriteInteger(uint16_t value) override;
    void WriteInteger(int32_t value) override;
    void WriteInteger(uint32_t value) override;
    void WriteInteger(int64_t value) override;
    void WriteInteger(uint64_t value) override;
    void WriteFloat(float value) override;
    void WriteFloat(double value) override;
    void WriteFloat(long double value) override;
    void WriteObjectReference(const field_serializer& serializer, const void* pObj) override;
    void WriteObject(const field_serializer& serializer, const void* pObj) override;
    void WriteDescriptor(const descriptor& descriptor, const void* pObj) override;
    void WriteArray(IArrayReader&& ary) override;
    void WriteDictionary(IDictionaryReader&& dictionary) override;

    //Size query functions.  Note that these do not return the total size of the object,
    //but rather the size they take up in a table (sizeof(uint32_t) for objects stored by reference).
    uint64_t SizeInteger(int64_t value, uint8_t ncb) const override;
    uint64_t SizeFloat(float value) const override;
    uint64_t SizeFloat(double value) const override;
    uint64_t SizeFloat(long double value) const override;
    uint64_t SizeBool(bool value) const override;
    uint64_t SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const override;
    uint64_t SizeObjectReference(const field_serializer& serializer, const void* pObj) const override;
    uint64_t SizeDescriptor(const descriptor& descriptor, const void* pObj) const override;
    uint64_t SizeArray(IArrayReader&& ary) const override;
    uint64_t SizeDictionary(IDictionaryReader&& dictionary) const override;

  private:
    // Current tab level, if pretty printing is turned on, otherwise ignored
    size_t currentTabLevel = 0;
    std::ostream& os;

    // Prints TabLevel spaces to the output stream
    void TabOut(void) const;
  };

  class IArchiveJSON :
    public IArchiveRegistry
  {
  public:
    IArchiveJSON(std::istream& is);

    void ReadObject(const field_serializer& sz, void* pObj, internal::AllocationBase* pOwner) override;
    ReleasedMemory ReadObjectReferenceResponsible(ReleasedMemory(*pfnAlloc)(), const field_serializer& sz, bool isUnique) override;

    void Skip(uint64_t ncb) override;
    uint64_t Count(void) const override { return 0; }

    void ReadDescriptor(const descriptor& descriptor, void* pObj, uint64_t ncb) override;
    void ReadByteArray(void* pBuf, uint64_t ncb) override;
    void ReadString(std::function<void*(uint64_t)> getBufferFn, uint8_t charSize, uint64_t ncb) override;
    bool ReadBool() override;
    uint64_t ReadInteger(uint8_t ncb) override;
    void ReadFloat(float& value) override;
    void ReadFloat(double& value) override;
    void ReadArray(IArrayAppender&& ary) override;
    void ReadDictionary(IDictionaryInserter&& dictionary) override;

    void* ReadObjectReference(const create_delete& cd, const field_serializer& desc) override;
  };
};
