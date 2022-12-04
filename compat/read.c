#define READLINE_LIBRARY

#include "readline_glue.h"

#include <rldefs.h>
#include <readline.h>
#include <rlprivate.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

#include <assert.h>

// You may define FORCE_WIN10_16299_VT_INPUT to force using the
// ENABLE_VIRTUAL_TERMINAL_INPUT console mode.  But the mode is only available
// in Windows 10 build 16299 and higher, and it implements only a subset of VT
// input sequences.
//#ifdef FORCE_WIN10_16299_VT_INPUT

#define true                    (1)
#define false                   (0)

static char s_verbose_input = false;
void set_verbose_input(int verbose)
{
    s_verbose_input = (char)verbose;
}

/*
 * Key sequences.
 */

#define CTRL_PRESSED            (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)
#define ALT_PRESSED             (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)

#define CSI(x) "\x1b[" #x
#define SS3(x) "\x1bO" #x
#define ACSI(x) "\x1b\x1b[" #x
#define ASS3(x) "\x1b\x1bO" #x
#define MOK(x) "\x1b[27;" #x "~"

#ifndef FORCE_WIN10_16299_VT_INPUT

//                                            Shf        Ctl        CtlShf     Alt        AtlShf     AltCtl     AltCtlShf
static const char* const kcuu1[] = { CSI(A),  CSI(1;2A), CSI(1;5A), CSI(1;6A), CSI(1;3A), CSI(1;4A), CSI(1;7A), CSI(1;8A) }; // up
static const char* const kcud1[] = { CSI(B),  CSI(1;2B), CSI(1;5B), CSI(1;6B), CSI(1;3B), CSI(1;4B), CSI(1;7B), CSI(1;8B) }; // down
static const char* const kcub1[] = { CSI(D),  CSI(1;2D), CSI(1;5D), CSI(1;6D), CSI(1;3D), CSI(1;4D), CSI(1;7D), CSI(1;8D) }; // left
static const char* const kcuf1[] = { CSI(C),  CSI(1;2C), CSI(1;5C), CSI(1;6C), CSI(1;3C), CSI(1;4C), CSI(1;7C), CSI(1;8C) }; // right
static const char* const kich1[] = { CSI(2~), CSI(2;2~), CSI(2;5~), CSI(2;6~), CSI(2;3~), CSI(2;4~), CSI(2;7~), CSI(2;8~) }; // insert
static const char* const kdch1[] = { CSI(3~), CSI(3;2~), CSI(3;5~), CSI(3;6~), CSI(3;3~), CSI(3;4~), CSI(3;7~), CSI(3;8~) }; // delete
static const char* const khome[] = { CSI(H),  CSI(1;2H), CSI(1;5H), CSI(1;6H), CSI(1;3H), CSI(1;4H), CSI(1;7H), CSI(1;8H) }; // home
static const char* const kend[]  = { CSI(F),  CSI(1;2F), CSI(1;5F), CSI(1;6F), CSI(1;3F), CSI(1;4F), CSI(1;7F), CSI(1;8F) }; // end
static const char* const kpp[]   = { CSI(5~), CSI(5;2~), CSI(5;5~), CSI(5;6~), CSI(5;3~), CSI(5;4~), CSI(5;7~), CSI(5;8~) }; // pgup
static const char* const knp[]   = { CSI(6~), CSI(6;2~), CSI(6;5~), CSI(6;6~), CSI(6;3~), CSI(6;4~), CSI(6;7~), CSI(6;8~) }; // pgdn
#ifdef DIFFERENTIATE_BACKSPACE_MODIFIERS
static const char* const kbks[]  = { "\b",    MOK(2;8),  "\x7f",    MOK(6;8),  "\x1b\b",  MOK(4;8),  "\x1b\x7f",MOK(8;8)  }; // bkspc
#else
static const char* const kbks[]  = { "\b",    "\b",      "\x7f",    "\x7f",    "\x1b\b",  "\x1b\b",  "\x1b\x7f","\x1b\x7f"}; // bkspc
#endif
#ifdef DIFFERENTIATE_ENTER_MODIFIERS
static const char* const kret[]  = { "\r",    MOK(2;13), MOK(5;13), MOK(6;13), MOK(3;13), MOK(4;13), MOK(7;13), MOK(8;13) }; // enter (return)
#else
static const char* const kret[]  = { "\r",    "\r",      "\r",      "\r",      "",        "",        "",        ""        }; // enter (return)
#endif
static const char* const kcbt    = CSI(Z);
static const char* const kaltO   = CSI(27;4;79~);
static const char* const kaltlb  = CSI(27;3;91~);
static const char* const kfx[]   = {
    // kf1-12 : Fx unmodified
    SS3(P),     SS3(Q),     SS3(R),     SS3(S),
    CSI(15~),   CSI(17~),   CSI(18~),   CSI(19~),
    CSI(20~),   CSI(21~),   CSI(23~),   CSI(24~),

    // kf13-24 : shift
    CSI(1;2P),  CSI(1;2Q),  CSI(1;2R),  CSI(1;2S),
    CSI(15;2~), CSI(17;2~), CSI(18;2~), CSI(19;2~),
    CSI(20;2~), CSI(21;2~), CSI(23;2~), CSI(24;2~),

    // kf25-36 : ctrl
    CSI(1;5P),  CSI(1;5Q),  CSI(1;5R),  CSI(1;5S),
    CSI(15;5~), CSI(17;5~), CSI(18;5~), CSI(19;5~),
    CSI(20;5~), CSI(21;5~), CSI(23;5~), CSI(24;5~),

    // kf37-48 : ctrl-shift
    CSI(1;6P),  CSI(1;6Q),  CSI(1;6R),  CSI(1;6S),
    CSI(15;6~), CSI(17;6~), CSI(18;6~), CSI(19;6~),
    CSI(20;6~), CSI(21;6~), CSI(23;6~), CSI(24;6~),

    // kf1-12 : alt
    ASS3(P),     ASS3(Q),     ASS3(R),     ASS3(S),
    ACSI(15~),   ACSI(17~),   ACSI(18~),   ACSI(19~),
    ACSI(20~),   ACSI(21~),   ACSI(23~),   ACSI(24~),

    // kf13-24 : alt-shift
    ACSI(1;2P),  ACSI(1;2Q),  ACSI(1;2R),  ACSI(1;2S),
    ACSI(15;2~), ACSI(17;2~), ACSI(18;2~), ACSI(19;2~),
    ACSI(20;2~), ACSI(21;2~), ACSI(23;2~), ACSI(24;2~),

    // kf25-36 : alt-ctrl
    ACSI(1;5P),  ACSI(1;5Q),  ACSI(1;5R),  ACSI(1;5S),
    ACSI(15;5~), ACSI(17;5~), ACSI(18;5~), ACSI(19;5~),
    ACSI(20;5~), ACSI(21;5~), ACSI(23;5~), ACSI(24;5~),

    // kf37-48 : alt-ctrl-shift
    ACSI(1;6P),  ACSI(1;6Q),  ACSI(1;6R),  ACSI(1;6S),
    ACSI(15;6~), ACSI(17;6~), ACSI(18;6~), ACSI(19;6~),
    ACSI(20;6~), ACSI(21;6~), ACSI(23;6~), ACSI(24;6~),
};

