/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright © 2025 Keith Packard
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

#include "iconv_private.h"
#include <stdbool.h>

/* Write the bytes that return the output to its initial shift state */
static size_t
iconv_reset(iconv_t ic, char **outbuf, size_t *outbytesleft)
{
    char      nul[MB_LEN_MAX], seq[MB_LEN_MAX];
    mbstate_t initial;
    int       nul_len, seq_len;
    size_t    reset;

    if (outbuf && *outbuf && outbytesleft && *outbytesleft > 0) {
        /* The reset sequence is what precedes a null character */
        memset(&initial, 0, sizeof(initial));
        nul_len = ic->out_wctomb(nul, L'\0', &initial);
        seq_len = ic->out_wctomb(seq, L'\0', &ic->out_state);
        if (nul_len > 0 && seq_len > nul_len) {
            reset = (size_t)(seq_len - nul_len);
            if (reset > *outbytesleft) {
                errno = E2BIG;
                return (size_t)-1;
            }
            memcpy(*outbuf, seq, reset);
            *outbuf += reset;
            *outbytesleft -= reset;
        }
    }
    memset(&ic->in_state, 0, sizeof(ic->in_state));
    memset(&ic->out_state, 0, sizeof(ic->out_state));
    return 0;
}

size_t
iconv(iconv_t ic, char ** __restrict inbuf, size_t * __restrict inbytesleft,
      char ** __restrict outbuf, size_t * __restrict outbytesleft)
{
    char  *in, *out;
    size_t inbytes, outbytes;
    size_t inexact_count = 0;
    int    err = 0;

    if (ic == (iconv_t)-1) {
        errno = EINVAL;
        return (size_t)-1;
    }

    /* No input: return to the initial shift state */
    if (!inbuf || !*inbuf)
        return iconv_reset(ic, outbuf, outbytesleft);

    in = *inbuf;
    out = *outbuf;
    inbytes = *inbytesleft;
    outbytes = *outbytesleft;

    /*
     * Convert one character at a time, changing nothing until its
     * output fits, so that the pointers and states always stop just
     * after the last character converted
     */
    while (inbytes) {
        mbstate_t in_state = ic->in_state;
        mbstate_t out_state = ic->out_state;
        char      mb[MB_LEN_MAX];
        wchar_t   wc;
        int       in_len, out_len;
        bool      inexact = false;

        in_len = ic->in_mbtowc(&wc, in, inbytes, &in_state);
        if (in_len == -2) {
            /* Incomplete character at the end of the input */
            err = EINVAL;
            break;
        }
        if (in_len == -1) {
            if (ic->mode == iconv_ignore) {
                /* Discard the invalid byte */
                in++;
                inbytes--;
                continue;
            }
            err = EILSEQ;
            break;
        }
        if (in_len == 0) {
            /* A null character: count any shift bytes before it */
            char *nul = memchr(in, '\0', inbytes);
            in_len = nul ? (int)(nul - in) + 1 : 1;
        }

        out_len = ic->out_wctomb(mb, wc, &out_state);
        if (out_len == -1) {
            /* No identical character in the output codeset */
            inexact = true;
            out_state = ic->out_state;
            if (ic->mode == iconv_ignore || ic->mode == iconv_discard)
                out_len = 0;
            else
                out_len = ic->out_wctomb(mb, L'?', &out_state);
            if (out_len == -1) {
                err = EILSEQ;
                break;
            }
        }

        if ((size_t)out_len > outbytes) {
            err = E2BIG;
            break;
        }

        memcpy(out, mb, (size_t)out_len);
        out += out_len;
        outbytes -= out_len;
        in += in_len;
        inbytes -= in_len;
        ic->in_state = in_state;
        ic->out_state = out_state;
        if (inexact)
            inexact_count++;
    }

    *inbuf = in;
    *inbytesleft = inbytes;
    *outbuf = out;
    *outbytesleft = outbytes;
    if (err) {
        errno = err;
        return (size_t)-1;
    }
    return inexact_count;
}
