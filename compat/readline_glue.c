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

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // EPOCH is the number of 100 nanosecond intervals
    // from January 1, 1601 (UTC) to January 1, 1970.
    // (the correct value has 9 trailing zeros)
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time =  ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}
