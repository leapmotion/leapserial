#pragma once

namespace leap {
  /// <summary>
  /// Base declaration entry, used to mark inheritance relationships between type
  /// </summary>
  /// <remarks>
  /// This is a tag type.  To use it, instantiate it with the correct base and derived
  /// types in the descriptor list.  The base type may appear in any order and will be
  /// serialized as any other member of your class would be serialized.
  ///
  /// The named base type must be serializable.
  ///
  /// The base type must not be a virtual base of the derived type.
  ///
  /// This entry will cause a link to the base type's serializer to be added to the
  /// descriptor where it appears.  The base descriptor is not transformed or copied
  /// into the current descriptor.  For very deep inheritance heirarchies, this may
  /// potentially impact performance.
  /// </remarks>
  template<typename Base, typename Derived>
  struct base
  {
    static_assert(std::is_base_of<Base, Derived>::value, "Base must be an actual base class of Derived");
  };
}