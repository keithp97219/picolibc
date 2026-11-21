/*
Copyright (c) 1990 Regents of the University of California.
All rights reserved.
 */
/* NetWare can not use this implementation of abort.  It provides its
   own version of abort in clib.nlm.  If we can not use clib.nlm, then
   we must write abort in sys/netware.  */

#ifdef ABORT_PROVIDED

int _dummy_abort = 1;

#else

/*
FUNCTION
<<abort>>---abnormal termination of a program

INDEX
        abort

SYNOPSIS
        #include <stdlib.h>
        void abort(void);

DESCRIPTION
Use <<abort>> to signal that your program has detected a condition it
cannot deal with.  Normally, <<abort>> ends your program's execution.

Before terminating your program, <<abort>> raises the exception <<SIGABRT>>
(using `<<raise(SIGABRT)>>'), even when it is blocked or ignored. A
handler registered for it may keep control by not returning; when it
returns, <<abort>> restores the default action and raises <<SIGABRT>>
again, so the program ends as one killed by that signal.

In this implementation, <<abort>> does not perform any stream- or
file-related cleanup (the host environment may do so; if not, you can
arrange for your program to do its own cleanup with a <<SIGABRT>>
exception handler).

RETURNS
<<abort>> does not return to its caller.

PORTABILITY
ANSI C requires <<abort>>.

Supporting OS subroutines required: <<_exit>>, <<raise>>, <<signal>>,
<<sigprocmask>> and optionally, <<write>>.
*/

#define _POSIX_C_SOURCE 202405L
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void
abort(void)
{
    sigset_t abrt;

#ifdef ABORT_MESSAGE
    write(2, "Abort called\n", sizeof("Abort called\n") - 1);
#endif

    sigemptyset(&abrt);
    sigaddset(&abrt, SIGABRT);

    /* POSIX: abort overrides blocking or ignoring SIGABRT */
    (void)sigprocmask(SIG_UNBLOCK, &abrt, NULL);
    (void)raise(SIGABRT);

    /* SIGABRT is ignored, or its handler returned */
    (void)signal(SIGABRT, SIG_DFL);
    (void)sigprocmask(SIG_UNBLOCK, &abrt, NULL);
    (void)raise(SIGABRT);

    for (;;)
        _exit(127);
}

#endif
