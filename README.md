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
- Trivial to incorporate into a project
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

Interacting with `test` then looks a bit like this:

```
test> 
test> ?

Unknown command.  Commands are:

	prback <args...>	Print args backwards
	prcaps <a> <b>		Print both args IN CAPS
	special				Enter special mode
	unspecial			Exit special mode
	quit				Quit back to sanity


	[ You can navigate a line using cursors (use them with CTRL
	  to navigate by word), and ^A/^E to skip to the start/end.
	  Erase by word (^W), or to line start (^U) are also supported. ]
test> 
test> prback 1 2 3
Got 3 args.  In reverse order, they are: '3' '2' '1' 
test>    prback  wevw ioijasofija dsfsdf   s
Got 4 args.  In reverse order, they are: 's' 'dsfsdf' 'ioijasofija' 'wevw' 
test> 
test>  prcaps one_arg_oops

Command args are incorrect.  Commands are:

	prback <args...>	Print args backwards
	prcaps <a> <b>		Print both args IN CAPS
	special				Enter special mode
	unspecial			Exit special mode
	quit				Quit back to sanity


	[ You can navigate a line using cursors (use them with CTRL
	  to navigate by word), and ^A/^E to skip to the start/end.
	  Erase by word (^W), or to line start (^U) are also supported. ]
test> 
test>   special
specialmode> unspecial
test> 
test> quit
```

Note whitespace is ignored, and a chopped-up array of args is passed to a command handler.  The prompt can be dynamic, and here a pair of commands change/restore it.  Also, input is checked for the correct number of args for a given command (where fixed).

# Licence

MIT
