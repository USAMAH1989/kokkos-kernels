#ifndef __KOKKOSBATCHED_EIGENVALUE_SERIAL_INTERNAL_HPP__
#define __KOKKOSBATCHED_EIGENVALUE_SERIAL_INTERNAL_HPP__


/// \author Kyungjoo Kim (kyukim@sandia.gov)

#include "KokkosBatched_Util.hpp"
#include "KokkosBatched_WilkinsonShift_Serial_Internal.hpp"
#include "KokkosBatched_HessenbergQR_WithShift_Serial_Internal.hpp"
#include "KokkosBatched_Francis_Serial_Internal.hpp"

namespace KokkosBatched {
  namespace Experimental {
    ///
    /// Serial Internal Impl
    /// ==================== 
    ///
    /// this impl follows the flame interface of householder transformation
    ///
    struct SerialEigenvalueInternal {
      /// Given a strictly Hessenberg matrix H (m x m), this computes all eigenvalues
      /// using the Francis method and stores them into a vector e. This routine does 
      /// not scale nor balance the matrix for the numerical stability.
      /// 
      /// Parameters:
      ///   [in]m 
      ///     A dimension of the square matrix H.
      ///   [in/out]H, [in]hs0, [in]hs1 
      ///     Real Hessenberg matrix H(m x m) with strides hs0 and hs1.
      ///     Entering the routine, H is assumed to have a upper Hessenberg form, where
      ///     all subdiagonals are zero. The matrix is overwritten on exit.
      ///   [out]er, [in]ers, [out]ei, [in]eis
      ///     A complex vector er(m)+ei(m)i with a stride ers and eis to store computed eigenvalues.
      ///     For a complex eigen pair, it stores a+bi and a-bi consecutively. 
      ///   [in]max_iteration(300)
      ///     Unlike LAPACK which uses various methods for different types of matrices,
      ///     this routine uses the Francis method only. A user can set the maximum number 
      ///     of iterations. When it reaches the maximum iteration counts without converging 
      ///     all eigenvalues, the routine returns -1. 
      ///   [in]user_tolerence(-1)
      ///     If |value| < tolerence*|reference value|, then the value is considered as zero.
      ///     By default, it uses 1e5*(machie epsilon).
      ///   [in]restart(false)
      ///     With a restart option, the routine assume that the matrix H and the vector e 
      ///     contain the partial results from the previous run. When m = 1 or 2, this option
      ///     won't work as the routine always computes the all eigenvalues.
      template<typename RealType>
      KOKKOS_INLINE_FUNCTION
      static int
      invoke(const int m,
             /* */ RealType * H, const int hs0, const int hs1,
             /* */ RealType * er, const int ers,
             /* */ RealType * ei, const int eis,
             const int max_iteration = 300,
             const RealType user_tolerence = RealType(-1),
             const bool restart = false) {
        typedef RealType real_type;
        typedef Kokkos::Details::ArithTraits<real_type> ats;
        const real_type zero(0), nan(ats::nan());
        const real_type tol = ( user_tolerence < 0 ? 
                                1e5*ats::epsilon() : user_tolerence);

        int r_val = 0;
        if (restart) {
          if (m <= 2) {
            Kokkos::abort("Error: restart option cannot be used for m=1 or m=2");
          }
        } else {
          for (int i=0;i<m;++i) 
            er[i*ers] = nan; 
        }
        
        switch (m) {
        case 0: { /* do nothing */ break; }
        case 1: { er[0] = H[0]; ei[0] = zero; break; }
        case 2: {
          /// compute eigenvalues from the characteristic determinant equation
          bool is_complex;
          Kokkos::complex<real_type> lambda1, lambda2;
          SerialWilkinsonShiftInternal::invoke(H[0*hs0+0*hs1], H[0*hs0+1*hs1], 
                                               H[1*hs0+0*hs1], H[1*hs0+1*hs1],
                                               &lambda1, &lambda2,
                                               &is_complex);
          er[0] = lambda1.real(); ei[0] = lambda1.imag();
          er[1] = lambda2.real(); ei[1] = lambda2.imag();
          break;
        }
        default: {
          /// Francis method 
          const int hs = hs0+hs1;  /// diagonal stride
          int iter(0);             /// iteration count
          bool converge = false;   /// bool to check all eigenvalues are converged

          while (!converge && iter < max_iteration) {
            /// Step 1: find a set of unrevealed eigenvalues 
            int cnt = 1;
            
            /// find mbeg (first nonzero subdiag value)
            for (;cnt<m;++cnt) {
              const auto     val = ats::abs(*(H+cnt*hs-hs1));
              const auto ref_val = ats::abs(*(H+cnt*hs));
              if (val > tol*ref_val) break;
            }
            const int mbeg = cnt-1;
            
            /// find mend (first zero subdiag value)
            for (;cnt<m;++cnt) {
              const auto     val = ats::abs(*(H+cnt*hs-hs1));
              const auto ref_val = ats::abs(*(H+cnt*hs));
              if (val < tol*ref_val) break;
            }
            const int mend = cnt;
            
            /// Step 2: if there exist non-converged eigen values
            if (mbeg < (mend-1)) {
#             if 0 /// implicit QR with shift for testing 
              {
                /// Rayleigh quotient shift 
                const real_type shift = *(H+(mend-1)*hs); 
                SerialHessenbergQR_WithShiftInternal::invoke(mend-mbeg, 
                                                             H+hs*mbeg, hs0, hs1,
                                                             shift);
                real_type *sub2x2 = H+(mend-2)*hs;
                const auto     val = ats::abs(sub2x2[hs0]);
                const auto ref_val = ats::abs(sub2x2[hs]);
                if (val < tol*ref_val) { /// this eigenvalue converges
                  er[(mend-1)*ers] = sub2x2[hs]; ei[(mend-1)*eis] = zero;
                }
              }
#             endif

#             if 1 /// Francis double shift method
              {
                /// find a complex eigen pair
                Kokkos::complex<real_type> lambda1, lambda2;
                bool is_complex;
                real_type *sub2x2 = H+(mend-2)*hs;
                SerialWilkinsonShiftInternal::invoke(sub2x2[0*hs0+0*hs1], sub2x2[0*hs0+1*hs1], 
                                                     sub2x2[1*hs0+0*hs1], sub2x2[1*hs0+1*hs1],
                                                     &lambda1, &lambda2,
                                                     &is_complex);

                if ((mend-mbeg) == 2) {
                  /// eigenvalues are from wilkinson shift
                  er[(mbeg+0)*ers] = lambda1.real(); ei[(mbeg+0)*eis] = lambda1.imag();
                  er[(mbeg+1)*ers] = lambda2.real(); ei[(mbeg+1)*eis] = lambda2.imag();
                  sub2x2[1*hs0+0*hs1] = zero;
                } else {
                  SerialFrancisInternal::invoke(mend-mbeg,
                                                H+hs*mbeg, hs0, hs1,
                                                lambda1, lambda2,
                                                is_complex);
                  const auto     val1 = ats::abs(*(sub2x2+hs0));
                  const auto     val2 = ats::abs(*(sub2x2-hs1));
                  const auto ref_val1 = ats::abs(*(sub2x2+hs ));
                  const auto ref_val2 = ats::abs(*(sub2x2    ));

                  if (val1 < tol*ref_val1) { 
                    er[(mend-1)*ers] = sub2x2[hs]; ei[(mend-1)*eis] = zero;
                  } else if (val2 < tol*(ref_val1+ref_val2)) {
                    er[(mend-1)*ers] = lambda1.real(); ei[(mend-1)*eis] = lambda1.imag();
                    er[(mend-2)*ers] = lambda2.real(); ei[(mend-2)*eis] = lambda2.imag();
                    sub2x2[hs0] = zero; // consider two eigenvalues are converged 
                  }
                }
              }
#             endif
                                             
            } else {
              /// all eigenvalues are converged
              converge = true;
            }
            ++iter;
          }
          /// Step 3: record missing real eigenvalues from the diagonals
          if (converge) {
            for (int i=0;i<m;++i) 
              if (ats::isNan(er[i*ers])) {
                er[i*ers] = H[i*hs]; ei[i*eis] = zero;
              }
            r_val = 0;
          } else {
            r_val = -1;
          }
          break;
        }          
        }
        return r_val;
      }

      /// complex interface
      template<typename RealType>
      KOKKOS_INLINE_FUNCTION
      static int
      invoke(const int m,
             /* */ RealType * H, const int hs0, const int hs1,
             /* */ Kokkos::complex<RealType> * e, const int es,
             const int max_iteration = 300,
             const RealType user_tolerence = RealType(-1),
             const bool restart = false) {
        RealType * er = (RealType*)e; 
        RealType * ei = er+1;
        const int two_es = 2*es;
        return invoke(m,
                      H, hs0, hs1, 
                      er, two_es,
                      ei, two_es,
                      max_iteration,
                      user_tolerence,
                      restart);
      }
    };



  }/// end namespace Experimental
} /// end namespace KokkosBatched


#endif
