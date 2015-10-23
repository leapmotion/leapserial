// Copyright (C) 2012-2015 Leap Motion, Inc. All rights reserved.
#pragma once
#include <atomic>

namespace leap {
  struct descriptor;

  template<typename T>
  struct serial_traits;

  // Common base type for linkage
  struct descriptor_entry
  {
    descriptor_entry(const std::type_info& ti, const descriptor& desc);

    const descriptor_entry* const Next;
    const std::type_info& ti;
    const descriptor& desc;
  };

  // Entry generator type
  template<typename T>
  class descriptor_entry_t :
    descriptor_entry
  {
  public:
    static_assert(
      serial_traits<T>::is_object,
      "If T is being registered with descriptor_entry_t, that type must be an object"
    );

    descriptor_entry_t(void) :
      descriptor_entry{ typeid(T), serial_traits<T>::get_descriptor() }
    {}

    static const descriptor_entry_t s_init;
  };

  template<typename T>
  const descriptor_entry_t<T> descriptor_entry_t<T>::s_init;

  /// <summary>
  /// Utility class for enumerating all registered serialization descriptors
  /// </summary>
  /// <remarks>
  /// To enumerate all descriptors, just write this:
  ///
  ///    for (const descriptor_entry& cur : leap::descriptors{})
  ///       ...;
  ///
  /// The above traverses a forward-linked list in a thread-safe way and does not incur
  /// any construction penalty.
  ///
  /// Note that the above will only enumerate descriptors for non-empty, instantiated
  /// types that have been compiled into the final binary.  If the linker chooses to
  /// remove code associated with a descriptor--for instance, if it is only defined in
  /// an external library--you might not be able to enumerate it.  Furthermore, a trivial
  /// descriptor, such as for MyType below:
  ///
  ///    class MyType {
  ///      static leap::descriptor GetDescriptor(void) { return {}; }
  ///    };
  /// 
  /// ...will not be enumerated by this collection.
  /// </remarks>
  class descriptors
  {
  public:
    descriptors(void) {}

    /// <summary>
    /// Standard iterator type
    /// </summary>
    class const_iterator {
    public:
      typedef std::forward_iterator_tag iterator_category;
      typedef std::ptrdiff_t difference_type;
      typedef descriptor_entry value_type;
      typedef const descriptor_entry* pointer;
      typedef const descriptor_entry& reference;

      const_iterator(const descriptor_entry* entry = nullptr) :
        m_cur(entry)
      {}

    private:
      const descriptor_entry* m_cur;

    public:
      pointer operator->(void) const { return m_cur; }
      reference operator*(void) { return *m_cur; }

      bool operator==(const const_iterator& rhs) const { return m_cur == rhs.m_cur; }
      bool operator!=(const const_iterator& rhs) const { return m_cur != rhs.m_cur; }

      // Iterator operator overloads:
      const_iterator& operator++(void) {
        m_cur = m_cur->Next;
        return *this;
      }
      const_iterator operator++(int) {
        auto prior = m_cur;
        ++*this;
        return{ prior };
      }
    };

  private:
    // Application linked list head
    static const descriptor_entry* s_pHead;

  public:
    static const descriptor_entry* Link(descriptor_entry& entry);

    // Iterator support logic:
    const_iterator begin(void) const { return{ s_pHead }; }
    const_iterator end(void) const { return{ nullptr }; }
  };
}
