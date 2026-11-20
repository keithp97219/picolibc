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

#define _POSIX_C_SOURCE 202405L
#include <unistd.h>

/* Without the system support this test needs, it is skipped */
#if !defined(__PICOLIBC__)                                                                     \
    || ((defined(__PICOLIBC_POSIX_FILE_SYSTEM) && __PICOLIBC_POSIX_FILE_SYSTEM > 0)            \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_EXT) && __PICOLIBC_POSIX_FILE_SYSTEM_EXT > 0) \
        && (defined(__PICOLIBC_POSIX_FILE_SYSTEM_FD) && __PICOLIBC_POSIX_FILE_SYSTEM_FD > 0))

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

/*
 * statvfs and fstatvfs describe the file system holding a file. The
 * values depend on the host, so only their relationships are checked.
 */

#define check(condition, message)                      \
    do {                                               \
        if (!(condition)) {                            \
            printf("%s: %s\n", (message), #condition); \
            exit(1);                                   \
        }                                              \
    } while (0)

static char dir[] = "/tmp/fstatvfs-XXXXXX";

static void
check_counts(const struct statvfs *sv)
{
    check(sv->f_bsize > 0 && sv->f_frsize > 0, "block sizes");
    check(sv->f_bfree <= sv->f_blocks, "f_bfree <= f_blocks");
    check(sv->f_bavail <= sv->f_bfree, "f_bavail <= f_bfree");
    check(sv->f_ffree <= sv->f_files, "f_ffree <= f_files");
    check(sv->f_favail <= sv->f_ffree, "f_favail <= f_ffree");
    check(sv->f_namemax > 0, "f_namemax");
#ifdef __PICOLIBC__
    /* glibc passes on the kernel's other mount flags, such as ST_NODEV */
    check((sv->f_flag & ~(unsigned long)(ST_RDONLY | ST_NOSUID)) == 0,
          "f_flag holds only ST_ flags");
#endif
}

static int
has_option(const char *options, const char *option)
{
    size_t len = strlen(option);

    while (options) {
        if (strncmp(options, option, len) == 0 && (options[len] == ',' || options[len] == '\0'))
            return 1;
        options = strchr(options, ',');
        if (options)
            options++;
    }
    return 0;
}

/*
 * On Linux, ST_RDONLY and ST_NOSUID match the "ro" and "nosuid" options
 * in /proc/self/mountinfo. Only the last mount on each mount point is
 * visible, and mount points with escaped characters are skipped.
 */
#define MAX_MOUNTS 256
static char mounts[MAX_MOUNTS][256];
static char options[MAX_MOUNTS][256];

static void
check_mount_flags(void)
{
    FILE          *f = fopen("/proc/self/mountinfo", "r");
    char           line[1024];
    int            n = 0, i, j, checked = 0;
    struct statvfs sv;

    if (!f) {
        printf("no /proc/self/mountinfo, skipping the mount flag checks\n");
        return;
    }
    while (n < MAX_MOUNTS && fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%*s %*s %*s %*s %255s %255s", mounts[n], options[n]) == 2
            && !strchr(mounts[n], '\\'))
            n++;
    }
    fclose(f);
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++)
            if (strcmp(mounts[i], mounts[j]) == 0)
                break;
        if (j < n || statvfs(mounts[i], &sv) != 0)
            continue;
        check_counts(&sv);
        if (!(sv.f_flag & ST_RDONLY) != !has_option(options[i], "ro")) {
            printf("%s: options %s, f_flag %#lx\n", mounts[i], options[i], sv.f_flag);
            check(0, "ST_RDONLY matches the ro option");
        }
        if (!(sv.f_flag & ST_NOSUID) != !has_option(options[i], "nosuid")) {
            printf("%s: options %s, f_flag %#lx\n", mounts[i], options[i], sv.f_flag);
            check(0, "ST_NOSUID matches the nosuid option");
        }
        checked++;
    }
    printf("checked the flags of %d mounts\n", checked);
}

int
main(void)
{
    struct statvfs sv, fsv;
    char           path[PATH_MAX], name[NAME_MAX + 2];
    int            fd, dfd;

    check(mkdtemp(dir) != NULL, "mkdtemp");
    snprintf(path, sizeof(path), "%s/file", dir);
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);
    check(fd >= 0, "create");

    /* statvfs and fstatvfs describe the same file system */
    check(statvfs(path, &sv) == 0, "statvfs");
    check_counts(&sv);
    check(fstatvfs(fd, &fsv) == 0, "fstatvfs");
    check_counts(&fsv);
    check(sv.f_bsize == fsv.f_bsize && sv.f_frsize == fsv.f_frsize && sv.f_blocks == fsv.f_blocks
              && sv.f_files == fsv.f_files && sv.f_fsid == fsv.f_fsid && sv.f_flag == fsv.f_flag
              && sv.f_namemax == fsv.f_namemax,
          "statvfs and fstatvfs agree");

    /* Neither read nor write permission is needed */
    check(fchmod(fd, 0) == 0, "chmod 0");
    check(statvfs(path, &sv) == 0, "statvfs without permission");
    check(fchmod(fd, 0600) == 0, "chmod back");

    /* A directory opened for searching works too */
    dfd = open(dir, O_RDONLY | O_DIRECTORY);
    check(dfd >= 0 && fstatvfs(dfd, &fsv) == 0 && fsv.f_fsid == sv.f_fsid, "fstatvfs directory");
    check(close(dfd) == 0, "close directory");

    /* The flags match the mount options */
    check_mount_flags();

    /* Errors */
    errno = 0;
    check(statvfs("", &sv) == -1 && errno == ENOENT, "ENOENT empty path");
    snprintf(path, sizeof(path), "%s/missing", dir);
    errno = 0;
    check(statvfs(path, &sv) == -1 && errno == ENOENT, "ENOENT");
    snprintf(path, sizeof(path), "%s/file/x", dir);
    errno = 0;
    check(statvfs(path, &sv) == -1 && errno == ENOTDIR, "ENOTDIR");
    snprintf(path, sizeof(path), "%s/file/", dir);
    errno = 0;
    check(statvfs(path, &sv) == -1 && errno == ENOTDIR, "ENOTDIR trailing slash");
    memset(name, 'n', sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
    snprintf(path, sizeof(path), "%s/%s", dir, name);
    errno = 0;
    check(statvfs(path, &sv) == -1 && errno == ENAMETOOLONG, "ENAMETOOLONG");
    snprintf(path, sizeof(path), "%s/loop", dir);
    check(symlink("loop", path) == 0, "symlink loop");
    errno = 0;
    check(statvfs(path, &sv) == -1 && errno == ELOOP, "ELOOP");
    check(unlink(path) == 0, "unlink loop");
    if (geteuid() != 0) {
        /* Root can search anything */
        check(chmod(dir, 0600) == 0, "remove search permission");
        snprintf(path, sizeof(path), "%s/file", dir);
        errno = 0;
        check(statvfs(path, &sv) == -1 && errno == EACCES, "EACCES");
        check(chmod(dir, 0700) == 0, "restore search permission");
    }
    check(close(fd) == 0, "close");
    errno = 0;
    check(fstatvfs(fd, &fsv) == -1 && errno == EBADF, "EBADF");

    snprintf(path, sizeof(path), "%s/file", dir);
    check(unlink(path) == 0 && rmdir(dir) == 0, "clean up");
    printf("success\n");
    return 0;
}

#else

#include <stdio.h>

int
main(void)
{
    printf("no system support, skipping\n");
    return 77;
}

#endif
