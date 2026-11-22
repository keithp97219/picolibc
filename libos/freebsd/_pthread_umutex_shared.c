/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2026 Keith Packard
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "local-umutex.h"
#include <freebsd/freebsd-errno.h>
#include <freebsd/freebsd-param.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * The off-pages of process-shared priority inheritance and robust
 * mutexes, which the kernel allocates and keys by the memory holding
 * the mutex. Each process maps a page once and remembers it by the
 * mutex's address, so locking needs no system call.
 *
 * A cached page can go stale: another process may destroy the mutex
 * (marking the page destroyed and dropping the kernel's key, so the
 * next pthread_mutex_init there gets a new page), or this process may
 * unmap the memory and map other memory, with another mutex, at that
 * address. Each initialization stores a new number in the page and in
 * the mutex's __state, so a page is current only while the two match
 * and it isn't marked destroyed; otherwise the process unmaps it and
 * asks the kernel again. A stale page left unused stays mapped until
 * the next lookup of its address, or of another in its hash bucket,
 * finds it destroyed.
 *
 * The first few entries come from a static pool, so a process can use
 * a few process-shared mutexes even when malloc fails. Locking a
 * mutex fails with ENOMEM when its page can't be cached, as the page
 * must stay mapped while the mutex is held (the robust list holds its
 * address).
 */

#define CACHE_BUCKETS 32

struct umutex_cache {
    struct umutex_cache    *next;
    pthread_mutex_t        *mutex;
    struct _pthread_umutex *page;
};

#define CACHE_STATIC 16

static struct umutex_cache *umutex_cache[CACHE_BUCKETS];
static struct umutex_cache  umutex_cache_static[CACHE_STATIC];
static pthread_rwlock_t     umutex_cache_lock = PTHREAD_RWLOCK_INITIALIZER;

/* The cache must be write-locked for these */
static struct umutex_cache *
cache_alloc(void)
{
    for (int i = 0; i < CACHE_STATIC; i++)
        if (!umutex_cache_static[i].page)
            return &umutex_cache_static[i];
    return malloc(sizeof(struct umutex_cache));
}

static void
cache_free(struct umutex_cache *c)
{
    if (c >= umutex_cache_static && c < umutex_cache_static + CACHE_STATIC)
        c->page = NULL;
    else
        free(c);
}

static struct umutex_cache **
cache_bucket(pthread_mutex_t *mutex)
{
    uintptr_t key = (uintptr_t)mutex;

    key ^= key >> 7;
    key ^= key >> 13;
    return &umutex_cache[key % CACHE_BUCKETS];
}

static bool
page_current(struct _pthread_umutex *page, unsigned int gen)
{
    return __atomic_load_n(&page->gen, __ATOMIC_ACQUIRE) == gen
        && !__atomic_load_n(&page->destroyed, __ATOMIC_ACQUIRE);
}

static void
page_unmap(struct _pthread_umutex *page)
{
    (void)_syscall_raw(FREEBSD_SYS_munmap, page, FREEBSD_PAGE_SIZE);
}

/* Map the kernel's page for the mutex: op is UMTX_SHM_CREAT or UMTX_SHM_LOOKUP */
static struct _pthread_umutex *
page_map(pthread_mutex_t *mutex, int op, int *error)
{
    long fd, addr;

    fd = _syscall_raw(FREEBSD_SYS__umtx_op, NULL, FREEBSD_UMTX_OP_SHM, (long)op, mutex, NULL);
    if (fd < 0) {
        *error = _syscall_errno(fd);
        return NULL;
    }
    /* The offset is 64 bits, so pass it as a long */
    addr = _syscall_raw(FREEBSD_SYS_mmap, NULL, FREEBSD_PAGE_SIZE,
                        FREEBSD_PROT_READ | FREEBSD_PROT_WRITE, FREEBSD_MAP_SHARED, fd, (long)0);
    (void)_syscall_raw(FREEBSD_SYS_close, fd);
    if (addr < 0) {
        *error = ENOMEM;
        return NULL;
    }
    return (struct _pthread_umutex *)addr;
}

/*
 * Drop the mutex's entry, and any of its bucket whose page was
 * destroyed; the cache must be write-locked
 */
static void
cache_sweep(pthread_mutex_t *mutex)
{
    struct umutex_cache **prev, *c;

    for (prev = cache_bucket(mutex); (c = *prev) != NULL;) {
        if (c->mutex == mutex || __atomic_load_n(&c->page->destroyed, __ATOMIC_ACQUIRE)) {
            *prev = c->next;
            page_unmap(c->page);
            cache_free(c);
        } else {
            prev = &c->next;
        }
    }
}