//                                            Shf     Ctl         CtlShf      Alt   AtlShf   AltCtl      AltCtlShf
static const char* const ktab[]  = { "\t",    CSI(Z), MOK(5;9),   MOK(6;9),   "",   "",      "",         ""         }; // TAB
#ifdef DIFFERENTIATE_SPACE_MODIFIERS
static const char* const kspc[]  = { " ",  MOK(2;32), MOK(5;32),  MOK(6;32),  "",   "",      MOK(7;32),  MOK(8;32)  }; // SPC
#else
static const char* const kspc[]  = { " ",  " ",       " ",        " ",        "",   "",      "",         ""         }; // SPC
#endif

static int xterm_modifier(int key_flags)
{
    int i = 0;
    i |= !!(key_flags & SHIFT_PRESSED);
    i |= !!(key_flags & ALT_PRESSED) << 1;
    i |= !!(key_flags & CTRL_PRESSED) << 2;
    return i + 1;
}

static int keymod_index(int key_flags)
{
    int i = 0;
    i |= !!(key_flags & SHIFT_PRESSED);
    i |= !!(key_flags & CTRL_PRESSED) << 1;
    i |= !!(key_flags & ALT_PRESSED) << 2;
    return i;
}

static int is_vk_recognized(int key_vk)
{
    switch (key_vk)
    {
    case 'A':   case 'B':   case 'C':   case 'D':
    case 'E':   case 'F':   case 'G':   case 'H':
    case 'I':   case 'J':   case 'K':   case 'L':
    case 'M':   case 'N':   case 'O':   case 'P':
    case 'Q':   case 'R':   case 'S':   case 'T':
    case 'U':   case 'V':   case 'W':   case 'X':
    case 'Y':   case 'Z':
        return true;
    case '0':   case '1':   case '2':   case '3':
    case '4':   case '5':   case '6':   case '7':
    case '8':   case '9':
        return true;
    case VK_OEM_1:              // ';:' for US
    case VK_OEM_PLUS:           // '+' for any country
    case VK_OEM_COMMA:          // ',' for any country
    case VK_OEM_MINUS:          // '-' for any country
    case VK_OEM_PERIOD:         // '.' for any country
    case VK_OEM_2:              // '/?' for US
    case VK_OEM_3:              // '`~' for US
    case VK_OEM_4:              // '[{' for US
    case VK_OEM_5:              // '\|' for US
    case VK_OEM_6:              // ']}' for US
    case VK_OEM_7:              // ''"' for US
        return true;
    default:
        return false;
    }
}

