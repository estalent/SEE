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
 * 3. Neither the name of Mr Leonard nor the names of the contributors
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
# include <config.h>
#endif

#if STDC_HEADERS
# include <stdlib.h>
#endif

#include <see/mem.h>
#include <see/interpreter.h>

#include "dprint.h"

/*
 * This module provides an interface abstraction for the memory
 * allocator. If configured properly, the default allocator is
 * the Boehm-GC collector. An application could see if a default
 * exists by testing (SEE_mem_malloc_hook != NULL).
 */

#ifndef NDEBUG
int SEE_mem_debug = 0;
#undef SEE_malloc
#undef SEE_malloc_string
#endif

static void memory_exhausted(struct SEE_interpreter *) SEE_dead;

#if defined(HAVE_GC_MALLOC)
static void *malloc_gc(struct SEE_interpreter *, SEE_size_t);
static void *malloc_string_gc(struct SEE_interpreter *, SEE_size_t);
# define INITIAL_MALLOC		malloc_gc
# define INITIAL_MALLOC_STRING	malloc_string_gc
# define INITIAL_FREE		NULL
#else
# define INITIAL_MALLOC		NULL	/* application must provide */
# define INITIAL_MALLOC_STRING	NULL
# define INITIAL_FREE		NULL
#endif /* !HAVE_GC_MALLOC */

/*
 * Application-visible memory allocator hooks.
 * Changing these will allow an application to wrap memory allocation
 * and handle out-of-memory conditions.
 */
void * (*SEE_mem_malloc_hook)(struct SEE_interpreter *, SEE_size_t)
		= INITIAL_MALLOC;
void * (*SEE_mem_malloc_string_hook)(struct SEE_interpreter *, SEE_size_t)
		= INITIAL_MALLOC_STRING;
void (*SEE_mem_free_hook)(struct SEE_interpreter *, void *) 
		= INITIAL_FREE;
void (*SEE_mem_exhausted_hook)(struct SEE_interpreter *) SEE_dead
		= memory_exhausted;

/*------------------------------------------------------------
 * Simple exhaustion handling strategy: abort!
 */

static void
memory_exhausted(interp)
	struct SEE_interpreter *interp;
{
	/* Call the interpreter's abort mechanism */
	(*SEE_abort)(interp, "memory exhausted");
}

#if defined HAVE_GC_MALLOC
/*------------------------------------------------------------
 * Boehm-GC wrapper
 */

#if HAVE_GC_H
# include <gc.h>
#else
extern void *GC_malloc(int);
extern void *GC_malloc_atomic(int);
#endif

static void *
malloc_gc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	return GC_malloc(size);
}

static void *
malloc_string_gc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	return GC_malloc_atomic(size);
}
#endif /* HAVE_GC_MALLOC */

/*------------------------------------------------------------
 * Wrappers around memory allocators that check for failure
 */

/**
 * Allocates size bytes of garbage-collected storage.
 */
void *
SEE_malloc(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	void *data;

	if (size == 0)
		return NULL;
	data = (*SEE_mem_malloc_hook)(interp, size);
	if (data == NULL) 
		(*SEE_mem_exhausted_hook)(interp);
	return data;
}

/**
 * Allocates size bytes of garbage-collected, string storage.
 * This function is just like SEE_malloc(), except that the caller
 * guarantees that no pointers will be stored in the data. This
 * improves performance with strings.
 */
void *
SEE_malloc_string(interp, size)
	struct SEE_interpreter *interp;
	SEE_size_t size;
{
	void *data;

	if (size == 0)
		return NULL;
	if (SEE_mem_malloc_string_hook)
		data = (*SEE_mem_malloc_string_hook)(interp, size);
	else
		data = (*SEE_mem_malloc_hook)(interp, size); /* fallback */
	if (data == NULL) 
		(*SEE_mem_exhausted_hook)(interp);
	return data;
}

/*
 * Called when we *know* that previously allocated storage
 * can be released. 
 *
 *   *** NOT RECOMMENDED TO BE USED ***
 *
 * (Much better is to allocate temp storage on the stack with SEE_ALLOCA().)
 */
void
SEE_free(interp, ptr)
	struct SEE_interpreter *interp;
	void *ptr;
{
	if (SEE_mem_free_hook)
		(*SEE_mem_free_hook)(interp, ptr);
}

/*
 * The debug variants exist for when the library is compiled with NDEBUG,
 * but applications are not. This is just a convenience. 
 */

void *
SEE_malloc_debug(interp, size, file, line, arg)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
	const char *arg;
{
	void *data;

#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("malloc %u (%s:%d '%s')", size, file, line, arg);
#endif
	data = SEE_malloc(interp, size);
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf(" -> %p\n", data);
#endif
	return data;
}

void *
SEE_malloc_string_debug(interp, size, file, line, arg)
	struct SEE_interpreter *interp;
	SEE_size_t size;
	const char *file;
	int line;
	const char *arg;
{
	void *data;

#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf("malloc_string %u (%s:%d '%s')", size, file, line, arg);
#endif
	data = SEE_malloc_string(interp, size);
#ifndef NDEBUG
	if (SEE_mem_debug)
		dprintf(" -> %p\n", data);
#endif
	return data;
}
