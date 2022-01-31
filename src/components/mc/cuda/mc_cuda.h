/**
 * Copyright (C) Mellanox Technologies Ltd. 2021.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCC_MC_CUDA_H_
#define UCC_MC_CUDA_H_

#include "components/mc/base/ucc_mc_base.h"
#include "components/mc/ucc_mc_log.h"
#include "utils/ucc_mpool.h"
#include <cuda_runtime.h>

static inline ucc_status_t cuda_error_to_ucc_status(cudaError_t cu_err)
{
    switch(cu_err) {
    case cudaSuccess:
        return UCC_OK;
    case cudaErrorNotReady:
        return UCC_INPROGRESS;
    default:
        break;
    }
    return UCC_ERR_NO_MESSAGE;
}

typedef struct ucc_mc_cuda_config {
    ucc_mc_config_t                super;
    unsigned long                  reduce_num_blocks;
    int                            reduce_num_threads;
    size_t                         mpool_elem_size;
    int                            mpool_max_elems;
} ucc_mc_cuda_config_t;

typedef struct ucc_mc_cuda {
    ucc_mc_base_t                  super;
    cudaStream_t                   stream;
    ucc_mpool_t                    events;
    ucc_mpool_t                    strm_reqs;
    ucc_mpool_t                    mpool;
    int                            mpool_init_flag;
    ucc_spinlock_t                 init_spinlock;
    ucc_thread_mode_t              thread_mode;
} ucc_mc_cuda_t;

ucc_status_t ucc_mc_cuda_reduce(const void *src1, const void *src2,
                                void *dst, size_t count, ucc_datatype_t dt,
                                ucc_reduction_op_t op);

ucc_status_t ucc_mc_cuda_reduce_multi(const void *src1, const void *src2,
                                      void *dst, size_t n_vectors,
                                      size_t count, size_t stride,
                                      ucc_datatype_t dt,
                                      ucc_reduction_op_t op);

ucc_status_t
ucc_mc_cuda_reduce_multi_alpha(const void *src1, const void *src2, void *dst,
                               size_t n_vectors, size_t count, size_t stride,
                               ucc_datatype_t dt, ucc_reduction_op_t reduce_op,
                               ucc_reduction_op_t vector_op, double alpha);

extern ucc_mc_cuda_t ucc_mc_cuda;
#define CUDACHECK(cmd) do {                                                    \
        cudaError_t e = cmd;                                                   \
        if(e != cudaSuccess) {                                                 \
            mc_error(&ucc_mc_cuda.super, "cuda failed with ret:%d(%s)", e,     \
                     cudaGetErrorString(e));                                   \
            return UCC_ERR_NO_MESSAGE;                                         \
        }                                                                      \
} while(0)

#define CUDA_FUNC(_func)                                                       \
    ({                                                                         \
        ucc_status_t _status = UCC_OK;                                         \
        do {                                                                   \
            cudaError_t _result = (_func);                                     \
            if (cudaSuccess != _result) {                                      \
                mc_error(&ucc_mc_cuda.super, "%s() failed: %s",                \
                       #_func, cudaGetErrorString(_result));                   \
                _status = UCC_ERR_INVALID_PARAM;                               \
            }                                                                  \
        } while (0);                                                           \
        _status;                                                               \
    })

#define CUDADRV_FUNC(_func)                                                    \
    ({                                                                         \
        ucc_status_t _status = UCC_OK;                                         \
        do {                                                                   \
            CUresult _result = (_func);                                        \
            const char *cu_err_str;                                            \
            if (CUDA_SUCCESS != _result) {                                     \
                cuGetErrorString(_result, &cu_err_str);                        \
                mc_error(&ucc_mc_cuda.super, "%s() failed: %s",                \
                        #_func, cu_err_str);                                   \
                _status = UCC_ERR_INVALID_PARAM;                               \
            }                                                                  \
        } while (0);                                                           \
        _status;                                                               \
    })

#define MC_CUDA_CONFIG                                                         \
    (ucc_derived_of(ucc_mc_cuda.super.config, ucc_mc_cuda_config_t))

#define UCC_MC_CUDA_INIT_STREAM() do {                                         \
    if (ucc_mc_cuda.stream == NULL) {                                          \
        cudaError_t cuda_st = cudaSuccess;                                     \
        ucc_spin_lock(&ucc_mc_cuda.init_spinlock);                             \
        if (ucc_mc_cuda.stream == NULL) {                                      \
            cuda_st = cudaStreamCreateWithFlags(&ucc_mc_cuda.stream,           \
                                                cudaStreamNonBlocking);        \
        }                                                                      \
        ucc_spin_unlock(&ucc_mc_cuda.init_spinlock);                           \
        if(cuda_st != cudaSuccess) {                                           \
            mc_error(&ucc_mc_cuda.super, "cuda failed with ret:%d(%s)",        \
                     cuda_st, cudaGetErrorString(cuda_st));                    \
            return UCC_ERR_NO_MESSAGE;                                         \
        }                                                                      \
    }                                                                          \
} while(0)

#endif