#endif // !FORCE_WIN10_16299_VT_INPUT

/*
 * Terminal state structure.
 */

enum
{
    // Currently, the first byte in UTF8 cannot have the high 5 bits all 1.
    // That gives us room to define some magic characters:
    //      0xff  0xfe  0xfd  0xfc  0xfb  0xfa  0xf9  0xf8
    //
    // Longer term, it's probably worth pushing valid UTF8 representations of
    // invalid UTF8 codepoints, if that's possible.

    input_abort_byte    = 0xff,
    input_none_byte     = 0xfe,

    input_abort_value   = -1,
    input_none_value    = -2,
};

struct terminal_state
{
    HANDLE          m_stdin;
    HANDLE          m_stdout;
    unsigned int    m_dimensions;
    DWORD           m_prevmodein;
    DWORD           m_prevmodeout;
    char            m_initialized;
    char            m_buffer_head;      // Head index in circular m_buffer.
    char            m_buffer_count;     // Count of characters in m_buffer.
    wchar_t         m_lead_surrogate;   // Pending lead surrogate.
    char            m_buffer[16];       // Must be a power of two.
};

static struct terminal_state s_term = { 0 };

/*
 * Internal helpers.
 */

static const char* const crsr[] =
{
    CSI(?25l),  // Hide cursor.
    CSI(?25h),  // Show cursor.
};

static void show_cursor(int show)
{
    /* Using VT sequence avoids altering cursor shape in Windows Terminal. */
    assert(s_term.m_initialized);
    const char* s = crsr[!!show];
    DWORD written;
    WriteConsoleA(s_term.m_stdout, s, strlen(s), &written, NULL);
}

static unsigned int get_dimensions()
{
    assert(s_term.m_initialized);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(s_term.m_stdout, &csbi);
    short cols = (short)(csbi.dwSize.X);
    short rows = (short)(csbi.srWindow.Bottom - csbi.srWindow.Top) + 1;
    return (cols << 16) | rows;
}

static const wchar_t* key_name_from_vk(int key_vk, int scan)
{
    UINT key_scan = scan ? scan : MapVirtualKeyW(key_vk, MAPVK_VK_TO_VSC);
    if (key_scan)
    {
        LONG l = (key_scan & 0x01ff) << 16;
        static wchar_t name[16];
        if (GetKeyNameTextW(l, name, sizeof(name) / sizeof(*name)))
            return name;
    }
    return NULL;
}

static void verbose_input(const KEY_EVENT_RECORD* record)
{
    int key_char = record->uChar.UnicodeChar;
    int key_vk = record->wVirtualKeyCode;
    int key_sc = record->wVirtualScanCode;
    int key_flags = record->dwControlKeyState;

    const wchar_t* key_name = key_name_from_vk(key_vk, key_sc);

    const wchar_t* pro = (s_verbose_input > 1) ? L"\x1b[s\x1b[H" : L"";
    const wchar_t* epi = (s_verbose_input > 1) ? L"\x1b[K\x1b[u" : L"\n";

    wprintf(L"%skey event:  %c%c%c %c%c  flags=0x%08.8x  char=0x%04.4x  vk=0x%04.4x  scan=0x%04.4x  \"%s\"%s",
            pro,
            (key_flags & SHIFT_PRESSED) ? 'S' : '_',
            (key_flags & LEFT_CTRL_PRESSED) ? 'C' : '_',
            (key_flags & LEFT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_ALT_PRESSED) ? 'A' : '_',
            (key_flags & RIGHT_CTRL_PRESSED) ? 'C' : '_',
            key_flags,
            key_char,
            key_vk,
            key_sc,
            key_name ? key_name : L"UNKNOWN",
            epi);
}

