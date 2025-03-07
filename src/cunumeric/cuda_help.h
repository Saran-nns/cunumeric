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

#pragma once

#include "legate.h"
#include <cublas_v2.h>
#include <cusolverDn.h>
#include <cuda_runtime.h>
#include <cufft.h>
#include <cutensor.h>

#define THREADS_PER_BLOCK 128
#define MIN_CTAS_PER_SM 4
#define MAX_REDUCTION_CTAS 1024
#define COOPERATIVE_THREADS 256
#define COOPERATIVE_CTAS_PER_SM 4

#define CHECK_CUDA(expr)                    \
  do {                                      \
    cudaError_t result = (expr);            \
    check_cuda(result, __FILE__, __LINE__); \
  } while (false)

#define CHECK_CUBLAS(expr)                    \
  do {                                        \
    cublasStatus_t result = (expr);           \
    check_cublas(result, __FILE__, __LINE__); \
  } while (false)

#define CHECK_CUFFT(expr)                    \
  do {                                       \
    cufftResult result = (expr);             \
    check_cufft(result, __FILE__, __LINE__); \
  } while (false)

#define CHECK_CUSOLVER(expr)                    \
  do {                                          \
    cusolverStatus_t result = (expr);           \
    check_cusolver(result, __FILE__, __LINE__); \
  } while (false)

#define CHECK_CUTENSOR(expr)                    \
  do {                                          \
    cutensorStatus_t result = (expr);           \
    check_cutensor(result, __FILE__, __LINE__); \
  } while (false)

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif

