/*
 * Copyright (c) 2003
 *      David Leonard.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by David Leonard and 
 *      contributors.
 * 4. Neither the name of Mr Leonard nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY DAVID LEONARD AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL DAVID LEONARD OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* $Id$ */

#if HAVE_CONFIG_H
# include <see/config.h>
#endif

#if STDC_HEADERS
# include <stdio.h>
# include <stdlib.h>
#endif

#include <see/type.h>
#include <see/value.h>
#include <see/try.h>
#include <see/object.h>
#include <see/string.h>
#include <see/interpreter.h>

#include "stringdefs.h"

/*
 * Exception handling
 *
 * This implementation of exceptions uses the C stack and
 * C's setjmp/longjmp to implement exceptions. Because the
 * SEE evaluation/execution engine uses the C stack, these
 * exceptions can be raised and caught by native code.
 *
 * NOTE: I had considered using a special-return mechanism
 * for exceptions, but the complexity, performance and annoyance 
 * of the vast number of 'return checking' required and the
 * small number of expected actual exceptions during normal
 * operation made this mechanism far more favourable. Also,
 * setjmp/longjmp is part of standard POSIX and ANSI C.
 */

/* 
 * This is a pointer to the currently active 
 * 'try' context (from the last use of SEE_TRY()).
 */
volatile struct SEE_try_context * volatile SEE_try_current_context;
struct SEE_throw_location * volatile SEE_try_current_location;

/*
 * Abort on an uncatchable exception.
 *
 * This can happen when a SEE_THROW occurs outside of any SEE_TRY context,
 * or when an error occurs during low-level throw handling. (See 
 * SEE_error_throw(), for instance).
 */
void
SEE_throw_abort(interp, file, line)
	struct SEE_interpreter *interp;
	const char *file;
	int line;
{
#ifndef NDEBUG
	fprintf(stderr, "%s:%d: uncatchable exception\n", file, line);
#endif
	(*SEE_abort)(interp, "exception thrown but no TRY block");
}

/* Return a location prefix string in the form "program.js:23: " */
struct SEE_string *
SEE_location_string(interp, loc)
	struct SEE_interpreter *interp;
	struct SEE_throw_location *loc;
{
	struct SEE_string *s;

	s = SEE_string_new(interp, 0);
	if (loc == NULL)
		return s;
	SEE_string_append(s, loc->filename ? loc->filename 
					   : STR(unknown_file));
	SEE_string_addch(s, ':');
	SEE_string_append_int(s, loc->lineno);
	SEE_string_addch(s, ':');
	SEE_string_addch(s, ' ');
	return s;
}

#undef SEE_throw
void
SEE_throw()
{
	/*
	 * This function exists soley for debuggers to break on,
	 * since the SEE_THROW macro calls longjmp directly (in order to
	 * give the compiler a chance at making safer code.)
	 */
}

void
longjmperror()
{
	(*SEE_abort)(NULL, "longjmp error");
}