static void fix_console_input_mode()
{
    assert(s_term.m_initialized);

    DWORD modeIn;
    if (GetConsoleMode(s_term.m_stdin, &modeIn))
    {
        DWORD mode = modeIn;

        // Compensate when this is reached with the console mode set wrong.
        // For example, this can happen when Lua code uses io.popen():lines()
        // and returns without finishing reading the output, or uses
        // os.execute() in a coroutine.
        mode &= ~ENABLE_PROCESSED_INPUT;

        if (mode != modeIn)
            SetConsoleMode(s_term.m_stdin, mode);
    }
}

static void fix_console_output_mode(HANDLE h, DWORD modeExpected)
{
    DWORD modeActual;
    if (GetConsoleMode(h, &modeActual) && modeActual != modeExpected)
        SetConsoleMode(h, modeExpected);
}

// Use unsigned; WCHAR and unsigned short can give wrong results.
#define IN_RANGE(n1, b, n2)     ((unsigned)((b) - (n1)) <= (unsigned)((n2) - (n1)))
static int is_lead_surrogate(unsigned int ch) { return IN_RANGE(0xD800, ch, 0xDBFF); }

static void input_push_seq(const char* seq)
{
    static const unsigned int mask = sizeof(s_term.m_buffer) - 1;

    int index;

    assert(s_term.m_initialized);
    assert(!s_term.m_lead_surrogate);
    s_term.m_lead_surrogate = 0;

    index = s_term.m_buffer_head + s_term.m_buffer_count;
    for (; s_term.m_buffer_count <= mask && *seq; ++s_term.m_buffer_count, ++index, ++seq)
    {
        assert(s_term.m_buffer_count < sizeof(s_term.m_buffer));
        if (s_term.m_buffer_count < sizeof(s_term.m_buffer))
            s_term.m_buffer[index & mask] = *seq;
        else
            return;
    }
}

static void input_push_wchar(unsigned int value)
{
    static const unsigned int mask = sizeof(s_term.m_buffer) - 1;

    assert(s_term.m_initialized);

    int index = s_term.m_buffer_head + s_term.m_buffer_count;
    wchar_t wc[3];
    unsigned int len = 0;
    char utf8[sizeof(s_term.m_buffer)];

    if (value < 0x80)
    {
        assert(!s_term.m_lead_surrogate);
        s_term.m_lead_surrogate = 0;

        assert(s_term.m_buffer_count < sizeof(s_term.m_buffer));
        if (s_term.m_buffer_count < sizeof(s_term.m_buffer))
        {
            s_term.m_buffer[index & mask] = value;
            ++s_term.m_buffer_count;
        }
        return;
    }

    if (is_lead_surrogate(value))
    {
        s_term.m_lead_surrogate = value;
        return;
    }

    if (s_term.m_lead_surrogate)
    {
        wc[len++] = s_term.m_lead_surrogate;
        s_term.m_lead_surrogate = 0;
    }
    wc[len++] = (wchar_t)value;
    wc[len] = 0;

    unsigned int n = WideCharToMultiByte(CP_UTF8, 0, wc, len, utf8, sizeof(utf8), NULL, NULL);
    for (unsigned int i = 0; i < n; ++i, ++index)
    {
        assert(s_term.m_buffer_count < sizeof(s_term.m_buffer));
        if (s_term.m_buffer_count < sizeof(s_term.m_buffer))
        {
            s_term.m_buffer[index & mask] = utf8[i];
            s_term.m_buffer_count++;
        }
    }
}

static unsigned char input_pop()
{
    assert(s_term.m_initialized);

    if (!s_term.m_buffer_count)
        return input_none_byte;

    unsigned char value = s_term.m_buffer[s_term.m_buffer_head];

    --s_term.m_buffer_count;
    s_term.m_buffer_head = (s_term.m_buffer_head + 1) & (sizeof(s_term.m_buffer) - 1);

    return value;
}

