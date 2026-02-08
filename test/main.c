/* Example showing how to use mevcli
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

#include <assert.h>
#include <ctype.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

/* Override some default before including mevcli.h: */
#define MEVCLI_PROMPT   	_prompt
#define MEVCLI_ASSERT(x)	assert(x)
#define MEVCLI_EXTRA_HELPSTRING	"\r\n" \
	"\t[ You can navigate a line using cursors (use them with CTRL\r\n" \
	"\t  to navigate by word), and ^A/^E to skip to the start/end.\r\n" \
	"\t  Erase by word (^W), or to line start (^U) are also supported. ]\r\n"

static char _prompt[128];
static bool _quit = false;

#include "mevcli.h"



static void cmd_pback(void *opaque, int argc, char **argv)
{
	printf("Got %d args.", argc);
	if (argc > 0) {
		printf("  In reverse order, they are: ");
		for (int i = argc - 1; i >= 0; i--) {
			printf("'%s' ", argv[i]);
		}
	}
	printf("\r\n");
}

static void cmd_pcaps(void *opaque, int argc, char **argv)
{
	for (int i = 0; i < argc; i++) {
		char *a = argv[i];

		printf(" '");
		while (*a) {
			putchar(toupper(*(a++)));
		}
		printf("'");
	}
	printf("\r\n");
}

static void cmd_quit(void *opaque, int argc, char **argv)
{
	_quit = true;
}

/* This command uses the 'opaque' parameter to use a common
 * handler for >1 command, and differentiate invocations.
 */
static void cmd_special(void *opaque, int argc, char **argv)
{
	if ((uintptr_t)opaque != 0)
		strcpy(_prompt, "specialmode> ");
	else
		strcpy(_prompt, "test> ");
}

const mevcli_cmd_t cmds[] = {
	/* Two similar commands to demonstrate tab-completion */
	{ .name = "prback",
	  .help = " <args...>\tPrint args backwards",
	  .cmdfn = cmd_pback,
	  .nargs = -1,
	},
	{ .name = "prcaps",
	  .help = " <a> <b>\t\tPrint both args IN CAPS",
	  .cmdfn = cmd_pcaps,
	  .nargs = 2,
	},
	{ .name = "special",
	  .help = "\t\t\tEnter special mode",
	  .cmdfn = cmd_special,
	  .opaque = (void *)1,
	},
	{ .name = "unspecial",
	  .help = "\t\tExit special mode",
	  .cmdfn = cmd_special,
	  .opaque = (void *)0,
	},
	{ .name = "quit",
	  .help = "\t\t\tQuit back to sanity",
	  .cmdfn = cmd_quit
	},
};

static void my_putchar(char c)
{
	write(1, &c, 1);
}

/* All of the line-editing storage/state lives here: */
static mevcli_ctx_t mcctx;

int main(int argc, char *argv[])
{
	/* Let's futz with termio to make things rawwwww */
	struct termios tios;
	tcgetattr(0, &tios);
	struct termios tios_new = tios;
	cfmakeraw(&tios_new);
	tcsetattr(0, TCSANOW, &tios_new);

	/* In this example, the prompt is configured to
	 * come from the _prompt array; it is therefore dynamic,
	 * and can be affected by commands.  See cmd_special()
	 */
	strcpy(_prompt, "test> ");

	mevcli_init(&mcctx,
		    cmds,
		    sizeof(cmds)/sizeof(mevcli_cmd_t),
		    my_putchar);

	/* Process input */
	while (!_quit) {
		struct pollfd pfd = {
			.fd = 0, .events = POLLIN
		};

		int r = poll(&pfd, 1, -1);

		if ((r == 1) && (pfd.revents & POLLIN)) {
			char c;
			read(0, &c, 1);

			if (c == '\x03')	/* intr */
				break;

			mevcli_input_char(&mcctx, c);

		} else if (r < 0) {
			break;
		}
	}

	tcsetattr(0, TCSANOW, &tios);

	return 0;
}
