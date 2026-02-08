/*
 * A tiny CLI for microcontrollers, with decent ANSI line-editing.
 * Commands are simply matched from an application-supplied table, and
 * invoked with any arguments provided.  This code avoids all dynamic
 * memory allocation, uses little stack, and needs no external
 * dependencies beyond C type headers; this makes it useful for very
 * small microcontrollers.
 *
 * To use mevcli, #include this header into one .c file after
 * overriding any necessary config (see "Config:" below).  In the .c
 * file, register commands in an array of mevcli_cmd_t structures,
 * then pass this to mevcli_init() along with a callback for character
 * output.  Finally, when input characters arrive pass them to the
 * command interpreter via mevcli_input_char(), which might then
 * invoke the command callbacks as appropriate.
 *
 * TODO:
 * - Config to reduce code size (e.g. remove certain navigation
 *   or editing functions)
 * - History buffer
 * - "Yank" for previously-cut words/spans
 *
 * v0.1 8 Feb 2026, Matt Evans
 *
 *
 *  Copyright © 2026 Matt Evans
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the “Software”), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _MEVCLI_H
#define _MEVCLI_H

#include <stdint.h>


////////////////////////////////////////////////////////////////////////////////
// Config: can be overridden by the application before #including this

#ifndef MEVCLI_HISTORY_LEN
#define MEVCLI_HISTORY_LEN		0	/* Number of lines of history to store */
#endif

#ifndef MEVCLI_MAX_LINE_LEN
#define MEVCLI_MAX_LINE_LEN		78	/* Chars per line, beyond prompt */
#endif

#ifndef MEVCLI_MAX_ARGS
#define MEVCLI_MAX_ARGS			8	/* Max num of args for any command */
#endif

#ifndef MEVCLI_PROMPT
#define MEVCLI_PROMPT			"> "	/* Prompt; could be set to a char* variable */
#endif

#ifndef MEVCLI_ASSERT
#define MEVCLI_ASSERT(x)		do {} while(0)
#endif

#if MEVCLI_HISTORY_LEN > 0
/* FIXME: This'll get done next. :) */
#error "Mevcli doesn't yet support command history"
#endif


////////////////////////////////////////////////////////////////////////////////
// User-visible types

/* This struct defines a commmand; your application needs to create an
 * array of one or more mevcli_cmd_t structures and pass it to
 * mevcli_init().  The array must remain valid for the life of
 * interacting with mevcli.
 *
 * name:	A C string for the command name.
 * help:	A string describing the command and arguments, printed
 *		when command help is invoked.
 * cmdfn:	A function called in response to the command.  Arguments
 *		are passed as an array of strings (with argc of them;
 *		note unlike main(), argc=1 for one argument to the command,
 *		i.e. argv does not include the command name).
 * opaque:	A value passed to cmdfn() calls.
 * nargs:	Number of expected args, or -1 for a variable number (in
 *		all cases up to the hard limit of MEVCLI_MAX_ARGS).
 *		Used to avoid having to check arg number in all commands.
 */
typedef struct {
	const char *name;
	const char *help;
	void *opaque;
	void (*cmdfn)(void *opaque, int argc, char **argv);
	int nargs;
} mevcli_cmd_t;


////////////////////////////////////////////////////////////////////////////////
// External API

typedef struct mevcli_ctx mevcli_ctx_t;

/* Init mevcli and register commands with it.
 *
 * cmds:		Array of mevcli_cmd_t descriptors of commands.
 *			Read-only and accessed in place, so must remain valid
 *			throughout usage.
 * num_cmds:		Number of array entries
 * cb_output_char:	Callback used to output characters
 */

void	mevcli_init(mevcli_ctx_t *ctx,
		    const mevcli_cmd_t *cmds, unsigned int num_cmds,
		    void (*cb_output_char)(char out));

/* Pass input character to mevcli.
 * in:			Character to input
 */
void	mevcli_input_char(mevcli_ctx_t *ctx, const char in);


////////////////////////////////////////////////////////////////////////////////
//									      //
//									      //
//									      //
//									      //
//									      //
//									      //
//									      //
//									      //
////////////////////////////////////////////////////////////////////////////////
// Internal types

#define MEVCLI_BELL_CHAR	7

