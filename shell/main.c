/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id$ */

/*
 * The SEE shell
 *
 * If no files are given, it prompts for javascript code interactively.
 */

#include <stdio.h>
#include <err.h>
#include <see/see.h>
#include <readline/readline.h>
#include "shell.h"

static int run_input(struct SEE_interpreter *, struct SEE_input *,
	struct SEE_value *);
static int run_file(struct SEE_interpreter *, char *);
static void run_interactive(struct SEE_interpreter *);
static void debug(struct SEE_interpreter *, char);
static void trace(struct SEE_interpreter *, struct SEE_throw_location *);
static void run_html(struct SEE_interpreter *, char *);

/* Enables the debugging flag given by c. */
static void
debug(interp, c)
	struct SEE_interpreter *interp;
	char c;
{
#ifndef NDEBUG
	extern int SEE_native_debug, SEE_Error_debug, 
	SEE_parse_debug, SEE_lex_debug,
	SEE_eval_debug, SEE_error_debug,
	SEE_context_debug, SEE_regex_debug;

	switch (c) {
	case 'n': SEE_native_debug = 1; break;
	case 'E': SEE_Error_debug = 1; break;
	case 'l': SEE_lex_debug = 1; break;
	case 'p': SEE_parse_debug = 1; break;
	case 'v': SEE_eval_debug = 1; break;
	case 'e': SEE_error_debug = 1; break;
	case 'c': SEE_context_debug = 1; break;
	case 'r': SEE_regex_debug = 1; break;
	case 'T': interp->trace = trace; break;
	default:
		warnx("unknown debug flag '%c'", c);
	}
#endif
}

/* Trace function callback: just prints current location to stderr */
static void
trace(interp, loc)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
{
	if (loc) {
	    fprintf(stderr, "trace: ");
	    if (loc->filename) {
		SEE_string_fputs(loc->filename, stderr);
		fprintf(stderr, ", ");
	    }
	    fprintf(stderr, "line %d\n", loc->lineno);
	}
}

/*
 * Runs the input given by inp, printing any exceptions
 * to stderr.
 */
static int
run_input(interp, inp, res)
	struct SEE_interpreter *interp;
	struct SEE_input *inp;
	struct SEE_value *res;
{
	struct SEE_value v;
	SEE_try_context_t ctxt, ctxt2;
	struct SEE_traceback *tb;
	struct SEE_string *locstr;

	interp->traceback = NULL;
        SEE_TRY (interp, ctxt) {
            SEE_Global_eval(interp, inp, res);
        }
        if (SEE_CAUGHT(ctxt)) {
            fprintf(stderr, "exception: ");
            SEE_TRY(interp, ctxt2) {
                SEE_ToString(interp, SEE_CAUGHT(ctxt), &v);
                SEE_string_fputs(v.u.string, stderr);
                fprintf(stderr, "\n");
                if (ctxt.throw_file)
		    fprintf(stderr, "\t(thrown from %s:%d)\n", 
                        ctxt.throw_file, ctxt.throw_line);
		if (interp->traceback) {
		    fprintf(stderr, "traceback:\n");
		    for (tb = interp->traceback; tb; tb = tb->prev) {
			locstr = SEE_location_string(interp, tb->call_location);
		        fprintf(stderr, "\t");
			SEE_string_fputs(locstr, stderr);
		        fprintf(stderr, "\n");
		    }
		}
            }
            if (SEE_CAUGHT(ctxt2)) {
                fprintf(stderr, "exception thrown while "
				"printing exception");
		if (ctxt2.throw_file)
		    fprintf(stderr, " at %s:%d",
		        ctxt2.throw_file, ctxt2.throw_line);
                fprintf(stderr, "\n");
	    }
	    return 0;
        }
        return 1;
}

/*
 * Opens the file and runs the contents as if ECMAScript code.
 */
static int
run_file(interp, filename)
	struct SEE_interpreter *interp;
	char *filename;
{
	struct SEE_input *inp;
	struct SEE_value res;
	FILE *f;
	int ok;

	if (strcmp(filename, "-") == 0) {
		run_interactive(interp);
		return 1;
	}

	f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		return 0;
	}
	inp = SEE_input_file(interp, f, filename, NULL);
	ok = run_input(interp, inp, &res);
	SEE_INPUT_CLOSE(inp);
	return ok;
}

/*
 * Reads a line of text from the user.
 * (A simple replacement for GNU readline)
 */
