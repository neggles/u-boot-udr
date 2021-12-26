/*      $OpenBSD: htonl.c,v 1.6 2005/08/06 20:30:03 espie Exp $ */
/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>

#define SWAP16(x) ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8)
#define SWAP32(x) ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8) | ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24) 

unsigned short
htobe16(unsigned short x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return SWAP16(x);
#else
    return x;
#endif
}

unsigned short
htole16(unsigned short x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return x;
#else
    return SWAP16(x);
#endif
}

unsigned short
be16toh(unsigned short x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return SWAP16(x);
#else
    return x;
#endif
}

unsigned short
le16toh(unsigned short x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return x;
#else
    return SWAP16(x);
#endif
}

unsigned int
htobe32(unsigned int x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return SWAP32(x);
#else
    return x;
#endif
}

unsigned int
htole32(unsigned int x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return x;
#else
    return SWAP32(x);
#endif
}

unsigned int
be32toh(unsigned int x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return SWAP32(x)
#else
    return x;
#endif
}

unsigned int
le32toh(unsigned int x)
{
#if defined(LITTLE_ENDIAN_TARGET)
    return x;
#else
    return SWAP32(x);
#endif
}

unsigned int
ntohl(unsigned int x)
{
    return be32toh(x);
}

unsigned int
htonl(unsigned int x)
{
    return htobe32(x);
}

unsigned short int
ntohs(unsigned short int x)
{
    return be16toh(x);
}

unsigned short int
htons(unsigned short int x)
{
    return htobe16(x);
}

int
atoi_hex(const char *str)
{
    return((int)strtoul(str, (char **)NULL, 16));
}

char *
itoa(int val, int base)

{
    char *ptr;
    static char buffer[16]; /* result is built here */
                    /* 16 is sufficient since the largest number
                   we will ever convert will be 2^32-1,
                   which is 10 digits. */

    printf("%x", val);
    ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    if (val == 0)
    {
        *--ptr = '0';
    }
    else while (val != 0)
    {
        *--ptr = (val % base) + '0';
        val /= base;
    }

    return(ptr);
}
