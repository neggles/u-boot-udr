
    /* Copyright Ubiquiti Networks Inc.
    ** All Rights Reserved
    */


#ifndef TYPES_H_
#define TYPES_H_
//#include <ctype.h>

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int    size_t;
#endif

typedef unsigned int    u32;
typedef unsigned short  u16;
typedef unsigned char   u8;

typedef unsigned int    uint32_t;
typedef unsigned short  uint16_t;
typedef unsigned char   uint8_t;
typedef unsigned char   uchar;

typedef int             int32_t;
//typedef unsigned char   int8_t;

typedef unsigned long   ulong;
typedef unsigned short  ushort;

#ifndef NULL
#define NULL 0
#endif
#endif

/* KMLUOH: with -nostdinc */
#if 0
#ifndef _UBNT_CLOCK_T_
#define _UBNT_CLOCK_T_
typedef long clock_t;
#endif

#ifndef _UBNT_FILE_
#define _UBNT_FILE_
struct __STDIO_FILE_STRUCT {
	unsigned short __modeflags;
	/* There could be a hole here, but modeflags is used most.*/
#ifdef __UCLIBC_HAS_WCHAR__
	unsigned char __ungot_width[2]; /* 0: current (building) char; 1: scanf */
	/* Move the following futher down to avoid problems with getc/putc
	 * macros breaking shared apps when wchar config support is changed. */
	/* wchar_t ungot[2]; */
#else  /* __UCLIBC_HAS_WCHAR__ */
	unsigned char __ungot[2];
#endif /* __UCLIBC_HAS_WCHAR__ */
	int __filedes;
#ifdef __STDIO_BUFFERS
	unsigned char *__bufstart;	/* pointer to buffer */
	unsigned char *__bufend;	/* pointer to 1 past end of buffer */
	unsigned char *__bufpos;
	unsigned char *__bufread; /* pointer to 1 past last buffered read char */

#ifdef __STDIO_GETC_MACRO
	unsigned char *__bufgetc_u;	/* 1 past last readable by getc_unlocked */
#endif /* __STDIO_GETC_MACRO */
#ifdef __STDIO_PUTC_MACRO
	unsigned char *__bufputc_u;	/* 1 past last writeable by putc_unlocked */
#endif /* __STDIO_PUTC_MACRO */

#endif /* __STDIO_BUFFERS */

#ifdef __STDIO_HAS_OPENLIST
	struct __STDIO_FILE_STRUCT *__nextopen;
#endif
#ifdef __UCLIBC_HAS_GLIBC_CUSTOM_STREAMS__
	void *__cookie;
	_IO_cookie_io_functions_t __gcs;
#endif
#ifdef __UCLIBC_HAS_WCHAR__
	wchar_t __ungot[2];
#endif
#ifdef __STDIO_MBSTATE
	__mbstate_t __state;
#endif
#ifdef __UCLIBC_HAS_XLOCALE__
	void *__unused;				/* Placeholder for codeset binding. */
#endif
#ifdef __UCLIBC_HAS_THREADS__
	int __user_locking;
	__UCLIBC_IO_MUTEX(__lock);
#endif
/* Everything after this is unimplemented... and may be trashed. */
#if __STDIO_BUILTIN_BUF_SIZE > 0
	unsigned char __builtinbuf[__STDIO_BUILTIN_BUF_SIZE];
#endif /* __STDIO_BUILTIN_BUF_SIZE > 0 */
};

typedef struct __STDIO_FILE_STRUCT FILE;
#endif

#ifndef CHAR_BIT
#define CHAR_BIT    8
#endif

#ifndef EOF
#define EOF (-1)
#endif
#endif
