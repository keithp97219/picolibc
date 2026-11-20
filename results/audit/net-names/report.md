# <netdb.h>, <arpa/inet.h>, <net/if.h>, <netinet/in.h> audit against POSIX.1-2024 (Issue 8)

Worktree `/local/home/keithpac/audit-net-names`, branch `audit/net-names`,
based on 2d4e737cfc (`claude/posix-spec-audit`). Host: AWS Graviton2,
aarch64 Linux, builds and tests in the Debian sid chroot.

Implementation: `libc/posix/netdb/*.c` (`local-netdb.h`, the text-database
reader `__netdb_db.c`, the thread-safe lookups `__netdb_lookup.c`),
`libc/posix/arpa-inet/*.c`, `libos/linux/if_*.c`; headers
`libc/include/netdb.h`, `arpa/inet.h`, `net/if.h`, `netinet/in.h`, with the
Linux ABI parts in `libos/linux/include/netinet/_in.h` and `net/_if.h`.
Tests: `test/test-posix/test-netdb/`, `test-arpa-inet/`, `test-net-if/`,
`test-netinet-in/` (paths below are relative to `test/test-posix`).

Spec source: pubs.opengroup.org/onlinepubs/9799919799 pages basedefs/netdb.h,
arpa_inet.h, net_if.h, netinet_in.h; functions/freeaddrinfo (getaddrinfo),
gai_strerror, getnameinfo, endhostent, endnetent, endprotoent, endservent,
htonl, inet_addr, inet_ntop, if_nametoindex, if_indextoname, if_nameindex,
if_freenameindex.

Status key: TESTED (test file:line), UNTESTED, NOT TESTABLE (reason).
Every TESTED row was run and passes for picolibc (`test-X`) and against glibc
(`test-X-native`) on aarch64-linux-gnu and arm-linux-gnueabihf, unless it
says picolibc only.

picolibc never uses DNS: it reads `/etc/hosts`, `/etc/services`,
`/etc/protocols` and `/etc/networks`, falling back to a few built-in lines
when a file can't be opened. The tests don't depend on the host's DNS or
network contents: they use the loopback address, numeric flags, the reserved
`.invalid` TLD, or compare against what the same library's database
functions report (`getservbyport`, `gethostent`).

## Requirement table

### Header <netdb.h>

