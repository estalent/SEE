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
# include <stdarg.h>
#endif

#include <see/mem.h>
#include <see/type.h>
#include <see/string.h>
#include <see/object.h>
#include <see/error.h>
#include <see/interpreter.h>
#include <see/value.h>

#include "stringdefs.h"

static struct SEE_stringclass fixed_stringclass = {
	0						/* growto */
};

/*
 * Strings.
 *
 * Strings are arrays of 16-bit characters with UTF-16 encoding.
 * Because the ECMAScript standard never needs the strings
 * interpreted in their full unicode form, (such as UCS-4),
 * the implementation here maintains them as an array of 16-bit 
 * unsigned integers.
 */

/*
 * Grow a string's data[] array to the given length
 */
static void
growto(s, minspace)
	struct SEE_string *s;
	unsigned int minspace;
{
	if (!s->stringclass || !s->stringclass->growto)
		SEE_error_throw_string(s->interpreter, s->interpreter->Error, 
			STR(no_string_space));
	(*s->stringclass->growto)(s, minspace);
}

/*
 * Copy a string
 */
struct SEE_string *
SEE_string_dup(interp, s)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
{
	struct SEE_string *cp;

	cp = SEE_string_new(interp, s->length);
	SEE_string_append(cp, s);
	return cp;
}

/*
 * Copy and return a substring.
 * Raises an error if the substring is out of bounds.
 */
struct SEE_string *
SEE_string_substr(interp, s, start, len)
	struct SEE_interpreter *interp;
	struct SEE_string *s;
	int start, len;
{
	struct SEE_string *subs;

	if (start < 0 
	 || len < 0 
	 || start + len > s->length)
		SEE_error_throw_string(interp, interp->Error, STR(bad_arg));

	subs = SEE_NEW(interp, struct SEE_string);
	subs->length = len;
	subs->data = s->data + start;
	subs->interpreter = interp;
	subs->flags = 0;
	subs->stringclass = &fixed_stringclass;
	return subs;
}

/*
 * Compare two strings. Returns
 *	-1 if a < b
 *	 0 if a = b
 *	+1 if a > b
 */
int
SEE_string_cmp(a, b)
	const struct SEE_string *a, *b;
{
	const SEE_char_t *ap, *bp;
	unsigned int alen, blen;

	if (a == b)
		return 0;

	ap = a->data; alen = a->length;
	bp = b->data; blen = b->length;

	while (alen && blen && *ap == *bp) {
		alen--;
		blen--;
		ap++;
		bp++;
	}
	if (!alen) {
		if (!blen)
			return 0;
		return -1;
	}
	if (!blen)
		return +1;
	return (*ap < *bp) ? -1 : 1;
}

/*
 * Append character c to the end of string s.
 * MODIFIES 's'!
 */
void
SEE_string_addch(s, c)
	struct SEE_string *s;
	SEE_char_t c;
{
	growto(s, s->length + 1);
	s->data[s->length++] = c;
}

/*
 * Append string t to the end of string s.
 * MODIFIES 's'!
 */
void
SEE_string_append(s, t)
	struct SEE_string *s;
	const struct SEE_string *t;
{
	growto(s, s->length + t->length);
	if (t->length) {
	    memcpy(s->data + s->length, t->data, 
		t->length * sizeof (SEE_char_t));
	    s->length += t->length;
	}
}

/*
 * Append an integer onto the end of string s
 * MODIFIES 's'!
 */
void
SEE_string_append_int(s, i)
	struct SEE_string *s;
	int i;
{
	if (i < 0) {
		i = -i;
		SEE_string_addch(s, '-');
	}
	if (i >= 10)
		SEE_string_append_int(s, i / 10);
	SEE_string_addch(s, (i % 10) + '0');
}

/*
 * Concatenate two strings together and return the resulting string.
 * May return one of the original strings or a new string.
 */
struct SEE_string *
SEE_string_concat(interp, a, b)
	struct SEE_interpreter *interp;
	struct SEE_string *a, *b;
{
	struct SEE_string *s;

	if (a->length == 0)
		return b;
	if (b->length == 0)
		return a;

	s = SEE_string_new(interp, a->length + b->length);
	if (a->length)
		memcpy(s->data, a->data, a->length * sizeof (SEE_char_t));
	if (b->length)
		memcpy(s->data + a->length, b->data, 
		    b->length * sizeof (SEE_char_t));
	s->length = a->length + b->length;
	return s;
}

/* 
 * Convert a UTF-16 string to UTF-8 and write to a stdio file.
 * Ref: RFC2279, RFC2781
 */
