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

#include <see/string.h>
#include <see/object.h>
#include <see/mem.h>
#include "enumerate.h"

/*
 * Enumeration of an object's properties
 *
 * (12.6) The for-in statement requires 'delete-safe' enumeration of an
 * object's property names. The SEE_enumerate() function of this module
 * constructs a nul-terminated array of names of all the enumerable
 * properties of the given object. A property name is enumerable if an object
 * or one of its prototypes has a property of that name, AND the
 * 'shallowest' property of that name DOESN'T have the 'DONT_ENUM' flag
 * set.
 *
 * SEE_enumerate() returns the list of names, BUT (according to the standard)
 * the for-in iterator MUST NOT include the property name if it has been
 * deleted in the meantime. That means a HasProperty check must be made
 * each time! (slow!) (See parser.c's IterationStatement evaluator to
 * make sure it happens.)
 *
 * Doing it this way (pre-computing the list of possible names and then
 * checking for deletions later) is probably faster than checking for
 * duplicates each time, and certainly simpler and safer than having
 * back references from the [[Delete]] methods that update the dynamic 
 * enumerators.
 */

struct propname_list {
	struct SEE_string *name;
	struct propname_list *next;
	int dontenum, depth;
};

/*
 * Add the property names of the local object to the property name list.
 */
static int
make_list(interp, o, depth, head)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
	int depth;
	struct propname_list **head;
{
	struct propname_list *l;
	struct SEE_string *s;
	struct SEE_enum *e;
	int dontenum;
	int count;

	e = SEE_OBJECT_ENUMERATOR(interp, o);
	count = 0;
	while ((s = SEE_ENUM_NEXT(interp, e, &dontenum)) != NULL) {
		l = SEE_NEW(interp, struct propname_list);
		l->name = s;
		l->depth = depth;
		l->dontenum = dontenum;
		l->next = *head;
		*head = l;
		count++;
	}
	if (o->Prototype)
		count += make_list(interp, o->Prototype, depth + 1, head);
	return count;
}

/*
 * property name comparison function, used when sorting
 */
static int
slist_cmp(a, b)
	void *a, *b;
{
	struct propname_list **sa = (struct propname_list **)a;
	struct propname_list **sb = (struct propname_list **)b;
	if ((*sa)->name != (*sb)->name)
		return (int)(*sa)->name - (int)(*sb)->name;
		 /*
		  * NB If lexicographically ordered enumerations are 
		  * required, just change the above into a call to
		  * SEE_string_cmp(). (The standard does not require
		  * any particular ordering though.) 
		  */
	return (*sa)->depth - (*sb)->depth;
}

/*
 * Return nul-terminated array of string pointers to
 * all enumerable properties of an object and of its
 * prototypes.
 */
struct SEE_string **
SEE_enumerate(interp, o)
	struct SEE_interpreter *interp;
	struct SEE_object *o;
{
	struct propname_list *head = NULL, **slist, **sp;
	int count, i;
	struct SEE_string *current, **res;

	count = make_list(interp, o, 0, &head);

	/*
	 * Copy the linked list of property names into 
	 * an array, and sort it. (Comparison function
	 * is constant time).
	 */
	slist = SEE_ALLOCA(count, struct propname_list *);
	for (sp = slist; head; head = head->next)
		*sp++ = head;
	qsort(slist, count, sizeof slist[0], slist_cmp);

	/*
	 * Remove duplicate names from the array; also
	 * remove the unique names from the array when their shallowest
	 * entry has the DONT-ENUM flag set.
	 */
	current = NULL;
	sp = slist;
	for (i = 0; i < count; i++) {
	    if (slist[i]->name != current) {
		current = slist[i]->name;
		if (!slist[i]->dontenum)
		    *sp++ = slist[i];
	    }
	}
	count = sp - slist;

	res = SEE_NEW_ARRAY(interp, struct SEE_string *, count + 1);
	for (i = 0; i < count; i++)
		res[i] = slist[i]->name;
	res[count] = NULL;

	return res;
}
