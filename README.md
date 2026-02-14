# Using Readline in Windows

This repo demonstrates how to compile and link the [GNU Readline library](https://tiswww.case.edu/php/chet/readline/rltop.html) in a Windows console application.

To learn how to configure Readline, and to identify what other integrations you may want to support (e.g. manage persistent history), read the [Readline documentation](https://tiswww.case.edu/php/chet/readline/rltop.html#Documentation).

> [!NOTE]
> Be sure to read the Readline documentation.  Fair warning:  any questions that can be answered simply by reading the documentation will be answered with "please read the Readline documentation".

The sample code in this repo is based on my experience with using Readline in the [Clink](https://github.com/chrisant996/clink) project, which enhances the CMD shell to use the Readline library for inputting and editing command lines.  Clink makes very specialized and sophisticated use of Readline (don't copy its approach; chances are 99.9999% that's not what you need), but this sample repo shows the basics so you can get started, and then you can determine how to proceed from there.

**LICENSE:**  This sample is distributed under the [GNU General Public License v3](https://github.com/chrisant996/readline_win32_sample/blob/main/LICENSE).

> [!WARNING]
> This sample is for illustration purposes only.  Do not use it in production software.  You are solely responsible for using Readline correctly and any problems that may arise.

## Prequisites

You'll need the following:
- [git](https://git-scm.com/downloads), to clone the repo and apply a patch and other source control operations.
- [Premake](https://premake.github.io/), to create project files for building using your preferred compiler.
- A C/C++ compiler such as Visual Studio or MinGW.
- Familiarity with [codepages](https://learn.microsoft.com/en-us/windows/win32/intl/code-pages), Active Code Page, UTF8, and -`A` ("ANSI") versus -`W` ("Wide") APIs on Windows.
- Windows 8.1 or higher, for native terminal support.
- _If you use Windows 8.0 or lower, you'll have to use either a terminal emulator or a utility like ANSICON._

## Clone the repo

First you'll need to set up the local repo:

```cmd
:: Optional: create a parent directory.
:: Otherwise: change to the directory under which you want to clone the repo.
md c:\repos
cd /d c:\repos

:: Clone the repo.
git clone --recurse-submodules https://github.com/chrisant996/readline_win32_sample.git
cd readline_win32_sample
```

You'll need to apply a patch to fix some Readline compile errors on Windows.  Readline was originally intended for use on Unix and Linux.  It has some support for being used on Windows, but the Windows support tends to get broken over time because it's not something that receives much testing.

```cmd
:: Apply the patch to fix Readline compile errors on Windows.
cd readline
git apply ../readline-8.2-win32-modifications.patch
cd ..
```

Now the repo should be cloned and the Readline library should be ready for compiling.

## Directories

- `compat\` contains "glue" needed for Readline to compile on Windows.
- `readline\` contains the Readline library.

The repo root contains the sample program, license file, Premake script, and README.md file.

## Compiling the sample

First, run Premake to generate project files for your compiler.  Refer to the Premake documentation for help, if needed.

```cmd
:: Generate project files for use with VS2019.
premake5 vs2019
```

Next, invoke your compiler to build using the generated project files.  For example, with Visual Studio you could open the `.build/vs2019/sample.sln` file and then press <kbd>F5</kbd> to build and run the sample.

## Input mode

The sample contains two functions that handle ensuring the console mode can support Readline:
- `config_console()` saves the current console mode, and adjusts the console mode to support Readline input.
- `unconfig_console()` restores the saved console mode.

Call `config_console()` before calling Readline, otherwise input won't work correctly in Readline.

Call `unconfig_console()` after Readline returns, otherwise input won't work correctly outside of Readline.

## IMPORTANT:  UTF8, ACP, -`A`, and -`W` APIs

For Readline to work properly, the input codepage needs to be UTF8.

The sample program in this repo sets the C runtime to use UTF8 as the input/output locale.

However, that also affects the rest of your program.  You'll need to make sure your program handles UTF8 correctly everywhere, or things will be wrong.

> **Warning:** If the program isn't properly prepared for the C runtime to be using UTF8, then the problematic effects will not be limited to just input.  Everywhere the program uses the C runtime library, UTF8 inputs and outputs will be expected, and things can go wrong if the program isn't handling UTF8 correctly.

You may need to use [`MultiByteToWideChar()`](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-multibytetowidechar) and other conversion APIs to correctly handle converting between ACP, UTF8, and UTF16 depending on the needs of your program.

Or you can update your program to interally use UTF8 and/or UTF16 -- this is what the [Clink](https://github.com/chrisant996/clink) project does.  It's the best long-term approach, and positions your application the best for international compatibility and for the future of Unicode.

## APPENDICES

### ANSI escape codes in prompt strings

Remember to surround any escape sequences in the prompt string with `\001` .. `\002` as described in the Readline documentation for [`rl_expand_prompt()`](https://tiswww.case.edu/php/chet/readline/readline.html#index-rl_005fexpand_005fprompt).  Even if you don't call that function directly, Readline calls it internally.  Any escape sequences must be bracketed properly or the display will become garbled.

> **Note:**  If you forget to do this, the display will intermittently become garbled in strange ways.  If you experience garbling, this is the first thing to check.

### VT output emulation

Readline requires VT (virtual terminal) emulation to be able to display the input line.

- Windows 8.1 and higher have VT emulation built-in.
- If you use Windows 8.0 or lower, you will need to use a separate terminal emulator or a utility program such as ANSICON.

Your program should check the Windows version to determine whether VT emulation is available.  Because of how [GetVersion()](https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversion) and [IsWindows8Point1OrGreater()](https://learn.microsoft.com/en-us/windows/win32/api/versionhelpers/nf-versionhelpers-iswindows8point1orgreater) work, they will not accurately report the Windows version unless your program uses a manifest to [target the OS versions it supports](https://learn.microsoft.com/en-us/windows/win32/sysinfo/targeting-your-application-at-windows-8-1).

### VT input emulation

Readline requires VT input emulation to be able to process input key bindings.

This sample repo uses a custom input callback which provides VT input emulation on all versions of Windows (see the `read.c` file).

Alternatively, you could supply your own custom input callback by setting `rl_getc_function` after calling `config_console()`.

### Readline versions

Some notes on specific Readline versions:

- Readline 8.2 can't be used (yet) without modifying Readline sources, because it introduced some new "timeout" stuff which is not guarded sufficiently and produces compilation errors.  The included patch file works around the compilation errors, until an official patch is available.
- Readline 8.1 can't be used without modifying Readline sources, because it introduced a compilation issue in `signals.c` by moving `sigprocmask` usage outside of a `HAVE_POSIX_SIGNALS` check.
- Readline 8.0 and earlier aren't viable for use on Windows unless the input is ASCII and `HANDLE_MULTIBYTE` is disabled.  It can be compiled by modifying a single line in the Readline sources.  But the UTF8 support has a crucial problem:  apparently on Unix `wchar_t` is 32 bits, but on Windows it is 16 bits.  That causes UTF8 support to malfunction on Windows.  The [Clink](https://github.com/chrisant996/clink) project contributed a fix for that in the Readline 8.1 timeframe.
