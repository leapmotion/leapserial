// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include "Archive.h"
#include <memory>

namespace leap {
  struct field_descriptor;

  class OArchiveProtobuf :
    public OArchiveRegistry
  {
  public:
    OArchiveProtobuf(IOutputStream& os);

    // OArchiveRegistry overrides
    void WriteByteArray(const void* pBuf, uint64_t ncb, bool writeSize = false) override;
    void WriteString(const void* pBuf, uint64_t charCount, uint8_t charSize) override;
    void WriteBool(bool value) override { WriteInteger(value, 1); }
    void WriteInteger(int64_t value, uint8_t) override;
    void WriteFloat(float value) override;
    void WriteFloat(double value) override;
    void WriteObjectReference(const field_serializer& serializer, const void* pObj) override;
    void WriteObject(const field_serializer& serializer, const void* pObj) override;

    /// <summary>
    /// Writes a top-level context-free descriptor
    /// </summary>
    /// <remarks>
    /// A context-free descriptor has no identifier word or length field; this must be written by
    /// the outer scope if it is desired.
    /// </remarks>
    void WriteDescriptor(const descriptor& descriptor, const void* pObj) override;

    void WriteArray(IArrayReader&& ary) override;
    void WriteDictionary(IDictionaryReader&& dictionary) override;

    //Size query functions.  Note that these do not return the total size of the object,
    //but rather the size they take up in a table (sizeof(uint32_t) for objects stored by reference).
    uint64_t SizeInteger(int64_t value, uint8_t) const override;
    uint64_t SizeFloat(float value) const override { return 4; }
    uint64_t SizeFloat(double value) const override { return 8; }
    uint64_t SizeBool(bool value) const override { return 1; }
    uint64_t SizeString(const void* pBuf, uint64_t ncb, uint8_t charSize) const override;
    uint64_t SizeObjectReference(const field_serializer& serializer, const void* pObj) const override;
    uint64_t SizeDescriptor(const descriptor& descriptor, const void* pObj) const override;
    uint64_t SizeArray(IArrayReader&& ary) const override;
    uint64_t SizeDictionary(IDictionaryReader&& dictionary) const override;

  private:
    IOutputStream& os;

    // Stateful:  Stores the identifier of the object presently being serialized
    mutable const std::pair<const uint64_t, field_descriptor>* curDescEntry = nullptr;
  };
}
