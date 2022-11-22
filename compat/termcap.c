// Portions Copyright (c) 2012 Martin Ridgers.
// Portions lifted from Clink (https://github.com/chrisant996/clink)
// under the MIT license (http://opensource.org/licenses/MIT).

#define READLINE_LIBRARY // Remove this if you use this file apart from Readline.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <rldefs.h>
#include <readline.h>
#include <rlprivate.h>

static char gt_termcap_buffer[64];

#define CSI(x) "\x1b[" #x
#define SS3(x) "\x1bO" #x
static const char c_default_term_ve[] = CSI(?12l) CSI(?25h);
static const char c_default_term_vs[] = CSI(?12;25h);
static const char c_default_term_vb[] = "";

static int get_cap(const char* name)
{
    int a = (int)*name;
    int b = a ? (int)name[1] : 0;
    return (a << 8) | b;
}

static void get_screen_size(int* width, int* height)
{
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle != INVALID_HANDLE_VALUE)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(handle, &csbi))
        {
            *width = csbi.dwSize.X;
            *height = (csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
            return;
        }
    }

    *width = 80;
    *height = 25;
}

int tputs(const char* str, int affcnt, int (*putc_func)(int))
{
    if (!str || !*str)
        return 0;

    if (putc_func == _rl_output_character_function)
    {
        DWORD dw;
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (!GetConsoleMode(h, &dw))
        {
            static wchar_t* s_buffer = NULL;
            static size_t s_capacity = 0;

            int len = (int)strlen(str);
            int needed = MultiByteToWideChar(CP_UTF8, 0, str, len, NULL, 0);
            if (needed <= 0)
                goto fallback;

            ++needed;
            if (s_capacity < needed)
            {
                wchar_t* buf = (wchar_t*)realloc(s_buffer, needed * sizeof(*s_buffer));
                if (!buf)
                    goto fallback;
                s_buffer = buf;
                s_capacity = needed;
            }

            int used = MultiByteToWideChar(CP_UTF8, 0, str, len, s_buffer, s_capacity);
            if (used <= 0)
                goto fallback;

            s_buffer[used] = '\0';
            _cputws(s_buffer);
            return 0;
        }
    }

fallback:
    while (*str)
        putc_func(*str++);

    return 0;
}

int tgetent(char* bp, const char* name)
{
    *bp = '\0';
    return 1;
}

int tgetnum(char* name)
{
    int width, height;
    int cap = get_cap(name);

    get_screen_size(&width, &height);

    switch (cap)
    {
    case 'co': return width;
    case 'li': return height;
    }

    return 0;
}

int tgetflag(char* name)
{
    int cap = get_cap(name);

    switch (cap)
    {
    case 'am':  return 1;
    case 'km':  return 1;
    case 'xn':  return 1;
    }

    return 0;
}

char* tgetstr(const char* name, char** out)
{
    int cap = get_cap(name);
    const char* str = NULL;

    switch (cap)
    {
    // Insert and delete N and single characters.
    case 'dc': str = CSI(P);   break;
    case 'DC': str = CSI(%dP); break;
    case 'ic': str = CSI(@);   break;
    case 'IC': str = CSI(%d@); break;

    // Clear lines and screens.
    case 'cb': str = CSI(1K);       break; // Line to cursor
    case 'ce': str = CSI(K);        break; // Line to end
    case 'cd': str = CSI(J);        break; // Screen to end
    case 'cl': str = CSI(H) CSI(J); break; // Clear screen, cursor to top-left.

    // Movement key bindings.
    case 'kh': str = CSI(H);  break; // Home
    case '@7': str = CSI(F);  break; // End
    case 'kD': str = CSI(3~); break; // Del
    case 'kI': str = CSI(2~); break; // Ins
    case 'ku': str = CSI(A);  break; // Up
    case 'kd': str = CSI(B);  break; // Down
    case 'kr': str = CSI(C);  break; // Right
    case 'kl': str = CSI(D);  break; // Left

    // Cursor movement.
    case 'ch': str = CSI(%dG); break;
    case 'cr': str = "\x0d"; break;
    case 'le': str = "\x08"; break;
    case 'LE': str = CSI(%dD); break;
    case 'nd': str = CSI(C); break;
    case 'ND': str = CSI(%dC); break;
    case 'up': str = CSI(A); break;
    case 'UP': str = CSI(%dA); break;

    // Saved cursor position.
    case 'sc': str = CSI(s); break;
    case 'rc': str = CSI(u); break;

    // Cursor style.
    case 've': str = c_default_term_ve; break;
    case 'vs': str = c_default_term_vs; break;

    // Visual bell.
    case 'vb': str = c_default_term_vb; break;
    }

    if (str && out && *out)
    {
        strcpy(*out, str);
        *out += strlen(str) + 1;
    }

    return (char*)str;

#undef SS3
#undef CSI
}

char* tgoto(const char* base, int x, int y)
{
    sprintf(gt_termcap_buffer, base, y);
    return gt_termcap_buffer;
}