typedef struct mevcli_ctx {
	void (*cb_output_char)(char out);
	const mevcli_cmd_t *commands;
	unsigned int num_commands;

	/* Length of the prompt, for screen drawing purposes;
	 * note this is dynamic and might be updated by a command!
	 */
	unsigned int prompt_len;

	/* Position of end of line in line[] buffer: */
	unsigned int linepos;

	/* Cursor position within line (expected to be between 0 and
	 * linepos) (this doesn't include the prompt).	If an edit is
	 * made, we can determine whether it's mid-line (if cursorpos
	 * < linepos) or at the end (cursorpos == linepos).
	 */
	unsigned int cursorpos;

	/* FSM for CSI/escape detection:
	 *  0 = idle, 1 = got escape, 2 = got [
	 */
	unsigned int csi_fsm_state;

	/* Line buffer working storage (inc terminator) */
	char line[MEVCLI_MAX_LINE_LEN + 1];

	/* Storage for argv pointers */
	char *args[MEVCLI_MAX_ARGS];
} mevcli_ctx_t;


////////////////////////////////////////////////////////////////////////////////
// Internal functions

///////////////////////// Output and ANSI cursor control ///////////////////////

/* It's a little crude to open-code string functions but, as we really
 * only need compare and copy, doing so avoids depending on any
 * libc/stdio functions.  This can be a big win when using this for
 * tiny embedded/MCU systems.
 */

static void	mevcli_putch(mevcli_ctx_t *ctx, char c)
{
	ctx->cb_output_char(c);
}

static unsigned int	mevcli_putstr(mevcli_ctx_t *ctx, const char *str)
{
	unsigned int len = 0;
	while (*str) {
		mevcli_putch(ctx, *(str++));
		len++;
	}
	return len;
}

static void	mevcli_newl(mevcli_ctx_t *ctx)
{
	mevcli_putstr(ctx, "\r\n");
}

static void	mevcli_prompt(mevcli_ctx_t *ctx)
{
	ctx->prompt_len = mevcli_putstr(ctx, MEVCLI_PROMPT);
}

static void	mevcli_ansi_eraseline(mevcli_ctx_t *ctx)
{
	mevcli_putstr(ctx, "\e[2K");
}

static void	mevcli_ansi_eraseright(mevcli_ctx_t *ctx)
{
	mevcli_putstr(ctx, "\e[0K");
}

static void	mevcli_ansi_cursorpos(mevcli_ctx_t *ctx, unsigned int x)
{
	if (x > 999)
		return;		/* SORRY */

	if (x == 0) {		/* Shortcut for common case */
		mevcli_putch(ctx, '\r');
		return;
	}

	x++;	/* Irritatingly, terminal columns are 1-indexed */

	mevcli_putstr(ctx, "\e[");
	/* Some hacky decimal=print formatting, avoiding printf */
	char digits[3];
	unsigned int numdig = 0;
	for ( ; numdig < 3; numdig++) {
		digits[numdig] = x % 10;
		x /= 10;
		if (x == 0)
			break;
	}
	for (int i = numdig; i >= 0; i--) {
		mevcli_putch(ctx, '0' + digits[i]);
	}
	mevcli_putch(ctx, 'G');
}


///////////////////////// Command execution ////////////////////////////////////

static void	mevcli_help(mevcli_ctx_t *ctx, const char *why)
{
	mevcli_newl(ctx);
	mevcli_putstr(ctx, why);
	mevcli_putstr(ctx, ".  Commands are:\r\n\r\n");
	for (unsigned int cmd = 0; cmd < ctx->num_commands; cmd++) {
		mevcli_putch(ctx, '\t');
		mevcli_putstr(ctx, ctx->commands[cmd].name);
		mevcli_putstr(ctx, ctx->commands[cmd].help);
		mevcli_newl(ctx);
	}
	mevcli_newl(ctx);
#ifdef MEVCLI_EXTRA_HELPSTRING
	mevcli_putstr(ctx, MEVCLI_EXTRA_HELPSTRING);
#endif
}

static char	mevcli_lowercase_alpha(char c)
{
	if (c >= 'A' && c <= 'Z')
		c |= 0x20;
	return c;
}

/* A case-insensitive string comparison; true on match.
 */
static bool	mevcli_str_match(char *needle, const char *haystack)
{
	while (*needle && *haystack) {
		if (mevcli_lowercase_alpha(*(needle++)) !=
		    mevcli_lowercase_alpha(*(haystack++)))
			return false;
	}
	/* True if we got to the end of both strings  without mismatch. */
	return !*needle && !*haystack;
}

