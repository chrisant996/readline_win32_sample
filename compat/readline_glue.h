#pragma once

//------------------------------------------------------------------------------
// HELPER FUNCTIONS FOR INTEGRATING WITH READLINE ON WINDOWS

// Call config_console() before using Readline (e.g. before calling
// readline()).  This saves the console mode and adjusts the console mode for
// reading one character at a time, so that Readline can read input.
extern int config_console(void);

// Call unconfig_console() after using Readline (e.g. after readline()
// returns).  This restores the console mode so that any other reading can
// still work properly.
extern void unconfig_console(void);

//------------------------------------------------------------------------------
// READLINE INTERNAL GLUE

#include <conio.h>
#include <io.h>
#include <limits.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <wchar.h>
#include <uchar.h>

#if !defined(__cplusplus) && !defined(_MSC_VER)
#define static_assert _Static_assert
#endif

int     compare_string(const char* s1, const char* s2, int casefold);

/*
// for purposes of utf-8 and capturing stdio
int     hooked_fwrite(const void*, int, int, FILE*);
void    hooked_fprintf(FILE*, const char*, ...);
int     hooked_putc(int, FILE*);
void    hooked_fflush(FILE*);
int     hooked_fileno(FILE*);
int     hooked_stat(const char*, struct hooked_stat*);
int     hooked_lstat(const char*, struct hooked_stat*);
int     hooked_fstat(int, struct hooked_stat*);
int     mk_wcwidth(char32_t);
int     mk_wcswidth(const char32_t *, size_t);
*/

#if defined(__MINGW32__)
#   undef fwrite
#   undef fprintf
#   undef putc
#   undef fflush
#   undef fileno
/*
#   undef mbrtowc
#   undef mbrlen
*/
#   undef stat
#   undef fstat
#   define __MSDOS__

// For consistency, make mingw compile Readline the same way msvc does.
#   if defined(BUILD_READLINE)
#       undef __MINGW32__
#   endif

/*
#   define RL_LIBRARY_VERSION "8.2"
*/
#endif // __MINGW32__

#if defined(BUILD_READLINE)
/*
#   define fwrite           hooked_fwrite
#   define fprintf          hooked_fprintf
#   define putc             hooked_putc
#   define fflush           hooked_fflush
#   define fileno           hooked_fileno
#   define stat             hooked_stat
#   define lstat            hooked_lstat
#   define fstat            hooked_fstat
*/
#   define S_ISLNK(mode)    (0)
#   define S_ISBLK(mode)    (0)
#endif // BUILD_READLINE

#undef MB_CUR_MAX
#define MB_CUR_MAX  4       // 4-bytes is enough for the Unicode standard.

// msvc vs posix|readline|gnu
//
#if defined(_MSC_VER)
#   define strncasecmp      strnicmp
#   define strcasecmp       _stricmp
#   define strchr           strchr
#   define getpid           _getpid
#   define snprintf         _snprintf

typedef ptrdiff_t           ssize_t;
typedef unsigned short      mode_t;

#   define __STDC__         0
/*
#   define __MINGW32__
#   define __WIN32__
*/

#   pragma warning(disable : 4018)  // signed/unsigned mismatch
#   pragma warning(disable : 4090)  // different 'const' qualifiers
#   pragma warning(disable : 4101)  // unreferenced local variable
#   pragma warning(disable : 4244)  // conversion from 'X' to 'Y', possible loss of data
#   pragma warning(disable : 4267)  // conversion from 'X' to 'Y', possible loss of data
#endif // _MSC_VER

#define HAVE_ISASCII 1
#define HAVE_STRCASECMP 1
#define HAVE_WCWIDTH 1
#define HAVE_DIRENT_H 1
#define WCHAR_T_BROKEN 1            // Visual Studio uses 2 byte wchar_t; Readline expects 4 byte wchar_t.

// Readline uses __MSDOS__ for both platform-specific file handling and
// terminal display, but with a sufficient implementation of the termcap
// library then it isn't needed.
#undef __MSDOS__

// Work around quirk in complete.c
#ifdef _WIN32
#   define __WIN32__
#endif

// Readline 8.2 added usage of posixtime.h and posixselect.h, and does not
// guard usage safely in Windows.  Work around the problem by including
// winsock.h; however, it slows down compilation time significantly.
#define HAVE_TIMEVAL 1
#if defined (READLINE_LIBRARY)
struct timeval {
    time_t tv_sec;
    long tv_usec;
};
#else
struct timeval; // Just a forward declaration to avoid collision with winsock.h.
#endif
#define HAVE_GETTIMEOFDAY 1
typedef int sigset_t;               // satisfy compilation.
extern int gettimeofday(struct timeval * tp, struct timezone * tzp);

typedef int wcwidth_t (char32_t);
typedef int wcswidth_t (const char32_t*, size_t);
extern wcwidth_t *wcwidth;
extern wcswidth_t *wcswidth;
