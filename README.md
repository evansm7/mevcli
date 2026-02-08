# mevcli, a little commandline for embedded projects

v0.1, Matt Evans 8th Feb 2026


This is a small & simple "library" providing a command-line interface for embedded use (such as microcontrollers).  The interface is very basic:

- Provide a user-defined list of command functions/names, and
- a character output callback function (e.g. `uart_tx`), and
- push input characters into it in an event loop.

Then, `mevcli` will manage calling your command functions, with arguments.

Full line editing features (a la `readline`) is supported.

Features:

- Single-header style
- Simple to use/incorporate
- ANSI cursor/navigation support, including `^A`/`^E` for start/end of line and `^←`/`^→` word-skip
 - `^W`/`^U` word/line cut
- 100% static allocation / no dynamic allocation required
- No external dependencies (not even `libc`)
- Small code size: about 2-3KB for RV32, AArch64

TODO, coming soon to a screen near you:

- History buffer

# Example

A simple Linux/UNIX test program is provided to illustrate how commands are defined, how `mevcli` is configured, etc.:

```
make -C test
./test/test
```


# Licence

MIT
