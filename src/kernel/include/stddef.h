#ifndef _STDDEF_H
#define _STDDEF_H

// Null crap
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif
#endif

// Standerd shit
#ifndef _PTRDIFF_T
#define _PTRDIFF_T
#ifdef __LP64__
typedef long ptrdiff_t;
#else
typedef int ptrdiff_t;
#endif
#endif

#ifndef _SIZE_T
#define _SIZE_T
#ifdef __LP64__
typedef unsigned long size_t;
#else
typedef unsigned int size_t;
#endif
#endif

// Offeset macro, dunnok if need
#define offsetof(type, member) __builtin_offsetof(type, member)

#endif
