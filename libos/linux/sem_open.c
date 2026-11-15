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

#define _DEFAULT_SOURCE
#include "local-pthread.h"
#include "local-shm.h"
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SEM_PREFIX "sem."

/*
 * Create a new semaphore file at path. The file is filled in under a
 * temporary name and then linked into place, so other processes never
 * see a partially initialized semaphore. Fails with EEXIST when path
 * already exists.
 */
static int
sem_create(const char *path, mode_t mode, unsigned value)
{
    static unsigned int serial;
    char                tmp[_SHM_PATH_MAX];
    char               *t;
    unsigned long       id;
    sem_t               init = { .__value = value, .__waiters = 0, .__pshared = 1 };
    int                 fd;
    int                 save_errno;

    for (;;) {
        /* /dev/shm/sem.tmp.<hex pid><hex serial> */
        memcpy(tmp, _SHM_DIR SEM_PREFIX "tmp.", sizeof(_SHM_DIR SEM_PREFIX "tmp.") - 1);
        t = tmp + sizeof(_SHM_DIR SEM_PREFIX "tmp.") - 1;
        id = ((unsigned long)getpid() << 16) ^ __atomic_fetch_add(&serial, 1, __ATOMIC_RELAXED);
        do {
            *t++ = "0123456789abcdef"[id & 0xf];
            id >>= 4;
        } while (id);
        *t = '\0';

        fd = open(tmp, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, mode);
        if (fd >= 0)
            break;
        if (errno != EEXIST)
            return -1;
    }

    if (write(fd, &init, sizeof(init)) != (ssize_t)sizeof(init) || link(tmp, path) < 0) {
        save_errno = errno;
        (void)unlink(tmp);
        (void)close(fd);
        errno = save_errno;
        return -1;
    }
    (void)unlink(tmp);
    return fd;
}

/* Map the semaphore in fd, reusing an existing mapping of the same file */
static sem_t *
sem_map(int fd)
{
    struct stat        st;
    struct _sem_named *n;
    sem_t             *sem = SEM_FAILED;

    if (fstat(fd, &st) < 0) {
        (void)close(fd);
        return SEM_FAILED;
    }
    pthread_mutex_lock(&_sem_named_lock);
    for (n = _sem_named; n; n = n->next) {
        if (n->dev == st.st_dev && n->ino == st.st_ino) {
            n->refs++;
            sem = n->sem;
            goto done;
        }
    }
    n = malloc(sizeof(*n));
    if (!n) {
        errno = ENOMEM;
        goto done;
    }
    sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sem == MAP_FAILED) {
        free(n);
        sem = SEM_FAILED;
        goto done;
    }
    n->sem = sem;
    n->dev = st.st_dev;
    n->ino = st.st_ino;
    n->refs = 1;
    n->next = _sem_named;
    _sem_named = n;
done:
    pthread_mutex_unlock(&_sem_named_lock);
    (void)close(fd);
    return sem;
}

sem_t *
sem_open(const char *name, int oflag, ...)
{
    char     path[_SHM_PATH_MAX];
    mode_t   mode = 0;
    unsigned value = 0;
    int      fd;
    int      ret;

    if (oflag & O_CREAT) {
        va_list ap;

        va_start(ap, oflag);
        mode = (mode_t)va_arg(ap, unsigned int);
        value = va_arg(ap, unsigned);
        va_end(ap);
        if (value > SEM_VALUE_MAX) {
            errno = EINVAL;
            return SEM_FAILED;
        }
    }

    ret = _shm_path(path, SEM_PREFIX, name);
    if (ret) {
        errno = ret;
        return SEM_FAILED;
    }

    for (;;) {
        if ((oflag & (O_CREAT | O_EXCL)) != (O_CREAT | O_EXCL)) {
            fd = open(path, O_RDWR | O_NOFOLLOW | O_CLOEXEC);
            if (fd >= 0)
                break;
            if (errno != ENOENT || !(oflag & O_CREAT))
                return SEM_FAILED;
        }
        fd = sem_create(path, mode, value);
        if (fd >= 0)
            break;
        /* Lost a race with another creator; open theirs unless O_EXCL */
        if (errno != EEXIST || (oflag & O_EXCL))
            return SEM_FAILED;
    }
    return sem_map(fd);
}