/* Having got an entered line, do two things:
 * 1) Match the first word into a command string
 * 2) Create an argv array of the remainder of the line chopped at whitespace
 *    boundaries, up to MEVCLI_MAX_ARGS
 */
static void	mevcli_process_cmd(mevcli_ctx_t *ctx)
{
	/* Terminate input line */
	ctx->line[ctx->linepos] = '\0';

	mevcli_newl(ctx);

	int command_idx = -1;
	/* Skip past any spaces that might be at the start of the
	 * line, to find the command:
	 */
	for (unsigned int i = 0; i < ctx->linepos; i++) {
		if (ctx->line[i] > ' ') {
			command_idx = i;
			break;
		}
	}
	/* No actual command on the line! */
	if (command_idx == -1)
		goto out;

	char *command = &ctx->line[command_idx];

	/* Plonk terminators between each word: */
	int aftercmd_idx = -1;
	for (unsigned int i = command_idx; i < ctx->linepos; i++) {
		if (ctx->line[i] <= ' ') {
			ctx->line[i] = '\0';
			/* Make note of first gap after command */
			if (aftercmd_idx == -1)
				aftercmd_idx = i;
		}
	}

	unsigned int gotcmd = ~0;
	for (unsigned int c = 0; c < ctx->num_commands; c++) {
		/* FIXME: Where a match is short, but unambiguous, match anyway
		 * for convenience.
		 */
		if (mevcli_str_match(command, ctx->commands[c].name)) {
			gotcmd = c;
			break;
		}
	}
	if (gotcmd == ~0) {
		mevcli_help(ctx, "Unknown command");
		goto out;
	}
	const mevcli_cmd_t *cmd = &ctx->commands[gotcmd];

	/* Finally, construct argv */
	unsigned int argc = 0;
	if (aftercmd_idx != -1) {
		/* There was text after the command, find args */
		for (unsigned int i = aftercmd_idx; i < ctx->linepos; i++) {
			if (ctx->line[i] != '\0') {
				ctx->args[argc] = &ctx->line[i];
				argc++;

				if (argc >= MEVCLI_MAX_ARGS)
					break;

				/* Skip over arg */
				for ( ; i < ctx->linepos; i++) {
					if (ctx->line[i] == '\0')
						break;
				}
			}
		}
	}

	if ((cmd->nargs != -1) && (cmd->nargs != argc)) {
		mevcli_help(ctx, "Command args are incorrect");
		goto out;
	}

	cmd->cmdfn(cmd->opaque, argc, ctx->args);

out:
	ctx->cursorpos = ctx->linepos = 0;
	mevcli_prompt(ctx);
}


///////////////////////// Cursor movement //////////////////////////////////////

static void	mevcli_cursor_up(mevcli_ctx_t *ctx)
{
	/* FIXME: Implement when we do history */
}

static void	mevcli_cursor_down(mevcli_ctx_t *ctx)
{
	/* FIXME: Implement when we do history */
}

static void	mevcli_cursor_right(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos < ctx->linepos) {
		ctx->cursorpos++;
		mevcli_ansi_cursorpos(ctx, ctx->cursorpos + ctx->prompt_len);
	}
}

static void	mevcli_cursor_left(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos > 0) {
		ctx->cursorpos--;
		mevcli_ansi_cursorpos(ctx, ctx->cursorpos + ctx->prompt_len);
	}
}

/* From the current cursor position, search leftwards (downwards)
 * in the line for a whitespace boundary and return the index
 * of the last character before (right of) the space.  If the
 * current character is immediately right of whitespace, skip
 * through the initial whitespace.
 */
static unsigned int	mevcli_search_word_left(mevcli_ctx_t *ctx)
{
	bool saw_real_char = false;
	if (ctx->cursorpos > 0) {
		for (unsigned int i = ctx->cursorpos; i > 0; i--) {
			if (ctx->line[i - 1] <= ' ') {
				if (saw_real_char)
					return i;
			} else {
				saw_real_char = true;
			}
		}
		/* No gap?  Fall through and return start of string */
	}
	return 0;
}

/* As above, but search right (up) and return the index of the
 * first space.	 If the current cursor position is in whitespace,
 * skip over it first.
 */
static unsigned int	mevcli_search_word_right(mevcli_ctx_t *ctx)
{
	bool saw_real_char = false;
	if (ctx->cursorpos < ctx->linepos) {
		for (unsigned int i = ctx->cursorpos; i < ctx->linepos; i++) {
			if (ctx->line[i] <= ' ') {
				if (saw_real_char)
					return i;
			} else {
				saw_real_char = true;
			}
		}
		/* No gap?  Fall through and return end of string */
	}
	return ctx->linepos;
}

