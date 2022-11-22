#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <locale.h>

#include <io.h>
#include <fcntl.h>

#include <readline/rldefs.h>
#include <readline/readline.h>
#include <readline/history.h>

static BOOL WINAPI ctrlevent_handler(DWORD ctrl_type)
{
    if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT)
    {
        // Restore console modes, otherwise input and output may not work
        // properly in the console window after the Ctrl-C and Ctrl-Break
        // terminates this program.
        unconfig_console();
    }
    return FALSE;
}

static void fix_stdin_to_binary(void)
{
    setmode(0/*stdin*/, O_BINARY);
}

int main(int argc, char **argv)
{
    // Set a Ctrl-C and Ctrl-Break handler to restore the console input and
    // output modes.  Otherwise input and output may not work properly in the
    // console window anymore after this program is aborted.
    SetConsoleCtrlHandler(ctrlevent_handler, TRUE);

    // Configure the console input and output modes for Readline.
    if (!config_console())
    {
        fputs("\x1b[93mwarning: Windows 8.1 or greater are required.\x1b[m\n"
                      "         The program must include a manifest for this warning to go away.\n\n", stderr);
    }

    // Set the locale to UTF8.  This is needed so that UTF8 input can work
    // properly in Readline.
    setlocale(LC_ALL, ".utf8");

    // Fix stdin to use binary mode so Enter (Ctrl-M) can be read as input.
    fix_stdin_to_binary();

    // Input loop.
    for (;;)
    {
        // Call readline to read a line of input.
        char *input = readline("\x1b[92mPrompt>\x1b[m ");
        if (input == 0)
            break;

        // Echo the input.
        printf("%s\n", input);

        // If the input is "exit" then break out of the input loop.
        if (strcmp(input, "exit") == 0)
            break;

        // Add the input to the history list.
        add_history(input);

        // Free the returned input string, otherwise it will leak.
        free(input);
    }

    // Restore the default locale.  Whether this is needed depends on how the
    // host program is written.  If it expects to use ACP (Active Code Page)
    // then this will be needed.  If it expects to use UTF8 then this won't be
    // needed.  If it uses some other codepage, then a different value may be
    // required here.
    setlocale(LC_ALL, "");

    unconfig_console();
    return 0;
}