namespace cunumeric {

// Defined in cudalibs.cu

// Return a cached stream for the current GPU
cudaStream_t get_cached_stream();
cublasHandle_t get_cublas();
cusolverDnHandle_t get_cusolver();
cutensorHandle_t* get_cutensor();

__host__ inline void check_cuda(cudaError_t error, const char* file, int line)
{
  if (error != cudaSuccess) {
    fprintf(stderr,
            "Internal CUDA failure with error %s (%s) in file %s at line %d\n",
            cudaGetErrorString(error),
            cudaGetErrorName(error),
            file,
            line);
    exit(error);
  }
}

__host__ inline void check_cublas(cublasStatus_t status, const char* file, int line)
{
  if (status != CUBLAS_STATUS_SUCCESS) {
    fprintf(stderr,
            "Internal cuBLAS failure with error code %d in file %s at line %d\n",
            status,
            file,
            line);
    exit(status);
  }
}

__host__ inline void check_cufft(cufftResult result, const char* file, int line)
{
  if (result != CUFFT_SUCCESS) {
    fprintf(stderr,
            "Internal cuFFT failure with error code %d in file %s at line %d\n",
            result,
            file,
            line);
    exit(result);
  }
}

__host__ inline void check_cusolver(cusolverStatus_t status, const char* file, int line)
{
  if (status != CUSOLVER_STATUS_SUCCESS) {
    fprintf(stderr,
            "Internal cuSOLVER failure with error code %d in file %s at line %d\n",
            status,
            file,
            line);
    exit(status);
  }
}

__host__ inline void check_cutensor(cutensorStatus_t result, const char* file, int line)
{
  if (result != CUTENSOR_STATUS_SUCCESS) {
    fprintf(stderr,
            "Internal Legate CUTENSOR failure with error %s (%d) in file %s at line %d\n",
            cutensorGetErrorString(result),
            result,
            file,
            line);
    exit(result);
  }
}

template <typename T>
__device__ __forceinline__ T shuffle(unsigned mask, T var, int laneMask, int width)
{
  // return __shfl_xor_sync(0xffffffff, value, i, 32);
  int array[(sizeof(T) + sizeof(int) - 1) / sizeof(int)];
  memcpy(array, &var, sizeof(T));
  for (int& value : array) {
    const int tmp = __shfl_xor_sync(mask, value, laneMask, width);
    value         = tmp;
  }
  memcpy(&var, array, sizeof(T));
  return var;
}

// Overload for complex
// TBD: if compiler optimizes out the shuffle function we defined, we could make it the default
// version
template <typename T, typename REDUCTION>
__device__ __forceinline__ void reduce_output(Legion::DeferredReduction<REDUCTION> result,
                                              complex<T> value)
{
  __shared__ complex<T> trampoline[THREADS_PER_BLOCK / 32];
  // Reduce across the warp
  const int laneid = threadIdx.x & 0x1f;
  const int warpid = threadIdx.x >> 5;
  for (int i = 16; i >= 1; i /= 2) {
    const complex<T> shuffle_value = shuffle(0xffffffff, value, i, 32);
    REDUCTION::template fold<true /*exclusive*/>(value, shuffle_value);
  }
  // Write warp values into shared memory
  if ((laneid == 0) && (warpid > 0)) trampoline[warpid] = value;
  __syncthreads();
  // Output reduction
  if (threadIdx.x == 0) {
    for (int i = 1; i < (THREADS_PER_BLOCK / 32); i++)
      REDUCTION::template fold<true /*exclusive*/>(value, trampoline[i]);
    result <<= value;
    // Make sure the result is visible externally
    __threadfence_system();
  }
}

template <typename T, typename REDUCTION>
__device__ __forceinline__ void reduce_output(Legion::DeferredReduction<REDUCTION> result, T value)
{
  __shared__ T trampoline[THREADS_PER_BLOCK / 32];
  // Reduce across the warp
  const int laneid = threadIdx.x & 0x1f;
  const int warpid = threadIdx.x >> 5;
  for (int i = 16; i >= 1; i /= 2) {
    const T shuffle_value = __shfl_xor_sync(0xffffffff, value, i, 32);
    REDUCTION::template fold<true /*exclusive*/>(value, shuffle_value);
  }
  // Write warp values into shared memory
  if ((laneid == 0) && (warpid > 0)) trampoline[warpid] = value;
  __syncthreads();
  // Output reduction
  if (threadIdx.x == 0) {
    for (int i = 1; i < (THREADS_PER_BLOCK / 32); i++)
      REDUCTION::template fold<true /*exclusive*/>(value, trampoline[i]);
    result <<= value;
    // Make sure the result is visible externally
    __threadfence_system();
  }
}

__device__ __forceinline__ void reduce_bool(Legion::DeferredValue<bool> result, int value)
{
  __shared__ int trampoline[THREADS_PER_BLOCK / 32];
  // Reduce across the warp
  const int laneid = threadIdx.x & 0x1f;
  const int warpid = threadIdx.x >> 5;
  for (int i = 16; i >= 1; i /= 2) {
    const int shuffle_value = __shfl_xor_sync(0xffffffff, value, i, 32);
    if (shuffle_value == 0) value = 0;
  }
  // Write warp values into shared memory
  if ((laneid == 0) && (warpid > 0)) trampoline[warpid] = value;
  __syncthreads();
  // Output reduction
  if (threadIdx.x == 0) {
    for (int i = 1; i < (THREADS_PER_BLOCK / 32); i++)
      if (trampoline[i] == 0) {
        value = 0;
        break;
      }
    if (value == 0) {
      result = false;
      // Make sure the result is visible externally
      __threadfence_system();
    }
  }
}

template <typename T>
__device__ __forceinline__ T load_cached(const T* ptr)
{
  return *ptr;
}

// Specializations to use PTX cache qualifiers to keep
// all the input data in as many caches as we can
// Use .ca qualifier to cache at all levels
template <>
__device__ __forceinline__ uint16_t load_cached<uint16_t>(const uint16_t* ptr)
{
  uint16_t value;
  asm volatile("ld.global.ca.u16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint32_t load_cached<uint32_t>(const uint32_t* ptr)
{
  uint32_t value;
  asm volatile("ld.global.ca.u32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint64_t load_cached<uint64_t>(const uint64_t* ptr)
{
  uint64_t value;
  asm volatile("ld.global.ca.u64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int16_t load_cached<int16_t>(const int16_t* ptr)
{
  int16_t value;
  asm volatile("ld.global.ca.s16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int32_t load_cached<int32_t>(const int32_t* ptr)
{
  int32_t value;
  asm volatile("ld.global.ca.s32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int64_t load_cached<int64_t>(const int64_t* ptr)
{
  int64_t value;
  asm volatile("ld.global.ca.s64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

// No half because inline ptx is dumb about the type

template <>
__device__ __forceinline__ float load_cached<float>(const float* ptr)
{
  float value;
  asm volatile("ld.global.ca.f32 %0, [%1];" : "=f"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ double load_cached<double>(const double* ptr)
{
  double value;
  asm volatile("ld.global.ca.f64 %0, [%1];" : "=d"(value) : "l"(ptr) : "memory");
  return value;
}

template <typename T>
__device__ __forceinline__ T load_l2(const T* ptr)
{
  return *ptr;
}

// Specializations to use PTX cache qualifiers to keep
// data loaded into L2 but no higher in the hierarchy
// Use .cg qualifier to cache at L2
template <>
__device__ __forceinline__ uint16_t load_l2<uint16_t>(const uint16_t* ptr)
{
  uint16_t value;
  asm volatile("ld.global.cg.u16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint32_t load_l2<uint32_t>(const uint32_t* ptr)
{
  uint32_t value;
  asm volatile("ld.global.cg.u32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint64_t load_l2<uint64_t>(const uint64_t* ptr)
{
  uint64_t value;
  asm volatile("ld.global.cg.u64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int16_t load_l2<int16_t>(const int16_t* ptr)
{
  int16_t value;
  asm volatile("ld.global.cg.s16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int32_t load_l2<int32_t>(const int32_t* ptr)
{
  int32_t value;
  asm volatile("ld.global.cg.s32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int64_t load_l2<int64_t>(const int64_t* ptr)
{
  int64_t value;
  asm volatile("ld.global.cg.s64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

// No half because inline ptx is dumb about the type

template <>
__device__ __forceinline__ float load_l2<float>(const float* ptr)
{
  float value;
  asm volatile("ld.global.cg.f32 %0, [%1];" : "=f"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ double load_l2<double>(const double* ptr)
{
  double value;
  asm volatile("ld.global.cg.f64 %0, [%1];" : "=d"(value) : "l"(ptr) : "memory");
  return value;
}

template <typename T>
__device__ __forceinline__ T load_streaming(const T* ptr)
{
  return *ptr;
}

// Specializations to use PTX cache qualifiers to prevent
// loads from interfering with cache state
// Use .cs qualifier to mark the line as evict-first
template <>
__device__ __forceinline__ uint16_t load_streaming<uint16_t>(const uint16_t* ptr)
{
  uint16_t value;
  asm volatile("ld.global.cs.u16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint32_t load_streaming<uint32_t>(const uint32_t* ptr)
{
  uint32_t value;
  asm volatile("ld.global.cs.u32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ uint64_t load_streaming<uint64_t>(const uint64_t* ptr)
{
  uint64_t value;
  asm volatile("ld.global.cs.u64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int16_t load_streaming<int16_t>(const int16_t* ptr)
{
  int16_t value;
  asm volatile("ld.global.cs.s16 %0, [%1];" : "=h"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int32_t load_streaming<int32_t>(const int32_t* ptr)
{
  int32_t value;
  asm volatile("ld.global.cs.s32 %0, [%1];" : "=r"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ int64_t load_streaming<int64_t>(const int64_t* ptr)
{
  int64_t value;
  asm volatile("ld.global.cs.s64 %0, [%1];" : "=l"(value) : "l"(ptr) : "memory");
  return value;
}

// No half because inline ptx is dumb about the type

template <>
__device__ __forceinline__ float load_streaming<float>(const float* ptr)
{
  float value;
  asm volatile("ld.global.cs.f32 %0, [%1];" : "=f"(value) : "l"(ptr) : "memory");
  return value;
}

template <>
__device__ __forceinline__ double load_streaming<double>(const double* ptr)
{
  double value;
  asm volatile("ld.global.cs.f64 %0, [%1];" : "=d"(value) : "l"(ptr) : "memory");
  return value;
}

template <typename T>
__device__ __forceinline__ void store_streaming(T* ptr, T value)
{
  *ptr = value;
}

// Specializations to use PTX cache qualifiers to avoid
// invalidating read data from caches as we are writing
// Use .cs qualifier to evict first at all levels
template <>
__device__ __forceinline__ void store_streaming<uint16_t>(uint16_t* ptr, uint16_t value)
{
  asm volatile("st.global.cs.u16 [%0], %1;" : : "l"(ptr), "h"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<uint32_t>(uint32_t* ptr, uint32_t value)
{
  asm volatile("st.global.cs.u32 [%0], %1;" : : "l"(ptr), "r"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<uint64_t>(uint64_t* ptr, uint64_t value)
{
  asm volatile("st.global.cs.u64 [%0], %1;" : : "l"(ptr), "l"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<int16_t>(int16_t* ptr, int16_t value)
{
  asm volatile("st.global.cs.s16 [%0], %1;" : : "l"(ptr), "h"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<int32_t>(int32_t* ptr, int32_t value)
{
  asm volatile("st.global.cs.s32 [%0], %1;" : : "l"(ptr), "r"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<int64_t>(int64_t* ptr, int64_t value)
{
  asm volatile("st.global.cs.s64 [%0], %1;" : : "l"(ptr), "l"(value) : "memory");
}

// No half because inline ptx is dumb about the type

template <>
__device__ __forceinline__ void store_streaming<float>(float* ptr, float value)
{
  asm volatile("st.global.cs.f32 [%0], %1;" : : "l"(ptr), "f"(value) : "memory");
}

template <>
__device__ __forceinline__ void store_streaming<double>(double* ptr, double value)
{
  asm volatile("st.global.cs.f64 [%0], %1;" : : "l"(ptr), "d"(value) : "memory");
}

}  // namespace cunumeric