static void	mevcli_cursor_right_word(mevcli_ctx_t *ctx)
{
	ctx->cursorpos = mevcli_search_word_right(ctx);
	mevcli_ansi_cursorpos(ctx, ctx->cursorpos + ctx->prompt_len);
}

static void	mevcli_cursor_left_word(mevcli_ctx_t *ctx)
{
	ctx->cursorpos = mevcli_search_word_left(ctx);
	mevcli_ansi_cursorpos(ctx, ctx->cursorpos + ctx->prompt_len);
}

static void	mevcli_cursor_start(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos > 0) {
		ctx->cursorpos = 0;
		mevcli_ansi_cursorpos(ctx, ctx->prompt_len);
	}
}

static void	mevcli_cursor_end(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos < ctx->linepos) {
		ctx->cursorpos = ctx->linepos;
		mevcli_ansi_cursorpos(ctx, ctx->cursorpos + ctx->prompt_len);
	}
}


///////////////////////// Editing/insertion ////////////////////////////////////

/* Delete chars from cursor down to pos, which might be left a char, a
 * word, or all the way to zero.
 */
static void	mevcli_cut_down_to(mevcli_ctx_t *ctx, unsigned int pos)
{
	MEVCLI_ASSERT(pos <= ctx->cursorpos);
	unsigned int distance = ctx->cursorpos - pos;

	if (ctx->cursorpos == ctx->linepos) {
		ctx->cursorpos = ctx->linepos = pos;
		if (distance == 1) {
			/* Shortcut for common case of cursor at end
			 * of line, deleting one char: Noddy 'rubout'
			 * by backspace-overwrite-backspace'ing:
			 */
			mevcli_putstr(ctx, "\b \b");
			return;
		}
	} else {
		for (unsigned int i = ctx->cursorpos; i < ctx->linepos; i++) {
			ctx->line[pos + i - ctx->cursorpos] = ctx->line[i];
		}
		ctx->linepos -= distance;
		ctx->cursorpos = pos;
	}

	/* Move cursor to pos, erase rightward, redraw, put cursor back */
	mevcli_ansi_cursorpos(ctx, ctx->prompt_len + ctx->cursorpos);
	mevcli_ansi_eraseright(ctx);
	for (unsigned int i = ctx->cursorpos; i < ctx->linepos; i++) {
		mevcli_putch(ctx, ctx->line[i]);
	}
	mevcli_ansi_cursorpos(ctx, ctx->prompt_len + ctx->cursorpos);
}

/* Regular user-hits-delete, take one char off at cursor pos (if there
 * are any chars there!)
 */
static void	mevcli_char_delete(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos > 0)
		mevcli_cut_down_to(ctx, ctx->cursorpos - 1);
}

/* Cut the line from the cursor leftwards to the beginning */
static void	mevcli_cut_start(mevcli_ctx_t *ctx)
{
	mevcli_cut_down_to(ctx, 0);
}

/* Cut the line from the cursor leftwards one word */
static void	mevcli_cut_word(mevcli_ctx_t *ctx)
{
	mevcli_cut_down_to(ctx, mevcli_search_word_left(ctx));
}

/* Delete rightwards until end of line */
static void	mevcli_cut_end(mevcli_ctx_t *ctx)
{
	if (ctx->cursorpos < ctx->linepos) {
		mevcli_ansi_eraseright(ctx);
		ctx->linepos = ctx->cursorpos;
	}
}

/* Regular user input at cursor; appends if cursor at end of
 * string, else make a gap, insert, and redraw.
 */
static void	mevcli_char_insert(mevcli_ctx_t *ctx, char in)
{
	if (ctx->linepos >= MEVCLI_MAX_LINE_LEN) {
		/* No room, soz */
		mevcli_putch(ctx, MEVCLI_BELL_CHAR); /* BOOP! */
		return;
	}

	if (ctx->cursorpos == ctx->linepos) {
		/* Simple, common case: append at end of line */
		ctx->line[ctx->linepos++] = in;
		ctx->cursorpos++;
		mevcli_putch(ctx, in);
	} else {
		/* Extend line; copy (backwards!) from cursor upwards */
		ctx->linepos++;
		for (int i = ctx->linepos; i > ctx->cursorpos; i--) {
			ctx->line[i] = ctx->line[i - 1];
		}
		/* Insert char, move cursor */
		ctx->line[ctx->cursorpos++] = in;
		/* Redraw from cursor-1 up */
		mevcli_ansi_cursorpos(ctx, ctx->prompt_len + ctx->cursorpos - 1);
		mevcli_ansi_eraseright(ctx);	/* belt and braces, not technically needed */
		for (unsigned int i = ctx->cursorpos - 1; i < ctx->linepos; i++) {
			mevcli_putch(ctx, ctx->line[i]);
		}
		mevcli_ansi_cursorpos(ctx, ctx->prompt_len + ctx->cursorpos);
	}
}


