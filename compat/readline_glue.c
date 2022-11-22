#define READLINE_LIBRARY

#include <config.h>
#include <rlmbutil.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

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

static BOOL s_have_saved_modes = 0;
static DWORD s_saved_input_mode = 0;
static DWORD s_saved_output_mode = 0;

int config_console(void)
{
    DWORD inmode;
    DWORD outmode;
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (s_have_saved_modes)
    {
        inmode = s_saved_input_mode;
        outmode = s_saved_output_mode;
    }
    else
    {
        GetConsoleMode(hin, &inmode);
        GetConsoleMode(hout, &outmode);
        s_saved_input_mode = inmode;
        s_saved_output_mode = outmode;
        s_have_saved_modes = TRUE;
    }

    inmode &= ~(ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_MOUSE_INPUT);
    inmode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    // outmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hin, inmode);
    // SetConsoleMode(hout, outmode);

    // Windows 8.1 or greater is needed for the virtual terminal escape code
    // sequences that Readline requires.  On older versions of Windows Readline
    // will display incorrectly.  But changes in the GetVersion() API make it
    // impossible to accurately detect Windows 8.1 without a manifest, so when
    // this function returns false it's inconclusive whether Windows 8.1 is
    // present.
    return IsWindows8Point1OrGreater();
}

void unconfig_console(void)
{
    if (s_have_saved_modes)
    {
        HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(hin, s_saved_input_mode);
        SetConsoleMode(hout, s_saved_output_mode);
        s_have_saved_modes = FALSE;
    }
}
