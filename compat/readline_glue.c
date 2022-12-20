#define READLINE_LIBRARY

#include <config.h>
#include <rlmbutil.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdint.h> // portable: uint64_t   MSVC: __int64
#include <uchar.h> // for char32_t

#include "support/wcwidth.c"

typedef int wcwidth_t (char32_t);
typedef int wcswidth_t (const char32_t*, size_t);

wcwidth_t *wcwidth = mk_wcwidth;
wcswidth_t *wcswidth = mk_wcswidth;
