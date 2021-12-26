// vim: ts=4:sw=4:expandtab

/*      $OpenBSD: strncmp.c,v 1.7 2005/08/08 08:05:37 espie Exp $       */

/*
 * Copyright (c) 1989 The Regents of the University of California.
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
#include <limits.h>
#include <errno.h>

int errno = 0;

int
strncmp(const char *s1, const char *s2, size_t n)
{

        if (n == 0)
                return (0);
        do {
                if (*s1 != *s2++)
                        return (*(unsigned char *)s1 - *(unsigned char *)--s2);
                if (*s1++ == 0)
                        break;
        } while (--n != 0);
        return (0);
}

#if 0
/*
 * Compare strings.
 */
int
strcmp(const char *s1, const char *s2)
{
        while (*s1 == *s2++)
                if (*s1++ == 0)
                        return (0);
        return (*(unsigned char *)s1 - *(unsigned char *)--s2);
}
#endif

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long
strtoul(const char *nptr, char **endptr, int base)
{
        const char *s;
        unsigned long acc, cutoff;
        int c;
        int neg, any, cutlim;

        /*
         * See strtol for comments as to the logic used.
         */
        s = nptr;
        do {
                c = (unsigned char) *s++;
        } while (isspace(c));
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;

        cutoff = ULONG_MAX / (unsigned long)base;
        cutlim = ULONG_MAX % (unsigned long)base;
        for (acc = 0, any = 0;; c = (unsigned char) *s++) {
                if (isdigit(c))
                        c -= '0';
                else if (isalpha(c))
                        c -= isupper(c) ? 'A' - 10 : 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0)
                        continue;
                if (acc > cutoff || (acc == cutoff && c > cutlim)) {
                        any = -1;
                        acc = ULONG_MAX;
                        errno = ERANGE;
                } else {
                        any = 1;
                        acc *= (unsigned long)base;
                        acc += c;
                }
        }
        if (neg && any > 0)
                acc = -acc;
        if (endptr != 0)
                *endptr = (char *) (any ? s - 1 : nptr);
        return (acc);
}

char *
strcat(char *s1, char*s2)
{
    char *os1;

    os1 = s1;
    while (*s1++);
    --s1;
    while (*s1++ = *s2++);

    return(os1);
}

char *
strcpy(char *to, const char *from)
{
        char *save = to;

        for (; (*to = *from) != '\0'; ++from, ++to);
        return(save);
}

size_t
strlen(const char *str)
{
        const char *s;

        for (s = str; *s; ++s)
                ;
        return (s - str);
}

size_t
strnlen(const char *str, size_t count)
{
    const char *s;

    for (s = str; count-- && *s != '\0'; ++s);

    return (s - str);
}

char *
strchr(const char *p, int ch)
{
    const char cmp = ch;

    for (;; ++p) {
        if (*p == cmp) {
            /* LINTED const cast-away */
            return((char *)p);
        }
        if (!*p)
            return((char *)NULL);
    }
    /* NOTREACHED */
}

char *
strtok_r(char *s, const char *delim, char **last)
{
        char *spanp;
        int c, sc;
        char *tok;

        if (s == NULL && (s = *last) == NULL)
                return (NULL);

        /*
         * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
         */
cont:
        c = *s++;
        for (spanp = (char *)delim; (sc = *spanp++) != 0;)
                if (c == sc)
                        goto cont;

        if (c == 0) {           /* no non-delimiter characters */
                *last = NULL;
                return (NULL);
        }
        tok = s - 1;

        /*
         * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
         * Note that delim must have one NUL; we stop if we see that, too.
         */
        for (;;) {
                c = *s++;
                spanp = (char *)delim;
                do {
                        if ((sc = *spanp++) == c) {
                                if (c == 0)
                                        s = NULL;
                                else {
                                        char *w = s - 1;
                                        *w = '\0';
                                }
                                *last = s;
                                return (tok);
                        }
                } while (sc != 0);
        }
        /* NOTREACHED */
}


char *
strtok(char *s, const char *delim)
{
        static char *last;

        return (strtok_r(s, delim, &last));
}

char *
strpbrk(const char *s1, const char *s2)
{
    const char *str1, *str2;

    for (str1 = s1; *str1 != '\0'; ++str1) {
        for(str2 = s2; *str2 != '\0'; ++str2) {
            if (*str1 == *str2) {
                return (char *) str1;
            }
        }
    }

    return NULL;
}

char *
strsep(char **s, const char *ct)
{
    char *sbegin = *s, *end;

    if (sbegin == NULL) {
        return NULL;
    }

    end = strpbrk(sbegin, ct);
    if (end) {
        *end++ = '\0';
    }
    *s = end;

    return sbegin;
}

char *
strdup(char *str)
{
    int len;
    char *copy;

    len = strlen(str) + 1;
    if (!(copy = malloc((unsigned int)len)))
       return((char *)NULL);
    memcpy(copy, str, len);
    return(copy);
}


int
toupper(int c) {

    if (c >= 0x61 && c <= 0x7a)
        c = c - 0x20;

    return c;
}