| # | Requirement | Status | Reference |
|---|---|---|---|
| NH1 | struct hostent members with the specified types | TESTED | test-netdb/test-netdb_h.c:57 (_Generic) |
| NH2 | struct netent with uint32_t n_net; uint32_t defined | TESTED | test-netdb_h.c:61 |
| NH3 | struct protoent | TESTED | test-netdb_h.c:65 |
| NH4 | struct servent | TESTED | test-netdb_h.c:68 |
| NH5 | IPPORT_RESERVED defined | TESTED | test-netdb_h.c:78 (value 1024: see observations) |
| NH6 | struct addrinfo members; socklen_t defined | TESTED | test-netdb_h.c:71 |
| NH7 | addrinfo has no floating-point members | Verified by reading | netdb.h |
| NH8 | AI_PASSIVE ... AI_ADDRCONFIG bitwise distinct | TESTED | test-netdb_h.c:81-91 (OR equals sum, each non-zero) |
| NH9 | NI_* flags, including NI_NUMERICSCOPE, bitwise distinct | TESTED | test-netdb_h.c:93-107 (NI_NUMERICSCOPE picolibc only: glibc doesn't define it) |
| NH10 | EAI_* usable in #if, distinct | TESTED | test-netdb_h.c:111-135 (#if, and a switch on every value) |
| NH11 | The 24 function prototypes | TESTED | test-netdb_h.c:138-162 (function-pointer initialisation) |

### Header <arpa/inet.h>

| # | Requirement | Status | Reference |
|---|---|---|---|
| AH1 | uint32_t and uint16_t defined | TESTED | test-arpa-inet/test-arpa_inet_h.c:46 (only `<arpa/inet.h>` included). **Was a deviation; fixed** |
| AH2 | htonl, htons, ntohl, ntohs as functions or macros with the specified types | TESTED | test-arpa_inet_h.c:47-53 |
| AH3 | in_port_t, in_addr_t, socklen_t, struct in_addr | TESTED | test-arpa_inet_h.c:66 |
| AH4 | INET_ADDRSTRLEN, INET6_ADDRSTRLEN | TESTED | test-arpa_inet_h.c:69 |
| AH5 | inet_addr, inet_ntoa, inet_ntop, inet_pton prototypes | TESTED | test-arpa_inet_h.c:74-78 |

### Header <net/if.h>

| # | Requirement | Status | Reference |
|---|---|---|---|
| IH1 | struct if_nameindex: unsigned if_index, char *if_name | TESTED | test-net-if/test-net_if_h.c:50 |
| IH2 | IF_NAMESIZE | TESTED | test-net_if_h.c:53-56 |
| IH3 | The four prototypes | TESTED | test-net_if_h.c:58-61 |

### Header <netinet/in.h>

| # | Requirement | Status | Reference |
|---|---|---|---|
| NI1 | in_port_t equivalent to uint16_t, in_addr_t to uint32_t | TESTED | test-netinet-in/test-netinet_in_h.c:57-58 |
| NI2 | sa_family_t, uint8_t, uint32_t defined | TESTED | test-netinet_in_h.c:58, 68 |
| NI3 | struct in_addr, sockaddr_in members | TESTED | test-netinet_in_h.c:67-70 |
| NI4 | struct in6_addr with uint8_t s6_addr[16]; sockaddr_in6 members | TESTED | test-netinet_in_h.c:71-76 |
| NI5 | struct ipv6_mreq | TESTED | test-netinet_in_h.c:77 |
| NI6 | in6addr_any, in6addr_loopback declared const, with the right values | TESTED | test-netinet_in_h.c:82, test-in6addr_any.c |
| NI7 | IN6ADDR_ANY_INIT, IN6ADDR_LOOPBACK_INIT constant at compile time | TESTED | test-netinet_in_h.c:85-86 (static initialisers), 214 |
| NI8 | IPPROTO_IP, IPV6, ICMP, RAW, TCP, UDP | TESTED | test-netinet_in_h.c:89 |
| NI9 | INADDR_ANY, INADDR_BROADCAST | TESTED | test-netinet_in_h.c:93-96 |
| NI10 | INET_ADDRSTRLEN 16, INET6_ADDRSTRLEN 46 | TESTED | test-netinet_in_h.c:97 |
| NI11 | The seven IPV6_* options, distinct | TESTED | test-netinet_in_h.c:102-117 (switch) |
| NI12 | The twelve IN6_IS_ADDR_* macros are int | TESTED | test-netinet_in_h.c:181 (in main: glibc's macros are statement expressions) |
| NI13 | IN6_IS_ADDR_* classify correctly | TESTED | test-netinet_in_h.c:137-160, 189-210 (20 addresses, with boundaries such as fe40::, febf::, feff::, ff12::, ::ffff:0:0, ::2) |
| NI14 | The byte order functions are available | TESTED | test-netinet_in_h.c:218 |
| NI15 | Functions returning sockaddr_in/in6 zero what isn't set | TESTED | test-netdb/test-getaddrinfo.c:231, 244 |
| NI16 | bind() with port 0 picks a port | Out of scope | net-sockets unit |

### getaddrinfo / freeaddrinfo

| # | Requirement | Status | Reference |
|---|---|---|---|
| GA1 | Translates node and service into socket addresses | TESTED | test-netdb/test-getaddrinfo.c:140-146 |
| GA2 | Thread-safe | UNTESTED | read: no static state; each call reads its own copy of the databases |
| GA3 | Neither node nor service: EAI_NONAME | TESTED | test-getaddrinfo.c:328 |
| GA4 | AF_UNSPEC tries every family | TESTED | test-getaddrinfo.c:160, 285 |
| GA5 | IPv4 dot notation (the inet_pton form) for AF_INET / AF_UNSPEC | TESTED | test-getaddrinfo.c:140 |
| GA6 | IPv6 text forms for AF_INET6 / AF_UNSPEC | TESTED | test-getaddrinfo.c:180, 285 |
| GA7 | Host names | TESTED | test-getaddrinfo.c:171 (localhost) |
| GA8 | Null service: network-level addresses | TESTED | test-getaddrinfo.c:171, 209 |
| GA9 | Decimal port strings | TESTED | test-getaddrinfo.c:140 |
| GA10 | Service names | TESTED | test-getaddrinfo.c:151-153 |
| GA11 | Null hints act as zero flags/type/protocol and AF_UNSPEC | TESTED | test-getaddrinfo.c:189-192 |
| GA12 | Only the seven AI_ flags (EAI_BADFLAGS) | TESTED | test-getaddrinfo.c:344 |
| GA13 | AI_PASSIVE, null node: INADDR_ANY / IN6ADDR_ANY_INIT | TESTED | test-getaddrinfo.c:165, 236 |
| GA14 | No AI_PASSIVE, null node: the loopback address | TESTED | test-getaddrinfo.c:231, 244 |
| GA15 | AI_PASSIVE ignored with a node | TESTED | test-getaddrinfo.c:218-220 |
| GA16 | AI_CANONNAME: canonical name in the first entry | TESTED | test-getaddrinfo.c:172 |
| GA17 | No canonical name (numeric host): the node string | TESTED | test-getaddrinfo.c:251 |
| GA18 | AI_NUMERICHOST: non-numeric node is EAI_NONAME | TESTED | test-getaddrinfo.c:330 |
| GA19 | AI_NUMERICSERV: non-numeric service is EAI_NONAME | TESTED | test-getaddrinfo.c:332, 290 |
| GA20 | AF_INET6 returns only IPv6 by default | TESTED | test-getaddrinfo.c:267-270 (EAI_NONAME picolibc only; glibc returns its own EAI_ADDRFAMILY) |
| GA21 | AI_V4MAPPED with AF_INET6: mapped addresses when there's no IPv6 | TESTED | test-getaddrinfo.c:183-185 |
| GA22 | AI_V4MAPPED ignored unless AF_INET6 | TESTED | test-getaddrinfo.c:258, 262 |
| GA23 | AI_ALL with AI_V4MAPPED: IPv6 and mapped IPv4 | TESTED | test-getaddrinfo.c:275-279 (numeric only; a name with both families needs a hosts file the test controls; read: the `have6` logic handles it) |
| GA24 | AI_ALL without AI_V4MAPPED ignored | TESTED | test-getaddrinfo.c:273 |
| GA25 | AI_ADDRCONFIG: families only when configured | UNTESTED | **Deviation, not fixed**: accepted and ignored (see below) |
| GA26 | A socket type limits the results; 0 gives every type | TESTED | test-getaddrinfo.c:152, 200 |
| GA27 | A protocol limits the results to it | TESTED | test-getaddrinfo.c:205, 209. **Was a deviation; fixed** |
| GA28 | At least one entry; ai_next null-terminated | TESTED | test-getaddrinfo.c:142, every call |
| GA29 | ai_family/socktype/protocol work with socket(), ai_addr/ai_addrlen with bind() | TESTED | test-getaddrinfo.c:297-303 |
| GA30 | Socket address fields not set explicitly are zero | TESTED | test-getaddrinfo.c:231, 244 |
| GA31 | EAI_NONAME for a name that doesn't resolve | TESTED | test-getaddrinfo.c:317-324 (`no-such-host.invalid`; glibc may also give EAI_AGAIN or EAI_FAIL) |
| GA32 | EAI_FAMILY, EAI_SOCKTYPE, EAI_SERVICE | TESTED | test-getaddrinfo.c:213, 335, 338, 341 |
| GA33 | EAI_AGAIN, EAI_FAIL, EAI_SYSTEM | NOT TESTABLE | picolibc has no resolver or system call that fails this way |
| GA34 | EAI_MEMORY | NOT TESTABLE | needs malloc failure injection |
| FA1 | freeaddrinfo frees the list | TESTED | every getaddrinfo call |
| FA2 | freeaddrinfo frees arbitrary sublists | TESTED | test-getaddrinfo.c:306-313 (no crash; leaks not checked) |
| FA3 | freeaddrinfo doesn't change errno | TESTED | test-getaddrinfo.c:314 |

### getnameinfo

| # | Requirement | Status | Reference |
|---|---|---|---|
| GN1 | Translates a socket address to host and service names | TESTED | test-netdb/test-getnameinfo.c:161-173 |
| GN2 | IPv4-mapped/compatible addresses named for the IPv4 address | TESTED | test-getnameinfo.c:303-307. **Was a deviation; fixed** |
| GN3 | The unspecified address isn't looked up | TESTED | test-getnameinfo.c:313-314. **Deviation by reading; fixed** (the test can't fail here: /etc/hosts doesn't list `::`) |
| GN4 | Null host or zero hostlen: no host name | TESTED | test-getnameinfo.c:166, 229 |
| GN5 | No name: the numeric address | TESTED | test-getnameinfo.c:314 |
| GN6 | Null service or zero servlen: no service | TESTED | test-getnameinfo.c:171 |
| GN7 | No service name: the port number | TESTED | test-getnameinfo.c:265-270 (port 0, compared with getservbyport) |
| GN8 | FQDN by default | NOT TESTABLE | depends on the hosts database |
| GN9 | NI_NOFQDN | TESTED | test-getnameinfo.c:282-288 (whole name or the part before a dot) |
| GN10 | NI_NUMERICHOST | TESTED | test-getnameinfo.c:161, 190, 310 |
| GN11 | NI_NAMEREQD: error when there's no name | TESTED | test-getnameinfo.c:182 (any resolver), 203 (picolibc only, after a gethostent scan), 313 |
| GN12 | NI_NUMERICSERV | TESTED | test-getnameinfo.c:161, 263 |
| GN13 | NI_NUMERICSCOPE: numeric scope; otherwise its name | TESTED | test-getnameinfo.c:329-346 (the NI_NUMERICSCOPE check is picolibc only: glibc doesn't define the flag). **Was a deviation; fixed** |
| GN14 | NI_NUMERICSCOPE ignored for IPv4 | TESTED (picolibc only) | test-getnameinfo.c:272-278 |
| GN15 | NI_DGRAM; stream by default | TESTED | test-getnameinfo.c:291-300 (port 514, compared with getservbyport) |
| GN16 | Thread-safe | UNTESTED | read: no static state |
| GN17 | Names are null-terminated | TESTED | every strcmp |
| GN18 | EAI_BADFLAGS | TESTED | test-getnameinfo.c:210-212. **Was a deviation; fixed** |
| GN19 | EAI_FAMILY for an unknown family or a short length | TESTED | test-getnameinfo.c:196, 215, 218 |
| GN20 | EAI_NONAME when neither name is asked for | TESTED (picolibc only) | test-getnameinfo.c:222 (glibc returns 0) |
| GN21 | EAI_OVERFLOW when a buffer is too small | TESTED | test-getnameinfo.c:183, 242-246, 254-263. **Was a deviation; fixed** for names that were found |
| GN22 | EAI_AGAIN, EAI_FAIL, EAI_MEMORY, EAI_SYSTEM | NOT TESTABLE | no resolver or allocation |

### gai_strerror

| # | Requirement | Status | Reference |
|---|---|---|---|
| GS1 | A string for each EAI_ value | TESTED | test-netdb/test-gai_strerror.c:119-121 (non-empty, distinct) |
| GS2 | An "unknown error" string otherwise | TESTED | test-gai_strerror.c:128-130 |

### Host, network, protocol and service databases

| # | Requirement | Status | Reference |
|---|---|---|---|
| HE1 | sethostent opens and rewinds | TESTED | test-netdb/test-gethostent.c:177 |
| HE2 | stayopen keeps it open; FD_CLOEXEC | TESTED | test-gethostent.c:130, 172 (file open: picolibc only) |
| HE3 | gethostent reads the next entry | TESTED | test-gethostent.c:146-156 |
| HE4 | Addresses in network byte order | TESTED | test-gethostent.c:195 (localhost is in 127/8) |
| HE5 | endhostent closes, releasing the descriptor | TESTED | test-gethostent.c:180-182 |
| NE1 | setnetent rewinds; stayopen; FD_CLOEXEC | TESTED | test-netdb/test-getnetbyname.c:130, 187 |
| NE2 | getnetent | TESTED | test-getnetbyname.c:147 |
| NE3 | getnetbyaddr matches type and number | TESTED | test-getnetbyname.c:152, 169 |
| NE4 | getnetbyname; null when missing | TESTED | test-getnetbyname.c:150, 156 |
| NE5 | n_net in host byte order | TESTED | test-getnetbyname.c:159-161 |
| NE6 | endnetent closes | TESTED | test-getnetbyname.c:189-191 |
| PE1 | setprotoent rewinds; FD_CLOEXEC | TESTED | test-netdb/test-getprotobyname.c:130, 175 |
| PE2 | getprotobyname, getprotobynumber, getprotoent | TESTED | test-getprotobyname.c:142-157 |
| PE3 | endprotoent closes | TESTED | test-getprotobyname.c:177-179 |
| SE1 | setservent rewinds; FD_CLOEXEC | TESTED | test-netdb/test-getservbyname.c:130, 181 |
| SE2 | getservbyname: name and protocol; null protocol matches any | TESTED | test-getservbyname.c:143-145, 157, 163 |
| SE3 | getservbyport: network-order port; null protocol matches any | TESTED | test-getservbyname.c:147, 159-161 |
| SE4 | s_port in network byte order; getservent | TESTED | test-getservbyname.c:143, 153 |
| SE5 | endservent closes | TESTED | test-getservbyname.c:183-185 |
| DB1 | Null at the end or when not found | TESTED | every loop; the "missing" checks |
| DB2 | EMFILE / ENFILE (may fail) | NOT TESTABLE | needs exhausting descriptors; optional |

### Byte order, inet_addr, inet_ntoa

| # | Requirement | Status | Reference |
|---|---|---|---|
| BO1 | htonl, htons, ntohl, ntohs | TESTED | test-arpa-inet/test-htonl.c:118-122 |
| IA1 | inet_addr a.b.c.d | TESTED | test-arpa-inet/test-inet_addr.c:110 |
| IA2 | a.b.c: last part 16 bits | TESTED | test-inet_addr.c:111, 125, 128 |
| IA3 | a.b: last part 24 bits | TESTED | test-inet_addr.c:112, 124, 127 |
| IA4 | a: stored directly | TESTED | test-inet_addr.c:113, 126, 129 |
| IA5 | Parts decimal, octal or hexadecimal | TESTED | test-inet_addr.c:114, 117, 122-123, 133 |
| IA6 | Network byte order | TESTED | test-inet_addr.c:135-141 |
| IA7 | Invalid: (in_addr_t)(-1) | TESTED | test-inet_addr.c:116-119, 130-132 |
| IA8 | inet_ntoa: dot notation of a network-order address | TESTED | test-arpa-inet/test-inet_ntoa.c:113-128 |

### inet_ntop / inet_pton

| # | Requirement | Status | Reference |
|---|---|---|---|
| IP1 | inet_ntop AF_INET | TESTED | test-arpa-inet/test-inet_ntop.c:125, 164 |
| IP2 | inet_ntop AF_INET6 | TESTED | test-inet_ntop.c:127-134, 165-166 |
| IP3 | INET_ADDRSTRLEN / INET6_ADDRSTRLEN are enough | TESTED | test-inet_ntop.c:141, 150 (the longest forms) |
| IP4 | Returns dst; ENOSPC when too small | TESTED | test-inet_ntop.c:126, 143, 152, 155-157 |
| IP5 | EAFNOSUPPORT | TESTED | test-inet_ntop.c:135 |
| IP6 | inet_pton AF_INET: four parts of one to three digits, 0-255 | TESTED | test-arpa-inet/test-inet_pton.c:130-147 |
| IP7 | Leading zeros allowed | TESTED (picolibc only) | test-inet_pton.c:136-141 (glibc rejects them). **Was a deviation; fixed** |
| IP8 | No other IPv4 forms | TESTED | test-inet_pton.c:133, 145 |
| IP9 | IPv6 preferred form, one to four hex digits | TESTED | test-inet_pton.c:156, 161, 169, 174 |
| IP10 | `::` once; unspecified as `::` | TESTED | test-inet_pton.c:153, 160, 168, 171 |
| IP11 | x:x:x:x:x:x:d.d.d.d | TESTED | test-inet_pton.c:157, 166-167, 170, 172 |
| IP12 | 1 with network-order result; 0 for invalid input | TESTED | every check; 149-151 (destination untouched on failure) |
| IP13 | -1 and EAFNOSUPPORT | TESTED | test-inet_pton.c:176-177 |

### if_nametoindex, if_indextoname, if_nameindex, if_freenameindex

| # | Requirement | Status | Reference |
|---|---|---|---|
| IF1 | if_nametoindex returns the index | TESTED | test-net-if/test-if_nameindex.c:118 |
| IF2 | ... or zero for a non-interface | TESTED | test-if_nameindex.c:129, 131-132 |
| IF3 | if_indextoname fills ifname and returns it | TESTED | test-if_nameindex.c:120 |
| IF4 | ENXIO | TESTED | test-if_nameindex.c:130, 134 (index 0 and 0x7fffffff) |
| IF5 | Array ends with index 0, name NULL | TESTED | test-if_nameindex.c:116, 122 |
| IF6 | ENOBUFS (may fail) | NOT TESTABLE | needs an allocation failure |
| IF7 | if_freenameindex doesn't change errno | TESTED | test-if_nameindex.c:125-127 |

### Counts

146 requirements:
- TESTED: 135 (8 of them only under `__PICOLIBC__` or `#ifdef NI_NUMERICSCOPE`
  because glibc differs: NH9 in part, GA20 in part, GN11 in part, GN13 in
  part, GN14, GN20, HE2 in part, IP7)
- UNTESTED: 3 (GA2, GA25, GN16)
- NOT TESTABLE: 6 (GA33, GA34, GN8, GN22, DB2, IF6)
- Verified by reading: 1 (NH7); out of scope: 1 (NI16)

Tested before the audit: about 63 of the 146 (counted against the
pre-existing tests; no header tests existed, and the existing inet_pton
test asserted the leading-zero deviation).

## Deviations found and fixed

All but the `::` part of 3 were verified by running: each test fails with
only the library change reverted (failing line quoted) and passes with it.

1. **inet_pton rejected leading zeros** (libc/posix/arpa-inet/inet_pton.c
   `__inet_pton4`). POSIX.1-2024: "ddd is a one to three digit decimal number
   between 0 and 255. Leading zeros shall be allowed." The existing test
   asserted rejection of "1.2.3.04". Fix: up to three digits per part. Also
   applies to the dotted tail of IPv6 text and to numeric hosts in
   getaddrinfo. Failing before:
   `leading zeros: inet_pton(AF_INET, "001.002.003.010", &in) == 1 && ...`.
   glibc rejects leading zeros, hence `#ifdef __PICOLIBC__`.
2. **getnameinfo ignored unknown flags** (getnameinfo.c). POSIX:
   "[EAI_BADFLAGS] The flags had an invalid value." Fix: reject bits beyond
   the six NI_ flags (glibc does too). Failing before:
   `EAI_BADFLAGS: getnameinfo(..., 0x40000000) == EAI_BADFLAGS`.
3. **getnameinfo didn't name mapped/compatible IPv6 addresses by their IPv4
   address, and looked up `::`** (getnameinfo.c). POSIX: "the implementation
   shall extract the embedded IPv4 address and lookup the node name for that
   IPv4 address. If the address is the IPv6 unspecified address ("::"), a
   lookup shall not be performed". Fix: look up the last four bytes as
   AF_INET for IN6_IS_ADDR_V4MAPPED/V4COMPAT, skip the lookup for `::`; the
   numeric form still shows the IPv6 address. Failing before:
   `IPv4-mapped: name6("::ffff:127.0.0.1", 0, host, sizeof(host), NI_NAMEREQD) == 0 && ...`.
   The `::` part is by reading only (its test passes either way here).
4. **getnameinfo replaced a found name that didn't fit with the number**
   (getnameinfo.c). Too-long host or service names fell back to the numeric
   form, or to EAI_NONAME with NI_NAMEREQD. POSIX: "[EAI_OVERFLOW] ... The
   buffer pointed to by the node argument or the service argument was too
   small." Fix: look names up into a local buffer (database lines are at
   most 512 bytes), apply NI_NOFQDN, then EAI_OVERFLOW if it doesn't fit.
   Failing before:
   `host name overflow: getnameinfo(..., host, (socklen_t)len, NULL, 0, NI_NAMEREQD) == EAI_OVERFLOW`.
5. **getnameinfo dropped the IPv6 scope** (getnameinfo.c). POSIX: "If the
   flag bit NI_NUMERICSCOPE is set, the numeric form of the scope identifier
   shall be returned (for example, interface index) instead of its name."
   Fix: for a non-zero sin6_scope_id, append `%` and the interface name
   (link-local and multicast link-local, via if_indextoname) or the number
   (otherwise, or with NI_NUMERICSCOPE), as glibc does. Failing before:
   `link-local scope: name6("fe80::1", index, host, sizeof(host), NI_NUMERICHOST) == 0 && strncmp(host, "fe80::1%", 8) == 0 && ...`.
   getnameinfo (libc/posix/netdb) now calls if_indextoname (libos/linux);
   Linux is the only network backend, but a new one would need it.
6. **getaddrinfo gave raw entries protocol 0** (getaddrinfo.c
   `__gai_entry`). With hints `ai_protocol = IPPROTO_ICMP` the result was a
   SOCK_RAW entry with ai_protocol 0, a different protocol, which socket()
   rejects on Linux. POSIX: "If ai_family is not AF_UNSPEC and ai_protocol
   is not zero, then addresses shall be returned for use only with the
   specified address family and protocol". Fix: raw entries take the hints'
   protocol (as glibc). Failing before:
   `ICMP only: getaddrinfo("127.0.0.1", NULL, &hints, &res) == 0 && all(res, 0, IPPROTO_ICMP)`.
7. **<arpa/inet.h> didn't define uint16_t** (and without networking not
   uint32_t either) (libc/include/arpa/inet.h). POSIX: "The <arpa/inet.h>
   header shall define the uint32_t and uint16_t types". Fix: define both
   with the usual `_UINTn_T_DECLARED` guards. Failing before:
   test-arpa_inet_h.c doesn't compile (`unknown type name 'uint16_t'`).

## Deviation not fixed

- **AI_ADDRCONFIG is ignored** (getaddrinfo.c). POSIX: "IPv4 addresses
  shall be returned only if an IPv4 address is configured on the local
  system, and IPv6 addresses shall be returned only if an IPv6 address is
  configured". Both families are configured on this host, so no test can
  show the difference and a fix couldn't be proved here. A fix would check
  each family (musl connects a UDP socket to each loopback address; reading
  the interface addresses is another way).

## Other observations (not changed)

- `IPPORT_RESERVED` is 1024, as in glibc and the BSDs; POSIX's wording
  ("the highest reserved Internet port number") reads as 1023.
- `getaddrinfo(NULL, service, AI_CANONNAME)` returns EAI_BADFLAGS, as glibc;
  POSIX only defines AI_CANONNAME with a node.
- With hints `ai_protocol = IPPROTO_TCP`, socket type 0 and no service,
  getaddrinfo returns a SOCK_RAW/TCP entry besides SOCK_STREAM; glibc
  returns only the stream entry. Both usable; not asserted.
- getaddrinfo takes only the inet_pton numeric forms, as Issue 8 says, so
  "127.1" is a host name to it (glibc accepts inet_addr forms); "010.1.1.1"
  is 10.1.1.1 in picolibc and 8.1.1.1 in glibc.
- NI_NOFQDN cuts every name at its first dot; all picolibc names come from
  /etc/hosts, so all are "local hosts".
- getnameinfo with neither host nor service buffer: picolibc EAI_NONAME as
  POSIX says, glibc 0.
- glibc doesn't define NI_NUMERICSCOPE.
- getnetbyname and getservbyname also match aliases, as glibc does.
- if_nameindex returns NULL with opendir's errno, not ENOBUFS, when
  /sys/class/net can't be read (ENOBUFS is "may fail").
- The NI_NAMEREQD check in test-getnameinfo.c already used 192.0.2.1
  (TEST-NET-1) on this base and accepted EAI_NONAME or any real name, so it
  holds whatever the resolver says; `test-getnameinfo-native` passes here.
  I added a strict picolibc-only check: EAI_NONAME, when a `gethostent()`
  scan shows /etc/hosts doesn't list the address (picolibc never uses DNS).

## Left undone

- GA25 (AI_ADDRCONFIG), above.
- Thread safety of getaddrinfo/getnameinfo (GA2, GN16): by reading only.
- GA23 with a host name having both IPv4 and IPv6 entries.
- GN3 (`::` not looked up) can't fail on this host.

## Test runs (verified by running)

- aarch64-linux-gnu (`builds/build-aarch64-linux-gnu`, -Dwerror=true), full
  `meson test -t 4`: 1179 Ok, 6 Expected Fail, 0 Fail, 41 Skipped. The four
  touched suites: 36 Ok (18 tests x picolibc and -native).
- arm-linux-gnueabihf (`builds/build-arm-linux-gnueabihf`, -Dwerror=true),
  full `meson test -t 4`: 1176 Ok, 6 Expected Fail, 0 Fail, 43 Skipped.
  Touched suites: 36 Ok.
- Bare-metal arm (`scripts/do-arm-configure -Dwerror=true -Dmultilib=false`;
  the full multilib build is 210k steps, too long on a shared host), touched
  suites with `meson test -t 20`: 18 Skipped (no networking), 0 Fail; the
  library and every new test build there.
- CMake thumbv7m (`scripts/do-cmake-thumbv7m-configure`, `ninja test`): 480
  tests, 100% passed (229 run, 251 skipped, including all net tests, which
  skip without networking).
- Each fix's test was run with only that library change reverted, and
  failed as quoted above.
- `clang-format --dry-run` clean on every touched file.

## Commits (audit/net-names, on top of 2d4e737cfc)

- 55ee833da2 libc/arpa-inet: Accept leading zeros in inet_pton IPv4 parts
- 68adcf46dc test-posix: Check the IPv4 and IPv6 forms in inet_pton
- 859ff3aeda libc/netdb: Reject unknown flags in getnameinfo
- 801c714f79 libc/netdb: Name mapped IPv6 addresses by their IPv4 address in getnameinfo
- 8763a75ac7 libc/netdb: Return EAI_OVERFLOW when a name doesn't fit in getnameinfo
- c98412914b libc/netdb: Add the IPv6 scope identifier to numeric names in getnameinfo
- 6ba6eb9d6a libc/include: Define uint16_t and uint32_t in <arpa/inet.h>
- fb1bbcb394 test-posix: Check flags, overflow, mapped addresses and scopes in getnameinfo
- aa3fb935b4 libc/netdb: Give raw getaddrinfo results the protocol from the hints
- e7b1b41b42 test-posix: Check hints, flags, results and freeing in getaddrinfo
- eeb583b71a test-posix: Check unknown errors in gai_strerror
- fbe92da5bd test-posix: Check rewinding, descriptors and byte order in gethostent
- f14a97386e test-posix: Check byte order, types and rewinding in getnetbyname
- 80345e5ea0 test-posix: Check lookups and rewinding in getprotobyname
- 20f68e1a10 test-posix: Check protocol matching and rewinding in getservbyname
- 1b10e06c82 test-posix: Check the buffer sizes, forms and byte order in inet_ntop, inet_addr and inet_ntoa
- e90d40ca7c test-posix: Check the terminator, errno and bad names in if_nameindex
- c0af74bcc7 test-posix: Check the types, functions and macros in arpa/inet.h
- a4db728a52 test-posix: Check the structure, constant and prototypes in net/if.h
- 3e5755ead1 test-posix: Check the structures, flags, errors and prototypes in netdb.h
- f74f3c9914 test-posix: Check the types, constants and address macros in netinet/in.h

Nothing pushed. No AI attribution in any commit.
