/* Copyright 2021 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "cunumeric/matrix/syrk.h"
#include "cunumeric/matrix/syrk_template.inl"

#include <cblas.h>

namespace cunumeric {

using namespace Legion;
using namespace legate;

template <typename Syrk, typename VAL>
static inline void syrk_template(Syrk syrk, VAL* lhs, const VAL* rhs, int32_t m, int32_t n)
{
  auto uplo  = CblasLower;
  auto trans = CblasNoTrans;

  syrk(CblasColMajor, uplo, trans, m, n, -1.0, rhs, m, 1.0, lhs, m);
}

template <typename Syrk, typename VAL>
static inline void complex_syrk_template(Syrk syrk, VAL* lhs, const VAL* rhs, int32_t m, int32_t n)
{
  auto uplo  = CblasLower;
  auto trans = CblasNoTrans;

  syrk(CblasColMajor, uplo, trans, m, n, -1.0, rhs, m, 1.0, lhs, m);
}

template <>
struct SyrkImplBody<VariantKind::CPU, LegateTypeCode::FLOAT_LT> {
  void operator()(float* lhs, const float* rhs, int32_t m, int32_t n)
  {
    syrk_template(cblas_ssyrk, lhs, rhs, m, n);
  }
};

template <>
struct SyrkImplBody<VariantKind::CPU, LegateTypeCode::DOUBLE_LT> {
  void operator()(double* lhs, const double* rhs, int32_t m, int32_t n)
  {
    syrk_template(cblas_dsyrk, lhs, rhs, m, n);
  }
};

template <>
struct SyrkImplBody<VariantKind::CPU, LegateTypeCode::COMPLEX64_LT> {
  void operator()(complex<float>* lhs_, const complex<float>* rhs_, int32_t m, int32_t n)
  {
    auto lhs = reinterpret_cast<__complex__ float*>(lhs_);
    auto rhs = reinterpret_cast<const __complex__ float*>(rhs_);

    complex_syrk_template(cblas_cherk, lhs, rhs, m, n);
  }
};

template <>
struct SyrkImplBody<VariantKind::CPU, LegateTypeCode::COMPLEX128_LT> {
  void operator()(complex<double>* lhs_, const complex<double>* rhs_, int32_t m, int32_t n)
  {
    auto lhs = reinterpret_cast<__complex__ double*>(lhs_);
    auto rhs = reinterpret_cast<const __complex__ double*>(rhs_);

    complex_syrk_template(cblas_zherk, lhs, rhs, m, n);
  }
};

/*static*/ void SyrkTask::cpu_variant(TaskContext& context)
{
#ifdef LEGATE_USE_OPENMP
  openblas_set_num_threads(1);  // make sure this isn't overzealous
#endif
  syrk_template<VariantKind::CPU>(context);
}

namespace  // unnamed
{
static void __attribute__((constructor)) register_tasks(void) { SyrkTask::register_variants(); }
}  // namespace

}  // namespace cunumeric
