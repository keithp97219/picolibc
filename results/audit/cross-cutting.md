# Cross-cutting findings

Problems that span headers, recorded by audit units rather than fixed.
Append yours under a heading naming the unit.

## From the pilot and earlier work (x86_64 host)

- `sys/syslimits.h` fixes `OPEN_MAX` at 64; on Linux the limit is
  RLIMIT_NOFILE (1024 or more). posix_spawn now uses `sysconf(_SC_OPEN_MAX)`.
- `PATH_MAX` is 1024 and `PIPE_BUF` 512 in `<limits.h>`; Linux allows 4096
  for both. glob reports ENAMETOOLONG past 1024; pathconf reports the
  `<limits.h>` values.
- `LOGIN_NAME_MAX` isn't defined (allowed when indeterminate).
- `sysconf(_SC_MQ_OPEN_MAX)` returns 8; Linux has no such limit.
- `MQ_PRIO_MAX` isn't in `<limits.h>` (allowed); sysconf reports 32768.