static unsigned char input_peek()
{
    assert(s_term.m_initialized);

    if (!s_term.m_buffer_count)
        return input_none_byte;

    return s_term.m_buffer[s_term.m_buffer_head];
}

#ifndef FORCE_WIN10_16299_VT_INPUT

// Try to handle Alt-Ctrl-[, Alt-Ctrl-], Alt-Ctrl-\ better, at least in keyboard
// layouts where the [, ], or \ is the regular (unshifted) name of the key.
static int translate_ctrl_bracket(int* key_vk, int key_sc)
{
    // Can't realistically apply caching here, because software keyboard layouts
    // can be changed dynamically.
    const wchar_t* key_name = key_name_from_vk(*key_vk, key_sc);
    if (!key_name || !key_name[0] || key_name[1])
        return false;

    switch (key_name[0])
    {
    case '[':
    case ']':
    case '\\':
        *key_vk = key_name[0] - '@';
        return true;
    default:
        return false;
    }
}

static void vt_emulation(int key_char, int key_vk, int key_sc, int key_flags)
{
    // Windows supports an AltGr substitute which we check for here and ignore.
    if (key_flags & LEFT_ALT_PRESSED)
    {
        int altgr_sub = !!(key_flags & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED));
        altgr_sub &= !!key_char;

        if (altgr_sub)
        {
            altgr_sub = false;
            key_char = 0;
        }

        if (!altgr_sub)
            key_flags &= ~(RIGHT_ALT_PRESSED);
        else
            key_flags &= ~(LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED);
    }

    // Special treatment for variations of tab and space. Do this before
    // clearing AltGr flags, otherwise ctrl-space gets converted into space.
    if (key_vk == VK_TAB && (key_char == 0x09 || !key_char) && !s_term.m_buffer_count)
    {
        input_push_seq(ktab[keymod_index(key_flags)]);
        return;
    }
    if (key_vk == VK_SPACE && (key_char == 0x20 || !key_char) && !s_term.m_buffer_count)
    {
        input_push_seq(kspc[keymod_index(key_flags)]);
        return;
    }

    // Special treatment for enter + modifiers.
    if (key_vk == VK_RETURN && key_flags && !s_term.m_buffer_count)
    {
        input_push_seq(kret[keymod_index(key_flags)]);
        return;
    }

    // If the input was formed using AltGr or LeftAlt-LeftCtrl then things get
    // tricky. But there's always a Ctrl bit set, even if the user didn't press
    // a ctrl key. We can use this and the knowledge that Ctrl-modified keys
    // aren't printable to clear appropriate AltGr flags.
    if ((key_char > 0x1f && key_char != 0x7f) && (key_flags & CTRL_PRESSED))
    {
        key_flags &= ~CTRL_PRESSED;
        if (key_flags & RIGHT_ALT_PRESSED)
            key_flags &= ~RIGHT_ALT_PRESSED;
        else
            key_flags &= ~LEFT_ALT_PRESSED;
    }

    // Special case for ctrl-shift-I (to behave like shift-tab aka. back-tab).
#if !defined (DIFFERENTIATE_CTRL_CHARS)
    if (key_char == '\t' && !s_term.m_buffer_count && (key_flags & SHIFT_PRESSED))
    {
        input_push_seq(kcbt);
        return;
    }
