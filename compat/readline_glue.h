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

// For diagnostic purposes.  Pass <=0 to disable, pass 1 to enable verbose
// input diagnostic output, or pass >1 to print verbose diagnostic output at
// the top of the terminal display.
extern void set_verbose_input(int verbose);

//------------------------------------------------------------------------------
// READLINE INTERNAL GLUE

#include <io.h>
#include <limits.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
// #include <sys/stat.h>
// #include <wchar.h>
#include <uchar.h>

#if !defined(__cplusplus) && !defined(_MSC_VER)
#define static_assert _Static_assert
#endif

/*
int     hooked_stat(const char*, struct hooked_stat*);
int     hooked_lstat(const char*, struct hooked_stat*);
int     hooked_fstat(int, struct hooked_stat*);
*/

#if defined(__MINGW32__)
#   undef stat
#   undef fstat
// For consistency, make mingw compile Readline the same way msvc does.
#   if defined(BUILD_READLINE)
#       undef __MINGW32__
#   endif
#endif // __MINGW32__

#if defined(BUILD_READLINE)
/*
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

// Work around quirk in complete.c
#ifdef _WIN32
#   define __WIN32__
#endif

// Readline 8.2 added usage of posixtime.h and posixselect.h, and does not
// guard usage safely in Windows.  When building READLINE_LIBRARY, define the
// timeval struct; otherwise use a forward reference to allow compilation
// while avoiding collision with winsock.h.
#define HAVE_TIMEVAL 1
#if defined (READLINE_LIBRARY)
struct timeval {
    time_t tv_sec;
    long tv_usec;
};
#else
struct timeval;
#endif
#define HAVE_GETTIMEOFDAY 1
typedef int sigset_t;               // satisfy compilation.
extern int gettimeofday(struct timeval * tp, struct timezone * tzp);

typedef int wcwidth_t (char32_t);
typedef int wcswidth_t (const char32_t*, size_t);
extern wcwidth_t *wcwidth;
extern wcswidth_t *wcswidth;