/*
 * Remember a page for the mutex, returning the one to use: another
 * thread may have cached a current one first
 */
static struct _pthread_umutex *
cache_insert(pthread_mutex_t *mutex, struct _pthread_umutex *page, unsigned int gen, int *error)
{
    struct umutex_cache **bucket = cache_bucket(mutex), *c;

    pthread_rwlock_wrlock(&umutex_cache_lock);
    for (c = *bucket; c; c = c->next) {
        if (c->mutex == mutex && page_current(c->page, gen)) {
            pthread_rwlock_unlock(&umutex_cache_lock);
            page_unmap(page);
            return c->page;
        }
    }
    cache_sweep(mutex);
    c = cache_alloc();
    if (c) {
        c->mutex = mutex;
        c->page = page;
        c->next = *bucket;
        *bucket = c;
    }
    pthread_rwlock_unlock(&umutex_cache_lock);
    if (!c) {
        page_unmap(page);
        *error = ENOMEM;
        return NULL;
    }
    return page;
}

struct _pthread_umutex *
_pthread_umutex_shared(pthread_mutex_t *mutex, int *error)
{
    unsigned int            gen = __atomic_load_n(&mutex->__state, __ATOMIC_ACQUIRE);
    struct _pthread_umutex *page = NULL;
    struct umutex_cache    *c;

    pthread_rwlock_rdlock(&umutex_cache_lock);
    for (c = *cache_bucket(mutex); c; c = c->next) {
        if (c->mutex == mutex) {
            page = c->page;
            break;
        }
    }
    pthread_rwlock_unlock(&umutex_cache_lock);
    if (page && page_current(page, gen))
        return page;

    /* Not cached, or stale: ask the kernel */
    *error = EINVAL;
    if (gen == 0)
        return NULL;
    page = page_map(mutex, FREEBSD_UMTX_SHM_LOOKUP, error);
    if (!page) {
        /* No page there: the mutex was destroyed, or never initialized */
        if (*error != ENOMEM)
            *error = EINVAL;
        return NULL;
    }
    if (!page_current(page, gen)) {
        page_unmap(page);
        *error = EINVAL;
        return NULL;
    }
    return cache_insert(mutex, page, gen, error);
}

/* A number for a new initialization, differing from any before it at that address */
static unsigned int
new_gen(void)
{
    static unsigned int count;
    unsigned int        pid = (unsigned int)_syscall_raw(FREEBSD_SYS_getpid);
    unsigned int        gen;

    do
        gen = pid * 2654435761U + __atomic_add_fetch(&count, 1, __ATOMIC_RELAXED);
    while (gen == 0);
    return gen;
}

int
_pthread_umutex_shared_create(pthread_mutex_t *mutex, __uint32_t flags)
{
    struct _pthread_umutex *page;
    unsigned int            gen;
    int                     error = 0;

    /* Forget any page for an earlier mutex here */
    pthread_rwlock_wrlock(&umutex_cache_lock);
    cache_sweep(mutex);
    pthread_rwlock_unlock(&umutex_cache_lock);

    /* The kernel returns the existing page if the last mutex here wasn't destroyed */
    page = page_map(mutex, FREEBSD_UMTX_SHM_CREAT, &error);
    if (!page)
        return error == ENOMEM ? ENOMEM : EAGAIN;
    gen = new_gen();
    memset(page, 0, sizeof(*page));
    page->um.m_flags = flags;
    __atomic_store_n(&page->gen, gen, __ATOMIC_RELEASE);
    __atomic_store_n(&mutex->__state, gen, __ATOMIC_RELEASE);

    /* Without memory to cache it, the page is looked up again when used */
    (void)cache_insert(mutex, page, gen, &error);
    return 0;
}

void
_pthread_umutex_shared_destroy(pthread_mutex_t *mutex, struct _pthread_umutex *page)
{
    /* Other processes caching the page find it destroyed */
    __atomic_store_n(&page->destroyed, 1, __ATOMIC_RELEASE);
    (void)_syscall_raw(FREEBSD_SYS__umtx_op, NULL, FREEBSD_UMTX_OP_SHM,
                       (long)FREEBSD_UMTX_SHM_DESTROY, mutex, NULL);
    pthread_rwlock_wrlock(&umutex_cache_lock);
    cache_sweep(mutex);
    pthread_rwlock_unlock(&umutex_cache_lock);
    __atomic_store_n(&mutex->__state, 0, __ATOMIC_RELEASE);
}

/* The only thread of a fork child can't find the cache locked by another */
void
_pthread_umutex_shared_fork_child(void)
{
    static const pthread_rwlock_t init = PTHREAD_RWLOCK_INITIALIZER;

    umutex_cache_lock = init;
}