#endif

    // Function keys (kf1-kf48 from xterm+pcf2)
    unsigned key_func = key_vk - VK_F1;
    if (key_func <= (VK_F12 - VK_F1))
    {
        int kfx_group = keymod_index(key_flags);
        input_push_seq((kfx + (12 * kfx_group) + key_func)[0]);
        return;
    }

    // Include an ESC character in the input stream if Alt is pressed.
    if (key_char)
    {
        int simple_char;

#ifdef DIFFERENTIATE_CTRL_CHARS
        const int differentiate_keys = true;
#else
        const int differentiate_keys = false;
#endif

        assert(key_vk != VK_TAB);
        if (key_vk == 'H' || key_vk == 'I' || key_vk == 'M')
            simple_char = !(key_flags & CTRL_PRESSED) || !differentiate_keys;
        else if (key_char == 0x1b && key_vk != VK_ESCAPE)
            simple_char = keymod_index(key_flags) < 2; // Modifiers were resulting in incomplete escape codes.
        else if (key_vk == VK_RETURN || key_vk == VK_BACK)
            simple_char = !(key_flags & (CTRL_PRESSED|SHIFT_PRESSED));
        else
            simple_char = !(key_flags & CTRL_PRESSED) || !(key_flags & SHIFT_PRESSED);

        if (simple_char)
        {
            if (key_flags & ALT_PRESSED)
                input_push_wchar(0x1b);
            input_push_wchar(key_char);
            return;
        }
    }

    const char* const* seqs = NULL;
    switch (key_vk)
    {
    case VK_UP:     seqs = kcuu1; break;    // up
    case VK_DOWN:   seqs = kcud1; break;    // down
    case VK_LEFT:   seqs = kcub1; break;    // left
    case VK_RIGHT:  seqs = kcuf1; break;    // right
    case VK_HOME:   seqs = khome; break;    // insert
    case VK_END:    seqs = kend; break;     // delete
    case VK_INSERT: seqs = kich1; break;    // home
    case VK_DELETE: seqs = kdch1; break;    // end
    case VK_PRIOR:  seqs = kpp; break;      // pgup
    case VK_NEXT:   seqs = knp; break;      // pgdn
    case VK_BACK:   seqs = kbks; break;     // bkspc
    }
    if (seqs)
    {
        input_push_seq(seqs[keymod_index(key_flags)]);
        return;
    }

    // This builds Ctrl-<key> c0 codes. Some of these actually come though in
    // key_char and some don't.
    if (key_flags & CTRL_PRESSED)
    {
        int ctrl_code = false;

        if (!(key_flags & SHIFT_PRESSED) || key_vk == '2' || key_vk == '6')
        {
            ctrl_code = true;

            switch (key_vk)
            {
            case 'A':   case 'B':   case 'C':   case 'D':
            case 'E':   case 'F':   case 'G':   case 'H':
            case 'I':   case 'J':   case 'K':   case 'L':
            case 'M':   case 'N':   case 'O':   case 'P':
            case 'Q':   case 'R':   case 'S':   case 'T':
            case 'U':   case 'V':   case 'W':   case 'X':
            case 'Y':   case 'Z':
#ifdef DIFFERENTIATE_CTRL_CHARS
                if (key_vk == 'H' || key_vk == 'I' || key_vk == 'M')
                    goto not_ctrl;
#endif
                key_vk -= 'A' - 1;
                ctrl_code = true;
                break;
            case '2':
#ifdef DIFFERENTIATE_CTRL_CHARS
                if (!(key_flags & SHIFT_PRESSED))
                    goto not_ctrl;
#endif
                key_vk = 0;
                break;
            case '6':
#ifdef DIFFERENTIATE_CTRL_CHARS
                if (!(key_flags & SHIFT_PRESSED))
                    goto not_ctrl;
#endif
                key_vk = 0x1e;
                break;
            case VK_OEM_MINUS:          // 0xbd, - in any country.
                key_vk = 0x1f;
                break;
            default:
                // Can't use VK_OEM_4, VK_OEM_5, and VK_OEM_6 for detecting ^[,
                // ^\, and ^] because OEM key mapping differ by keyboard/locale.
                // However, the OS/OEM keyboard driver produces enough details
                // to make it possible to identify what's really going on, at
                // least for these specific keys (but not for VK_OEM_MINUS, 2,
                // or 6).  Ctrl makes the bracket and backslash keys produce the
                // needed control code in key_char, so we can simply use that.
                switch (key_char)
                {
                case 0x1b:
#ifdef DIFFERENTIATE_CTRL_CHARS
                    goto not_ctrl;
#endif
                    // fall thru
                case 0x1c:
                case 0x1d:
                    key_vk = key_char;
                    break;
                default:
#ifdef DIFFERENTIATE_CTRL_CHARS
not_ctrl:
#endif
                    if (!translate_ctrl_bracket(&key_vk, key_sc))
                        ctrl_code = false;
                    break;
                }
                break;
            }
        }

        if (ctrl_code)
        {
            if (key_flags & ALT_PRESSED)
                input_push_wchar(0x1b);
            input_push_wchar(key_vk);
            return;
        }
    }

    // Ok, it's a key that doesn't have a "normal" terminal representation.  Can
    // we produce an extended XTerm input sequence for the input?
    if (is_vk_recognized(key_vk))
    {
        char key_seq[32];
        int mod = xterm_modifier(key_flags);
        if (mod >= 2)
            sprintf(key_seq, "\x1b[27;%u;%u~", mod, key_vk);
        else
            sprintf(key_seq, "\x1b[27;%u~", key_vk);
        input_push_seq(key_seq);
        return;
    }
}

