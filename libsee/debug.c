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
#endif

#include <see/try.h>
#include <see/value.h>
#include <see/object.h>
#include <see/string.h>
#include <see/debug.h>
#include <see/interpreter.h>

/*
 * Print the contents of a value, without raising an exception
 */
void
SEE_PrintValue(interp, v, f)
	struct SEE_interpreter *interp;
	struct SEE_value *v;
	FILE *f;
{
	int i;

	if (v == NULL) {
	    fprintf(f, "NULL");
	    return;
	}
	switch (v->type) {
	case SEE_UNDEFINED:
	    fprintf(f, "undefined");
	    break;
	case SEE_NULL:
	    fprintf(f, "null");
	    break;
	case SEE_BOOLEAN:
	    fprintf(f, v->u.boolean ? "true" : "false");
	    break;
	case SEE_NUMBER:
	    fprintf(f, "%.30g", v->u.number);
	    break;
	case SEE_STRING:
	    /* NB Replicates most of SEE_string_literal(). Yuck */
	    fprintf(f, "\"");
	    for (i = 0; i < v->u.string->length; i++) {
		SEE_char_t c = v->u.string->data[i];
		if (c == '\\') fprintf(f, "\\\\");
		else if (c == '\"') fprintf(f, "\\\"");
		else if (c == '\n') fprintf(f, "\\n");
		else if (c == '\t') fprintf(f, "\\t");
		else if (c >= ' ' && c <= '~')
			fputc(c & 0x7f, f);
		else if (c < 0x100)
			fprintf(f, "\\x%02x", c);
		else
			fprintf(f, "\\u%04x", c);
	    }
	    fprintf(f, "\"");
	    break;
	case SEE_OBJECT:
	    SEE_PrintObject(interp, v->u.object, f);
	    break;
	case SEE_REFERENCE:
	    fprintf(f, "<ref base=<object %p> prop=", v->u.reference.base);
	    SEE_string_fputs(v->u.reference.property, f);
	    fprintf(f, ">");
	    break;
	case SEE_COMPLETION:
	    switch (v->u.completion.type) {
	    case SEE_NORMAL:
		fprintf(f, "<normal");
		if (v->u.completion.value) {
		    fprintf(f, " ");
		    SEE_PrintValue(interp, v->u.completion.value, f);
		}
		fprintf(f, ">");
		break;
	    case SEE_BREAK:
		fprintf(f, "<break");
		if (v->u.completion.target != (void *)1)
			fprintf(f, " %p", v->u.completion.target);
		fprintf(f, ">");
		break;
	    case SEE_CONTINUE:
		fprintf(f, "<continue");
		if (v->u.completion.target != (void *)2)
			fprintf(f, " %p", v->u.completion.target);
		fprintf(f, ">");
		break;
	    case SEE_RETURN:
		fprintf(f, "<return ");
		SEE_PrintValue(interp, v->u.completion.value, f);
		fprintf(f, ">");
		break;
	    case SEE_THROW:
		fprintf(f, "<throw ");
		SEE_PrintValue(interp, v->u.completion.value, f);
		fprintf(f, ">");
		break;
	    default:
		fprintf(f, "<BAD completion %d>", v->u.completion.type);
	    }
	    break;
	default:
	    fprintf(f, "<BAD value %d>", v->type);
	}
}

/*
 * Print an object without raising an exception.
 * The object's class is shown in quotes.
 * If the object is known to the interpreter (eg Object, Array.prototype, etc.)
 * then its original name is shown in parentheses.
 */
void
SEE_PrintObject(interp, o, f)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	FILE *f;
{
	const char *known;

	if (o == NULL)				known = "NULL";
	else if (interp == NULL)		known = NULL;
	else if (o == interp->Global)		known = "Global";
	else if (o == interp->Object)		known = "Object";
	else if (o == interp->Object_prototype)	known = "Object.prototype";
	else if (o == interp->Error)		known = "Error";
	else if (o == interp->EvalError)	known = "EvalError";
	else if (o == interp->RangeError)	known = "RangeError";
	else if (o == interp->ReferenceError)	known = "ReferenceError";
	else if (o == interp->SyntaxError)	known = "SyntaxError";
	else if (o == interp->TypeError)	known = "TypeError";
	else if (o == interp->URIError)		known = "URIError";
	else if (o == interp->String)		known = "String";
	else if (o == interp->String_prototype)	known = "String.prototype";
	else if (o == interp->Function)		known = "Function";
	else if (o == interp->Function_prototype)known = "Function.prototype";
	else if (o == interp->Array)		known = "Array";
	else if (o == interp->Array_prototype)	known = "Array.prototype";
	else if (o == interp->Number)		known = "Number";
	else if (o == interp->Number_prototype)	known = "Number.prototype";
	else if (o == interp->Boolean)		known = "Boolean";
	else if (o == interp->Boolean_prototype)known = "Boolean.prototype";
	else if (o == interp->Math)		known = "Math";
	else if (o == interp->RegExp)		known = "RegExp";
	else if (o == interp->RegExp_prototype)	known = "RegExp.prototype";
	else if (o == interp->Date)		known = "Date";
	else if (o == interp->Date_prototype)	known = "Date.prototype";
	else					known = NULL;

	fprintf(f, "<object %p", o);
	if (known)
		fprintf(f, " (%s)", known);
	if (o && o->objectclass && !known) {
		fprintf(f, " \"");
		SEE_string_fputs(o->objectclass->Class, f);
		fprintf(f, "\"");
	}
	fprintf(f, ">");
}

/*
 * Print a string, in 'literal' form to the given stdio file.
 */
void
SEE_PrintString(interp, s, f)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	FILE *f;
{
	SEE_string_fputs(SEE_string_literal(interp, s), f);
	fprintf(f, "<%s%s%p>", 
		s->flags & SEE_STRING_FLAG_INTERNED ? "." : "",
		s->flags & SEE_STRING_FLAG_STATIC ? "@" : "",
		s);
}
