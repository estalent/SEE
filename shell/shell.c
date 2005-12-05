/*
 * SEE shell environment objects
 *
 * The shell provides a couple of objects useful for testing.
 *
 *  print	- a function object which takes one
 *		  string argument and prints it, followed
 *		  by a newline.
 *  version	- an undefined value, used to satisfy some tests.
 *
 * In HTML mode the following objects are provided:
 *
 *  document		- a simple Object with some properties:
 *  document.write	- a function to print a string to stdout
 *  document.navigator	- a simple Object used as a placeholder
 *  document.userAgent 	- "SEE-shell"
 *  document.window	- a reference to the Global object
 *
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
#endif

#include <see/see.h>
#include "shell.h"

static void print_fn(struct SEE_interpreter *,
        struct SEE_object *, struct SEE_object *, int,
	struct SEE_value **, struct SEE_value *);

/* Internalised constant strings for my convenience */
static struct SEE_string *s_print;
static struct SEE_string *s_version;
static struct SEE_string *s_document;
static struct SEE_string *s_write;
static struct SEE_string *s_navigator;
static struct SEE_string *s_userAgent;
static struct SEE_string *s_window;
static struct SEE_string *s_gc_dump;
struct SEE_string *s_interactive;

/*
 * Adds useful symbols into the interpreter's internal symbol table. 
 * This speeds up access to the symbols and lets us use UTF-16 strings
 * natively when populating the object space.
 */
void
shell_strings()
{
	static SEE_char_t SA_print[] =    {'p','r','i','n','t'};
	static struct SEE_string S_print = SEE_STRING_DECL(SA_print);
	static SEE_char_t SA_version[] =  {'v','e','r','s','i','o','n'};
	static struct SEE_string S_version = SEE_STRING_DECL(SA_version);
	static SEE_char_t SA_document[] = {'d','o','c','u','m','e','n','t'};
	static struct SEE_string S_document = SEE_STRING_DECL(SA_document);
	static SEE_char_t SA_write[] =    {'w','r','i','t','e'};
	static struct SEE_string S_write = SEE_STRING_DECL(SA_write);
	static SEE_char_t SA_navigator[] ={'n','a','v','i','g','a','t','o','r'};
	static struct SEE_string S_navigator = SEE_STRING_DECL(SA_navigator);
	static SEE_char_t SA_userAgent[] ={'u','s','e','r','A','g','e','n','t'};
	static struct SEE_string S_userAgent = SEE_STRING_DECL(SA_userAgent);
	static SEE_char_t SA_window[] =   {'w','i','n','d','o','w'};
	static struct SEE_string S_window = SEE_STRING_DECL(SA_window);
	static SEE_char_t SA_gc_dump[] =   {'g','c','_','d','u','m','p'};
	static struct SEE_string S_gc_dump = SEE_STRING_DECL(SA_gc_dump);
	static SEE_char_t SA_interactive[] = {'<','i','n','t','e','r','a','c','t','i','v','e','>'};
	static struct SEE_string S_interactive = SEE_STRING_DECL(SA_interactive);

	SEE_intern_global(s_print = &S_print);
	SEE_intern_global(s_version = &S_version);
	SEE_intern_global(s_document = &S_document);
	SEE_intern_global(s_write = &S_write);
	SEE_intern_global(s_navigator = &S_navigator);
	SEE_intern_global(s_userAgent = &S_userAgent);
	SEE_intern_global(s_window = &S_window);
	SEE_intern_global(s_gc_dump = &S_gc_dump);
	SEE_intern_global(s_interactive = &S_interactive);
}

/*
 * A print function that prints a string argument to stdout.
 * A newline is printed at the end.
 */
static void
print_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;

        if (argc) {
                SEE_ToString(interp, argv[0], &v);
                SEE_string_fputs(v.u.string, stdout);
        }
	printf("\n");
	fflush(stdout);
        SEE_SET_UNDEFINED(res);
}

#if HAVE_GC_DUMP
/*
 * Dump the garbage collector.
 */
static void
gc_dump_fn(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;

        if (argc) {
                SEE_ToString(interp, argv[0], &v);
                SEE_string_fputs(v.u.string, stdout);
        }
	gc_dump();
        SEE_SET_UNDEFINED(res);
}
#endif

/*
 * Adds global symbols 'print' and 'version' to the interpreter.
 * 'print' is a function and 'version' will be the undefined value.
 */
void
shell_add_globals(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *obj;
	struct SEE_value v;

	/* Create the print function, and attch to the Globals */
	obj = SEE_cfunction_make(interp, print_fn, s_print, 1);
	SEE_SET_OBJECT(&v, obj);
	SEE_OBJECT_PUT(interp, interp->Global, s_print, &v, 0);

#if HAVE_GC_DUMP
	/* Create the gc_dump function, and attach to the Globals. */
	obj = SEE_cfunction_make(interp, gc_dump_fn, s_gc_dump, 1);
	SEE_SET_OBJECT(&v, obj);
	SEE_OBJECT_PUT(interp, interp->Global, s_gc_dump, &v, 0);
#endif

	SEE_SET_UNDEFINED(&v);
	SEE_OBJECT_PUT(interp, interp->Global, s_version, &v, 0);
}

/*
 * A write function that prints its output to stdout.
 * No newline is appended.
 */
static void
document_write(interp, self, thisobj, argc, argv, res)
        struct SEE_interpreter *interp;
        struct SEE_object *self, *thisobj;
        int argc;
        struct SEE_value **argv, *res;
{
        struct SEE_value v;

        if (argc) {
                SEE_ToString(interp, argv[0], &v);
                SEE_string_fputs(v.u.string, stdout);
		fflush(stdout);
        }
        SEE_SET_UNDEFINED(res);
}

/*
 * Adds the following variables to emulate a browser environment:
 *   document             - dummy object
 *   document.write       - function to print strings
 *   navigator            - dummy object
 *   navigator.userAgent  - dummy string identifier for the SEE shell
 */
void
shell_add_document(interp)
	struct SEE_interpreter *interp;
{
	struct SEE_object *obj, *document, *navigator;
	struct SEE_value v;

	/* Create a dummy 'document' object. Add it to the global space */
	document = SEE_Object_new(interp);
	SEE_SET_OBJECT(&v, document);
	SEE_OBJECT_PUT(interp, interp->Global, s_document, &v, 0);

	/* Create a 'write' method and attach to 'document'. */
	obj = SEE_cfunction_make(interp, document_write, s_write, 1);
	SEE_SET_OBJECT(&v, obj);
	SEE_OBJECT_PUT(interp, document, s_write, &v, 0);

	/* Create a 'navigator' object and attach to the global space */
	navigator = SEE_Object_new(interp);
	SEE_SET_OBJECT(&v, navigator);
	SEE_OBJECT_PUT(interp, interp->Global, s_navigator, &v, 0);

	/* Create a string and attach as 'navigator.userAgent' */
	SEE_SET_STRING(&v, SEE_string_sprintf(interp, 
		"SEE-shell (%s-%s)", PACKAGE, VERSION));
	SEE_OBJECT_PUT(interp, navigator, s_userAgent, &v, 0);

	/* Create a dummy 'window' object */
	SEE_SET_OBJECT(&v, interp->Global);
	SEE_OBJECT_PUT(interp, interp->Global, s_window, &v, 0);
}
