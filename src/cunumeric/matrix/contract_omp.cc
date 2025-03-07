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

#include "cunumeric/matrix/contract.h"
#include "cunumeric/matrix/contract_template.inl"

#include <tblis/tblis.h>
#include <omp.h>

namespace cunumeric {

using namespace Legion;
using namespace tblis;

template <>
struct ContractImplBody<VariantKind::OMP, LegateTypeCode::FLOAT_LT> {
  void operator()(float* lhs_data,
                  size_t lhs_ndim,
                  int64_t* lhs_shape,
                  int64_t* lhs_strides,
                  int32_t* lhs_modes,
                  const float* rhs1_data,
                  size_t rhs1_ndim,
                  int64_t* rhs1_shape,
                  int64_t* rhs1_strides,
                  int32_t* rhs1_modes,
                  const float* rhs2_data,
                  size_t rhs2_ndim,
                  int64_t* rhs2_shape,
                  int64_t* rhs2_strides,
                  int32_t* rhs2_modes)
  {
    tblis_tensor lhs;
    tblis_init_tensor_s(&lhs, lhs_ndim, lhs_shape, lhs_data, lhs_strides);

    tblis_tensor rhs1;
    tblis_init_tensor_s(&rhs1, rhs1_ndim, rhs1_shape, const_cast<float*>(rhs1_data), rhs1_strides);

    tblis_tensor rhs2;
    tblis_init_tensor_s(&rhs2, rhs2_ndim, rhs2_shape, const_cast<float*>(rhs2_data), rhs2_strides);

    tblis_tensor_mult(nullptr, nullptr, &rhs1, rhs1_modes, &rhs2, rhs2_modes, &lhs, lhs_modes);
  }
};

template <>
struct ContractImplBody<VariantKind::OMP, LegateTypeCode::DOUBLE_LT> {
  void operator()(double* lhs_data,
                  size_t lhs_ndim,
                  int64_t* lhs_shape,
                  int64_t* lhs_strides,
                  int32_t* lhs_modes,
                  const double* rhs1_data,
                  size_t rhs1_ndim,
                  int64_t* rhs1_shape,
                  int64_t* rhs1_strides,
                  int32_t* rhs1_modes,
                  const double* rhs2_data,
                  size_t rhs2_ndim,
                  int64_t* rhs2_shape,
                  int64_t* rhs2_strides,
                  int32_t* rhs2_modes)
  {
    tblis_tensor lhs;
    tblis_init_tensor_d(&lhs, lhs_ndim, lhs_shape, lhs_data, lhs_strides);

    tblis_tensor rhs1;
    tblis_init_tensor_d(&rhs1, rhs1_ndim, rhs1_shape, const_cast<double*>(rhs1_data), rhs1_strides);

    tblis_tensor rhs2;
    tblis_init_tensor_d(&rhs2, rhs2_ndim, rhs2_shape, const_cast<double*>(rhs2_data), rhs2_strides);

    tblis_tensor_mult(nullptr, nullptr, &rhs1, rhs1_modes, &rhs2, rhs2_modes, &lhs, lhs_modes);
  }
};

template <>
struct ContractImplBody<VariantKind::OMP, LegateTypeCode::COMPLEX64_LT> {
  void operator()(complex<float>* lhs_data,
                  size_t lhs_ndim,
                  int64_t* lhs_shape,
                  int64_t* lhs_strides,
                  int32_t* lhs_modes,
                  const complex<float>* rhs1_data,
                  size_t rhs1_ndim,
                  int64_t* rhs1_shape,
                  int64_t* rhs1_strides,
                  int32_t* rhs1_modes,
                  const complex<float>* rhs2_data,
                  size_t rhs2_ndim,
                  int64_t* rhs2_shape,
                  int64_t* rhs2_strides,
                  int32_t* rhs2_modes)
  {
    tblis_tensor lhs;
    tblis_init_tensor_c(
      &lhs, lhs_ndim, lhs_shape, reinterpret_cast<std::complex<float>*>(lhs_data), lhs_strides);

    tblis_tensor rhs1;
    tblis_init_tensor_c(
      &rhs1,
      rhs1_ndim,
      rhs1_shape,
      reinterpret_cast<std::complex<float>*>(const_cast<complex<float>*>(rhs1_data)),
      rhs1_strides);

    tblis_tensor rhs2;
    tblis_init_tensor_c(
      &rhs2,
      rhs2_ndim,
      rhs2_shape,
      reinterpret_cast<std::complex<float>*>(const_cast<complex<float>*>(rhs2_data)),
      rhs2_strides);

    tblis_tensor_mult(nullptr, nullptr, &rhs1, rhs1_modes, &rhs2, rhs2_modes, &lhs, lhs_modes);
  }
};

template <>
struct ContractImplBody<VariantKind::OMP, LegateTypeCode::COMPLEX128_LT> {
  void operator()(complex<double>* lhs_data,
                  size_t lhs_ndim,
                  int64_t* lhs_shape,
                  int64_t* lhs_strides,
                  int32_t* lhs_modes,
                  const complex<double>* rhs1_data,
                  size_t rhs1_ndim,
                  int64_t* rhs1_shape,
                  int64_t* rhs1_strides,
                  int32_t* rhs1_modes,
                  const complex<double>* rhs2_data,
                  size_t rhs2_ndim,
                  int64_t* rhs2_shape,
                  int64_t* rhs2_strides,
                  int32_t* rhs2_modes)
  {
    tblis_tensor lhs;
    tblis_init_tensor_z(
      &lhs, lhs_ndim, lhs_shape, reinterpret_cast<std::complex<double>*>(lhs_data), lhs_strides);

    tblis_tensor rhs1;
    tblis_init_tensor_z(
      &rhs1,
      rhs1_ndim,
      rhs1_shape,
      reinterpret_cast<std::complex<double>*>(const_cast<complex<double>*>(rhs1_data)),
      rhs1_strides);

    tblis_tensor rhs2;
    tblis_init_tensor_z(
      &rhs2,
      rhs2_ndim,
      rhs2_shape,
      reinterpret_cast<std::complex<double>*>(const_cast<complex<double>*>(rhs2_data)),
      rhs2_strides);

    tblis_tensor_mult(nullptr, nullptr, &rhs1, rhs1_modes, &rhs2, rhs2_modes, &lhs, lhs_modes);
  }
};

/*static*/ void ContractTask::omp_variant(legate::TaskContext& context)
{
  std::stringstream ss;
  ss << omp_get_max_threads();
  std::string str = ss.str();
  setenv("TBLIS_NUM_THREADS", str.data(), false /*overwrite*/);
  contract_template<VariantKind::OMP>(context);
}

}  // namespace cunumeric
