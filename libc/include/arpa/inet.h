/* Copyright (c) 2004 Jeff Johnston  <jjohnstn@redhat.com> */
#ifndef __ARPA_INET_H__
#define __ARPA_INET_H__

#include <endian.h>
#include <sys/cdefs.h>

#ifdef __PICOLIBC_NETWORK
#include <netinet/in.h>
#endif

_BEGIN_STD_C

/* The byte order functions, also defined as macros below */
__uint32_t(htonl)(__uint32_t) __picolibc_export;
__uint16_t(htons)(__uint16_t) __picolibc_export;
__uint32_t(ntohl)(__uint32_t) __picolibc_export;
__uint16_t(ntohs)(__uint16_t) __picolibc_export;

#ifdef __PICOLIBC_NETWORK
in_addr_t   inet_addr(const char *__cp) __picolibc_export;
char       *inet_ntoa(struct in_addr __in) __picolibc_export;
const char *inet_ntop(int       __af, const void       *__restrict __src, char       *__restrict __dst,
                      socklen_t __size) __picolibc_export;
int inet_pton(int __af, const char * __restrict __src, void * __restrict __dst) __picolibc_export;
#endif

_END_STD_C

/* byteorder(3) - simimlar to linux <arpa/inet.h> */
#ifndef __machine_host_to_from_network_defined
#if _BYTE_ORDER == _LITTLE_ENDIAN
#define __htonl(_x) __bswap32(_x)
#define __htons(_x) __bswap16(_x)
#define __ntohl(_x) __bswap32(_x)
#define __ntohs(_x) __bswap16(_x)
#define htonl(_x)   __htonl(_x)
#define htons(_x)   __htons(_x)
#define ntohl(_x)   __htonl(_x)
#define ntohs(_x)   __htons(_x)
#else
#define __htonl(_x) ((__uint32_t)(_x))
#define __htons(_x) ((__uint16_t)(_x))
#define __ntohl(_x) ((__uint32_t)(_x))
#define __ntohs(_x) ((__uint16_t)(_x))
#define htonl(_x)   __htonl(_x)
#define htons(_x)   __htons(_x)
#define ntohl(_x)   __ntohl(_x)
#define ntohs(_x)   __ntohs(_x)
#endif
#endif /* __machine_host_to_from_network_defined */

#endif /* __ARPA_INET_H__ */
