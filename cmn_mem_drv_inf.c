/* ====================================================================
 *
 * 
 *   BSD LICENSE
 * 
 *   Copyright(c) 2016 Intel Corporation.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * 
 * ====================================================================
 */

/*****************************************************************************
 * @file cmn_mem_drv_inf.c
 *
 * This file provides an interface to use a memory driver to provide contig
 * pinned memory.
 *
 *****************************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "qat_utils.h"
#include "cmn_mem_drv_inf.h"
#include "qae_mem.h"

static pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER;

#ifdef QAT_DEBUG
# define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
# define DEBUG(...)
#endif

#define ERROR(...) fprintf(stderr, __VA_ARGS__)

#ifdef QAT_WARN
# define WARN(...) fprintf (stderr, __VA_ARGS__)
#else
# define WARN(...)
#endif

void qaeCryptoMemFree(void *ptr)
{
    int rc;

    DEBUG("%s: Address: %p\n", __func__, ptr);

    if (NULL == ptr) {
        WARN("qaeCryptoMemFree trying to free NULL pointer.\n");
        return;
    }

    DEBUG("%s: pthread_mutex_lock\n", __func__);
    if ((rc = pthread_mutex_lock(&mem_mutex)) != 0) {
        ERROR("pthread_mutex_lock: %s\n", strerror(rc));
        return;
    }

    qaeMemFreeNUMA(&ptr);

    if ((rc = pthread_mutex_unlock(&mem_mutex)) != 0) {
        ERROR("pthread_mutex_unlock: %s\n", strerror(rc));
        return;
    }
    DEBUG("%s: pthread_mutex_unlock\n", __func__);
}

void *qaeCryptoMemAlloc(size_t memsize, const char *file, int line)
{
    CpaStatus status = CPA_STATUS_SUCCESS;
    int rc;
    void *pAddress = NULL;

    DEBUG("%s: pthread_mutex_lock\n", __func__);
    if ((rc = pthread_mutex_lock(&mem_mutex)) != 0) {
        ERROR("pthread_mutex_lock: %s\n", strerror(rc));
        return NULL;
    }

    pAddress = qaeMemAllocNUMA(memsize, 0, QAT_BYTE_ALIGNMENT);
    DEBUG("%s: Address: %p Size: %d File: %s:%d\n", __func__, pAddress,
          memsize, file, line);
    if ((rc = pthread_mutex_unlock(&mem_mutex)) != 0) {
        ERROR("pthread_mutex_unlock: %s\n", strerror(rc));
    }
    DEBUG("%s: pthread_mutex_unlock\n", __func__);
    return pAddress;
}

void *qaeCryptoMemRealloc(void *ptr, size_t memsize, const char *file,
                          int line)
{
    void *nptr;

    nptr = copyAllocPinnedMemory(ptr, memsize, file, line);
    if (nptr) {
        qaeCryptoMemFree(ptr);
    }
    return nptr;
}

void *qaeCryptoMemReallocClean(void *ptr, size_t memsize,
                               size_t original_size, const char *file,
                               int line)
{
    void *nptr;

    if (original_size > memsize)
        return NULL;

    nptr =
        copyAllocPinnedMemoryClean(ptr, memsize, original_size, file, line);
    if (nptr) {
        qaeCryptoMemFree(ptr);
    }
    return nptr;
}

void *copyAllocPinnedMemory(void *ptr, size_t size, const char *file,
                            int line)
{
    void *nptr;

    if ((nptr = qaeCryptoMemAlloc(size, file, line)) == NULL) {
        WARN("%s: pinned memory allocation failure\n", __func__);
        return NULL;
    }
    memcpy(nptr, ptr, size);
    return nptr;
}

void *copyAllocPinnedMemoryClean(void *ptr, size_t size, size_t original_size,
                                 const char *file, int line)
{
    void *nptr;

    if ((nptr = qaeCryptoMemAlloc(size, file, line)) == NULL) {
        WARN("%s: pinned memory allocation failure\n", __func__);
        return NULL;
    }
    memcpy(nptr, ptr, original_size);
    return nptr;
}

void copyFreePinnedMemory(void *uptr, void *kptr, int size)
{
    memcpy(uptr, kptr, size);
    qaeCryptoMemFree(kptr);
}

CpaPhysicalAddr qaeCryptoMemV2P(void *v)
{
    return qaeVirtToPhysNUMA(v);
}

void qaeCryptoAtFork()
{
    qaeAtFork();
}