#endif // !FORCE_WIN10_16299_VT_INPUT

static void process_input(const KEY_EVENT_RECORD* record)
{
    assert(s_term.m_initialized);

    int key_char = record->uChar.UnicodeChar;
    int key_vk = record->wVirtualKeyCode;
    int key_sc = record->wVirtualScanCode;
    int key_flags = record->dwControlKeyState;

    // Only respond to key down events.
    if (!record->bKeyDown)
    {
        // Some times conhost can send through ALT codes, with the resulting
        // Unicode code point in the Alt key-up event.
        if (key_vk == VK_MENU && key_char)
            key_flags = 0;
        else
            return;
    }

    // We filter out Alt key presses unless they generated a character.
    if (key_vk == VK_MENU)
    {
        if (key_char)
        {
            if (s_verbose_input)
                verbose_input(record);
            input_push_wchar(key_char);
        }
        return;
    }

    // Early out of unaccompanied Ctrl/Shift/Windows key presses.
    if (key_vk == VK_CONTROL || key_vk == VK_SHIFT || key_vk == VK_LWIN || key_vk == VK_RWIN)
        return;

    if (s_verbose_input)
        verbose_input(record);

#ifdef FORCE_WIN10_16299_VT_INPUT

    if (key_char)
    {
        if (s_verbose_input)
            verbose_input(record);
        input_push_wchar(key_char);
        return;
    }

#else // !FORCE_WIN10_16299_VT_INPUT

    vt_emulation(key_char, key_vk, key_sc, key_flags);

#endif // !FORCE_WIN10_16299_VT_INPUT
}

static int read_console(DWORD _timeout, int peek)
{
    assert(s_term.m_initialized);

    // Hide the cursor unless we're accepting input so we don't have to see it
    // jump around as the screen's drawn.
    show_cursor(true);

    // Conhost restarts the cursor blink when writing to the console. It restarts
    // hidden which means that if you type faster than the blink the cursor turns
    // invisible. Fortunately, moving the cursor restarts the blink on visible.
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(s_term.m_stdout, &csbi);
    SetConsoleCursorPosition(s_term.m_stdout, csbi.dwCursorPosition);

    // Read input records sent from the terminal (aka conhost) until some
    // input has been buffered.
    const DWORD started = GetTickCount();
    const unsigned int buffer_count = s_term.m_buffer_count;
    while (buffer_count == s_term.m_buffer_count)
    {
        DWORD dwWait;
        DWORD modeExpected;
        const int has_mode = !!GetConsoleMode(s_term.m_stdout, &modeExpected);

        fix_console_input_mode();

        dwWait = WaitForSingleObject(s_term.m_stdin, _timeout);
        if (dwWait != WAIT_OBJECT_0)
            return false;

        if (has_mode)
            fix_console_output_mode(s_term.m_stdout, modeExpected);

        DWORD count;
        INPUT_RECORD record;
        if (!(peek ?
              PeekConsoleInputW(s_term.m_stdin, &record, 1, &count) :
              ReadConsoleInputW(s_term.m_stdin, &record, 1, &count)))
        {
            // Handle's probably invalid if ReadConsoleInput() failed.
            s_term.m_buffer_head = 0;
            s_term.m_buffer_count = 1;
            s_term.m_buffer[0] = input_abort_byte;
            goto out;
        }

        switch (record.EventType)
        {
        case KEY_EVENT:
            process_input(&record.Event.KeyEvent);
            break;

        case WINDOW_BUFFER_SIZE_EVENT:
            {
                unsigned int newdim = get_dimensions();
                if (newdim != s_term.m_dimensions)
                {
                    s_term.m_dimensions = newdim;
                    rl_resize_terminal();
                }
            }
            break;
        }

        // Eat records that don't result in available input.
        if (peek && buffer_count == s_term.m_buffer_count)
            ReadConsoleInputW(s_term.m_stdin, &record, 1, &count);
    }

out:
    show_cursor(false);
    return true;
}

/*
 * Public terminal functions.
 */

static int input_getc(FILE *stream);

