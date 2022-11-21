
# Compiling Readline on Windows

The `compat` directory contains "glue" needed for compiling Readline on Windows.

However, one issue in Readline currently requires editing the `readline\rldefs.h` file in order for compilation to properly succeed:

In `rldefs.h`, change this line:
```c
#    if !defined (__MINGW32__)
```

Into this line:
```c
#    if !defined (__MINGW32__) && !defined (_WIN32)
```

# Readline 8.1 and 8.2

For now, this sample code uses `git checkout readline-8.0` in the `readline` submodule, to pick up Readline 8.0 sources.

Readline 8.1 can't be used without modifying Readline sources, because it introduced a compilation issue in `signals.c` by moving `sigprocmask` usage outside of a `HAVE_POSIX_SIGNALS` check.

Readline 8.2 can't be used (yet) without modifying Readline sources, because it introduced some new "timeout" stuff which is not guarded sufficiently and produces compilation errors.