void
SEE_string_fputs(s, f)
	const struct SEE_string *s;
	FILE *f;
{
	int i;
	SEE_char_t ch, ch2;
	struct SEE_interpreter *interp = s->interpreter;

	for (i = 0; i < s->length; i++) {
		ch = s->data[i];
		if ((ch & 0xff80) == 0) 
		    fputc(ch & 0x7f, f);
		else if ((ch & 0xf800) == 0) {
		    fputc(0xc0 | ((ch >> 6) & 0x1f), f);
		    fputc(0x80 | (ch & 0x3f), f);
		} else if ((ch & 0xfc00) != 0xd800) {
		    fputc(0xe0 | ((ch >> 12) & 0x0f), f);
		    fputc(0x80 | ((ch >> 6) & 0x3f), f);
		    fputc(0x80 | (ch & 0x3f), f);
		} else {
		    if (i == s->length - 1)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch2 = s->data[++i];
		    if ((ch2 & 0xfc00) != 0xdc00)
			SEE_error_throw_string(interp, interp->Error, 
				STR(bad_utf16_string));
		    ch = (ch & 0x03ff) + 0x0040;
		    fputc(0xf0 | ((ch >> 8) & 0x07), f);
		    fputc(0x80 | ((ch >> 2) & 0x3f), f);
		    fputc(0x80 | ((ch & 0x3) << 4) |
				 ((ch2 & 0x03c0) >> 6), f);
		    fputc(0x80 | (ch2 & 0x3f), f);
		}
	}
}

/*
 * sprintf-like interface to new strings.
 * Assumes the format yields 7-bit ASCII (and not UTF-8!).
 * (Also assumes a va_list can be passed multiple times to vsnprintf!)
 */
struct SEE_string *
SEE_string_vsprintf(interp, fmt, ap)
	struct SEE_interpreter *interp;
	const char *fmt;
	va_list ap;
{
	char *buf = NULL;
	int len;
	struct SEE_string *str;
	SEE_char_t *p;

        len = vsnprintf(buf, 0, fmt, ap);
	if (len) {
		buf = SEE_ALLOCA(len, char);
		vsnprintf(buf, len, fmt, ap);
	}

	str = SEE_string_new(interp, len);
	str->length = len;
	for (p = str->data; len--; p++, buf++)
	    *p = *(unsigned char *)buf & 0x007f;

	return str;
}

struct SEE_string *
SEE_string_sprintf(struct SEE_interpreter *interp, const char *fmt, ...)
{
	va_list ap;
	struct SEE_string *s;

	va_start(ap, fmt);
	s = SEE_string_vsprintf(interp, fmt, ap);
	va_end(ap);
	return s;
}

/*------------------------------------------------------------
 * The simple string class
 */
struct simple_string {
	struct SEE_string string;
	unsigned int space;
};

/* 
 * grows the string to have the required space in its array.
 * Simple strings never shrink. Grows in powers of two, starting at 256.
 */
static void
simple_growto(s, minspace)
	struct SEE_string *s;
	unsigned int minspace;
{
	struct simple_string *ss = (struct simple_string *)s;
	int new_space;
	SEE_char_t *new_data;

	if (ss->space < minspace) {
	    new_space = ss->space ? ss->space * 2 : 256;
	    while (new_space < minspace)
		new_space *= 2;
	    new_data = SEE_NEW_ARRAY(s->interpreter, SEE_char_t, new_space);
	    if (s->length)
		memcpy(new_data, s->data, s->length * sizeof (SEE_char_t));
	    ss->string.data = new_data;
            ss->space = new_space;
	}
}

static struct SEE_stringclass simple_stringclass = {
	simple_growto					/* growto */
};

struct SEE_string *
SEE_string_new(interp, space)
	struct SEE_interpreter *interp;
	unsigned int space;
{
	struct simple_string *ss = SEE_NEW(interp, struct simple_string);

	ss->string.length = 0;
	ss->string.data = NULL;
	ss->string.interpreter = interp;
	ss->string.flags = 0;
	ss->space = 0;
	ss->string.stringclass = &simple_stringclass;
	if (space)
	    simple_growto((struct SEE_string *)ss, space);
	return (struct SEE_string *)ss;
}

/* Return a fully escaped string literal, suitable for lexical analysis */
struct SEE_string *
SEE_string_literal(interp, s)
	struct SEE_interpreter *interp;
	const struct SEE_string *s;
{
	struct SEE_string *lit;
	int i;
	SEE_char_t c;

	if (s == NULL)
		return NULL;

	lit = SEE_string_new(interp, 0);
	SEE_string_addch(lit, '\"');
	for (i = 0; i < s->length; i++) {
	    c = s->data[i];
	    switch (c) {
	    case 0x0008:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'b');
				break;
	    case 0x0009:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 't');
				break;
	    case 0x000a:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'n');
				break;
	    case 0x000b:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'v');
				break;
	    case 0x000c:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'f');
				break;
	    case 0x000d:	SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, 'r');
				break;
	    case '\\':
	    case '\"':		SEE_string_addch(lit, '\\');
				SEE_string_addch(lit, c);
				break;
	    default:
		if (c >= 0x20 && c < 0x7f)
			SEE_string_addch(lit, c);
		else if (c < 0x100) {
			SEE_string_addch(lit, '\\');
			SEE_string_addch(lit, 'x');
			SEE_string_addch(lit, SEE_hexstr[(c >> 4) & 0xf]);
			SEE_string_addch(lit, SEE_hexstr[c & 0xf]);
		} else {
			SEE_string_addch(lit, '\\');
			SEE_string_addch(lit, 'u');
			SEE_string_addch(lit, SEE_hexstr[(c >> 12) & 0xf]);
			SEE_string_addch(lit, SEE_hexstr[(c >> 8) & 0xf]);
			SEE_string_addch(lit, SEE_hexstr[(c >> 4) & 0xf]);
			SEE_string_addch(lit, SEE_hexstr[c & 0xf]);
		}
	    }
	}
	SEE_string_addch(lit, '\"');
	return lit;
}
