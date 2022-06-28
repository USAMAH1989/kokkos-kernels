#ifndef __KOKKOSBATCHED_SCALE_DECL_HPP__
#define __KOKKOSBATCHED_SCALE_DECL_HPP__

/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "impl/Kokkos_Error.hpp"

namespace KokkosBatched {

///
/// Serial Scale
///

struct SerialScale {
  template <typename ScalarType, typename AViewType>
  KOKKOS_INLINE_FUNCTION static int invoke(const ScalarType alpha,
                                           const AViewType &A) {
    Kokkos::abort(
        "KokkosBatched::SerialScale is deprecated: use KokkosBlas::SerialScale "
        "instead");
    return 0;
  }
};

///
/// Team Scale
///

template <typename MemberType>
struct TeamScale {
  template <typename ScalarType, typename AViewType>
  KOKKOS_INLINE_FUNCTION static int invoke(const MemberType &member,
                                           const ScalarType alpha,
                                           const AViewType &A) {
    Kokkos::abort(
        "KokkosBatched::TeamScale is deprecated: use KokkosBlas::TeamScale "
        "instead");
    return 0;
  }
};

///
/// TeamVector Scale
///

template <typename MemberType>
struct TeamVectorScale {
  template <typename ScalarType, typename AViewType>
  KOKKOS_INLINE_FUNCTION static int invoke(const MemberType &member,
                                           const ScalarType alpha,
                                           const AViewType &A) {
    // static_assert(false);
    Kokkos::abort(
        "KokkosBatched::TeamVectorScale is deprecated: use "
        "KokkosBlas::TeamVectorScale instead");
    return 0;
  }
};

}  // namespace KokkosBatched

#endif