///////////////////////// Input processing /////////////////////////////////////

/* Check input chars against escape/CSI tracking.  Returns true if
 * handled.
 */
static bool	mevcli_process_esc_seq(mevcli_ctx_t *ctx, char in)
{
	/* Looking for CSI sequence, mainly to spot cursor
	 * keys.  Bomb out of the FSM if anything gets tricky.
	 */

	bool ret = false;

	switch (ctx->csi_fsm_state) {
	case 0:
		if (in == '\e') {
			ctx->csi_fsm_state = 1;
			ret = true;
		}
		break;

	case 1: /* Saw ESC; did we get full CSI ^[[? */
		ctx->csi_fsm_state = 0;
		ret = true;

		switch (in) {
		case '[':
			ctx->csi_fsm_state = 2;
			break;

		case 'b':
			/* Some terminals send CTRL-left/CTRL-right as
			 * ESC-b, ESC-f.  We implement this as "move
			 * word left/right".
			 */
			mevcli_cursor_left_word(ctx);
			break;

		case 'f':
			mevcli_cursor_right_word(ctx);
			break;

		default:
			/* Unexpected, escape-somethingweird.  Ignore
			 * the escaped char
			 */
			break;
		}
		break;

	default: /* 2; Saw full CSI */
		switch (in) {
		case 'A':
			mevcli_cursor_up(ctx);
			break;
		case 'B':
			mevcli_cursor_down(ctx);
			break;
		case 'C':
			mevcli_cursor_right(ctx);
			break;
		case 'D':
			mevcli_cursor_left(ctx);
			break;
		}
		/* FIXME: Could support ESC[3~ for delete char right.*/

		ctx->csi_fsm_state = 0;
		ret = true;
	}
	return ret;
}


///////////////////////// External API /////////////////////////////////////////

void	mevcli_init(mevcli_ctx_t *ctx, const mevcli_cmd_t *cmds, unsigned int num_cmds,
		    void (*cb_output_char)(char out))
{
	ctx->commands = cmds;
	ctx->num_commands = num_cmds;
	ctx->cb_output_char = cb_output_char;
	ctx->csi_fsm_state = 0;
	ctx->cursorpos = ctx->linepos = 0;

	mevcli_prompt(ctx);
}

void	mevcli_input_char(mevcli_ctx_t *ctx, const char in)
{
	/* Process Emacs/bash-like basics in navigation and editing.
	 * Delete, left/right cursors, and
	 *
	 * ^W:	Cut word back from cursor
	 * ^U:	Clear to start of line from cursor
	 *	(Paste unsupported)
	 * ^A:	Go to start of line
	 * ^E:	Go to end of line
	 */
#ifdef _MEVCLI_DEBUG_INPUT
	printf("(%x)", in);
#endif

	/* Special case for escape sequence handling:
	 * if it's an escape, or we're tracking a CSI sequence,
	 * drop out of regular handling.
	 */
	if (mevcli_process_esc_seq(ctx, in))
		return;

	/* Regular handling resumes */
	switch (in) {
	case '\t':
		/* FIXME */
		break;

	case '\r':
		mevcli_process_cmd(ctx);
		break;

	case '\x7f': /* DEL */
		mevcli_char_delete(ctx);
		break;

	case '\x01': /* ^A */
		mevcli_cursor_start(ctx);
		break;

	case '\x05': /* ^E */
		mevcli_cursor_end(ctx);
		break;

	case '\x15': /* ^U */
		mevcli_cut_start(ctx);
		break;

	case '\x17': /* ^W */
		mevcli_cut_word(ctx);
		break;

	case '\x0b': /* ^K */
		mevcli_cut_end(ctx);
		break;

		/* Other control characters are ignored, below */

	default:
		if (in < ' ' || in > 126)
			break;
		mevcli_char_insert(ctx, in);
	}
}

#endif
