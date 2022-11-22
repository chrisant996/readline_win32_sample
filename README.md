# Title

- _TBD: Describe purpose of this repo._

## Compiling Readline on Windows

Readline was originally intended for use on Unix and Linux.  It has some support for being used on Windows, but it tends to get broken over time because compiling for Windows is not something that's tested.

Readline versions:
- Readline 8.2 can't be used (yet) without modifying Readline sources, because it introduced some new "timeout" stuff which is not guarded sufficiently and produces compilation errors.
- Readline 8.1 can't be used without modifying Readline sources, because it introduced a compilation issue in `signals.c` by moving `sigprocmask` usage outside of a `HAVE_POSIX_SIGNALS` check.
- Readline 8.0 and earlier aren't viable.  They can be compiled by modifying a single line in the Readline sources.  But the UTF8 support has a crucial problem:  apparently on Unix `wchar_t` is 32 bits, but on Windows it is 16 bits.  That causes UTF8 support to malfunction on Windows.  The [Clink](https://github.com/chrisant996/clink) project contributed a fix for that in the Readline 8.1 timeframe.

Because none of the versions of Readline compile/function properly for Windows at this time, this sample project includes a patch file which can be applied in the git submodule.

```cmd
cd readline
git apply ../readline-8.2-win32-modifications.patch
cd ..
```

## Directories

- `compat\` contains "glue" needed for Readline to compile on Windows.
- `readline\` contains the Readline library.

The repo root contains the sample program, license file, Premake script, and README.md file.

## How to build

- _TBD: Describe how to apply the Readline patch._
- _TBD: Describe how to build (including the Premake dependency)._

## Details

- _TBD: Explain the `compat\` source code.