int config_console(void)
{
    DWORD inmode;
    DWORD outmode;

    s_term.m_buffer_count = 0;
    s_term.m_lead_surrogate = 0;
    s_term.m_stdin = GetStdHandle(STD_INPUT_HANDLE);
    s_term.m_stdout = GetStdHandle(STD_OUTPUT_HANDLE);

    if (!s_term.m_initialized)
    {
        rl_getc_function = input_getc;
        GetConsoleMode(s_term.m_stdin, &s_term.m_prevmodein);
        GetConsoleMode(s_term.m_stdout, &s_term.m_prevmodeout);
        s_term.m_initialized = true;
    }
    inmode = s_term.m_prevmodein;
    outmode = s_term.m_prevmodeout;

    inmode &= ~(ENABLE_PROCESSED_INPUT|ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT|ENABLE_MOUSE_INPUT);
#ifdef FORCE_WIN10_16299_VT_INPUT
    inmode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
#endif
    SetConsoleMode(s_term.m_stdin, inmode);

    outmode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(s_term.m_stdout, outmode);

    s_term.m_dimensions = get_dimensions();

    show_cursor(false);

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
    if (s_term.m_initialized)
    {
        show_cursor(true);

        rl_getc_function = rl_getc;

        if (s_term.m_stdin)
            SetConsoleMode(s_term.m_stdin, s_term.m_prevmodein);
        if (s_term.m_stdout)
            SetConsoleMode(s_term.m_stdout, s_term.m_prevmodeout);
    }

    memset(&s_term, 0, sizeof(s_term));
}

static int calc_rl_timeout(DWORD* timeout)
{
    unsigned int secs;
    unsigned int usecs;
    switch (rl_timeout_remaining(&secs, &usecs))
    {
    case 0:     return false;
    case -1:    *timeout = INFINITE; return true;
    default:    *timeout = secs * 1000 + usecs / 1000; return true;
    }
}

void input_select()
{
    assert(s_term.m_initialized);

    if (!s_term.m_buffer_count)
    {
        DWORD timeout;
        if (!calc_rl_timeout(&timeout))
            return;

        read_console(timeout, false/*peek*/);
    }
}

int input_read()
{
    assert(s_term.m_initialized);

    if (s_term.m_buffer_count)
    {
        unsigned char c = input_pop();
        switch (c)
        {
        case input_none_byte:       return input_none_value;
        case input_abort_byte:      return input_abort_value;
        default:                    return c;
        }
    }

    return input_none_value;
}

int input_available(void)
{
    assert(s_term.m_initialized);

    while (!s_term.m_buffer_count)
    {
        DWORD timeout;
        if (!calc_rl_timeout(&timeout))
            return 0;

        // Read console input.  This is necessary to filter out OS events that
        // should not be processed as input.
        if (!read_console(timeout, true/*peek*/))
            return 0;

        // If real input is available, break out.
        const unsigned char k = input_peek();
        if (k != input_none_byte)
            break;

        // Eat the input.
        input_read();
    }
    return s_term.m_buffer_count > 0;
}

static int input_getc(FILE *stream)
{
    DWORD dummy;
    int c;

    assert(s_term.m_initialized);

    if (stream != rl_instream || !GetConsoleMode(s_term.m_stdin, &dummy))
        return rl_getc(stream);

    while (1)
    {
        RL_CHECK_SIGNALS();
        // We know at this point that _rl_caught_signal == 0.

        // Wait for more input if there isn't input pending.
// FUTURE: support SIGINT?
        input_select();

        // In case the terminal dimensions changed without being able to
        // trigger a console event, or we missed the event.
        if (!s_term.m_buffer_count)
        {
            unsigned int dimensions = get_dimensions();
            if (dimensions != s_term.m_dimensions)
            {
                s_term.m_dimensions = dimensions;
                rl_resize_terminal();
            }
        }

        c = input_read();
        if (c == input_none_value)
            continue;

        // If reading fails, consider it fatal.
        if (c == input_abort_value)
            return (RL_ISSTATE(RL_STATE_READCMD) ? READERR : EOF);

        // Return the next character from the input.
        assert(!(c & ~0xff));
        return (c);

#if 0
        // Keyboard-generated signals of interest.
        if (_rl_caught_signal == SIGINT)
            RL_CHECK_SIGNALS();

        if (rl_signal_event_hook)
            (*rl_signal_event_hook)();
#endif
    }
}