static char *
_readline(prompt)
	char *prompt;
{
	char buf[1024], *ret;
	int len;

	printf("%s", prompt);
	fflush(stdout);
	ret = fgets(buf, sizeof buf, stdin);
	if (!ret) return NULL;
	len = strlen(ret);
	while (len && (ret[len-1] == '\n' || ret[len-1] == '\r'))
		len--;
	ret[len] = '\0';
	return strdup(ret);
}

/*
 * Reads lines of ECMAscript from the user
 * and runs each one in the same interpreter.
 */
static void
run_interactive(interp)
	struct SEE_interpreter *interp;
{
	char *line;
	struct SEE_input *inp;
	struct SEE_value res;
	int len;

	for (;;) {
	    line = readline("> ");
	    if (line == NULL)
		break;
	    len = strlen(line);
	    while (len && line[len-1] == '\\') {
		char *a, *b;
		a = line;
		b = readline("+ ");
		if (!b) break;
		line = malloc(len + strlen(b) + 1);
		memcpy(line, a, len - 1);
		line[len-1] = '\n';
		strcpy(line + len, b);
		free(a);
		free(b);
		len = strlen(line);
	    }
	    inp = SEE_input_utf8(interp, line);
	    if (run_input(interp, inp, &res)) {
		printf(" = ");
		SEE_PrintValue(interp, &res, stdout);
		printf("\n");
	    }
            SEE_INPUT_CLOSE(inp);
	    free(line);
	}
}

#define toupper(c) 	(((c) >= 'a' && (c) <= 'z') ? (c)-'a'+'A' : (c))

static void
run_html(interp, filename)
	struct SEE_interpreter *interp;
	char *filename;
{
	FILE *f;
	int ch;
	const char *script_start = "<SCRIPT";
	const char *script_end = "</SCRIPT";
	const char *p;
	struct SEE_string *s = SEE_string_new(interp, 0);
	struct SEE_string *filenamestr;
	int endpos;
	int lineno = 1, first_lineno;
	struct SEE_input *inp;

	f = fopen(filename, "r");
	if (!f) {
		perror(filename);
		return;
	}

	filenamestr = SEE_string_sprintf(interp, "%s", filename);

	p = script_start;
	while ((ch = fgetc(f)) != EOF) {
	    if (ch == '\n' || ch == '\r') lineno++;
	    if (toupper(ch) != *p) {
		if (p != script_start)
		    printf("%.*s", p - script_start, script_start);
		p = script_start;
	    }
	    if (toupper(ch) == *p) {
		p++;
		if (!*p) {
		    /* skip to closing > */
		    while ((ch = fgetc(f)) != EOF)  {
		        if (ch == '\n' || ch == '\r') lineno++;
			if (ch == '>') break;
		    }
		    /* capture content up to the end tag */
		    s->length = 0;
		    first_lineno = lineno;
		    p = script_end;
		    endpos = 0;
		    while ((ch = fgetc(f)) != EOF) {
		        if (ch == '\n' || ch == '\r') lineno++;
			if (toupper(ch) != *p) {
			    p = script_end;
			    endpos = s->length;
			}
			if (toupper(ch) == *p) {
			    p++;
			    if (!*p) {
				/* truncate string and skip to closing > */
				s->length = endpos;
				while ((ch = fgetc(f)) != EOF) {
				    if (ch == '\n' || ch == '\r') lineno++;
				    if (ch == '>') break;
				}
				break;
			    }
			}
			SEE_string_addch(s, ch);
		    }

		    inp = SEE_input_string(interp, s);
		    inp->filename = filenamestr;
		    inp->first_lineno = first_lineno;
		    run_input(interp, inp, NULL);

		    p = script_start;
		    continue;
		}
	    } else
	        putchar(ch);
	}
	fclose(f);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct SEE_interpreter interp;
	int ch, error = 0;
	int do_interactive = 1;
	char *s;

	SEE_interpreter_init(&interp);
	shell_add_globals(&interp);
	shell_add_document(&interp);

	while ((ch = getopt(argc, argv, "d:h:f:")) != -1)
	    switch (ch) {
	    case 'd':
		if (*optarg == '*')
		    optarg = "nElpvecr";
		for (s = optarg; *s; s++)
		    debug(&interp, *s);
		break;
	    case 'h':
		do_interactive = 0;
		run_html(&interp, optarg);
		break;
	    case 'f':
		do_interactive = 0;
		if (!run_file(&interp, optarg))
			exit(1);
		break;
	    default:
		error = 1;
	    }
	if (error) {
	    fprintf(stderr, "usage: %s [-d[nElpvecr]] [-f file] [-h file]\n", argv[0]);
	    exit(1);
	}

	if (do_interactive)
	    run_interactive(&interp);

	exit(0);
}
