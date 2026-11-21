/*
Copyright (c) 1990 Regents of the University of California.
All rights reserved.
 */
/*
FUNCTION
<<system>>---execute command string

INDEX
        system
INDEX
        _system_r

SYNOPSIS
        #include <stdlib.h>
        int system(char *<[s]>);

        int _system_r(void *<[reent]>, char *<[s]>);

DESCRIPTION

Use <<system>> to pass a command string <<*<[s]>>> to <</bin/sh>> on
your system, and wait for it to finish executing.

Use ``<<system(NULL)>>'' to test whether your system has <</bin/sh>>
available.

The alternate function <<_system_r>> is a reentrant version.  The
extra argument <[reent]> is a pointer to a reentrancy structure.

RETURNS
<<system(NULL)>> returns a non-zero value if <</bin/sh>> is available, and
<<0>> if it is not.

With a command argument, the result of <<system>> is the exit status
returned by <</bin/sh>>.

PORTABILITY
ANSI C requires <<system>>, but leaves the nature and effects of a
command processor undefined.  ANSI C does, however, specify that
<<system(NULL)>> return zero or nonzero to report on the existence of
a command processor.

POSIX.2 requires <<system>>, and requires that it invoke a <<sh>>.
Where <<sh>> is found is left unspecified.

Supporting OS subroutines required: <<_exit>>, <<execve>>, <<fork>>,
<<sigaction>>, <<sigprocmask>>, <<waitpid>>.
*/

#define _POSIX_C_SOURCE 202405L
#include <alloca.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

/*
 * As POSIX describes: fork, then execl(<shell>, "sh", "-c", "--",
 * command, NULL), but without the "--" (see below). SIGINT and SIGQUIT
 * are ignored and SIGCHLD blocked while waiting; the child gets the
 * caller's dispositions and mask back before running the shell.
 */
int
system(const char *command)
{
    struct sigaction ign = { .sa_handler = SIG_IGN };
    struct sigaction old_int, old_quit;
    sigset_t         chld, old_mask;
    char            *argv[4];
    pid_t            pid;
    int              status;
    int              err = 0;

    /* Report whether there's a shell to run commands */
    if (command == NULL)
        return access("/bin/sh", X_OK) == 0;

    sigemptyset(&ign.sa_mask);
    sigemptyset(&chld);
    sigaddset(&chld, SIGCHLD);

    (void)sigprocmask(SIG_BLOCK, &chld, &old_mask);
    (void)sigaction(SIGINT, &ign, &old_int);
    (void)sigaction(SIGQUIT, &ign, &old_quit);

    /*
     * Some shells (FreeBSD's) take the argument after -c as the
     * command, even "--", so this leaves out the "--" POSIX shows. A
     * command starting with '-' or '+' gets a leading blank, which the
     * shell skips, instead, so it isn't taken as options.
     */
    if (command[0] == '-' || command[0] == '+') {
        size_t len = strlen(command);
        char  *blank = alloca(len + 2);

        blank[0] = ' ';
        memcpy(blank + 1, command, len + 1);
        command = blank;
    }
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = (char *)command;
    argv[3] = NULL;

    pid = fork();
    if (pid == 0) {
        (void)sigaction(SIGINT, &old_int, NULL);
        (void)sigaction(SIGQUIT, &old_quit, NULL);
        (void)sigprocmask(SIG_SETMASK, &old_mask, NULL);
        execve("/bin/sh", argv, environ);
        _exit(127);
    }

    if (pid < 0) {
        err = errno;
        status = -1;
    } else {
        /* Other signals may interrupt the wait; it ends with the child */
        while (waitpid(pid, &status, 0) < 0) {
            if (errno != EINTR) {
                err = errno;
                status = -1;
                break;
            }
        }
    }

    (void)sigaction(SIGINT, &old_int, NULL);
    (void)sigaction(SIGQUIT, &old_quit, NULL);
    (void)sigprocmask(SIG_SETMASK, &old_mask, NULL);

    if (status == -1)
        errno = err;
    return status;
}
