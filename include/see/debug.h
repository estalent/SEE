/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_debug_
#define _SEE_h_debug_

#include <see/config.h>

#if STDC_HEADERS
#include <stdio.h>
#endif

struct SEE_value;
struct SEE_object;
struct SEE_interpreter;

void SEE_PrintValue(struct SEE_interpreter *i, struct SEE_value *v, FILE *f);
void SEE_PrintObject(struct SEE_interpreter *i, struct SEE_object *o, FILE *f);
void SEE_PrintString(struct SEE_interpreter *i, struct SEE_string *s, FILE *f);
void SEE_PrintTraceback(struct SEE_interpreter *i, FILE *f);

#endif /* _SEE_h_debug_ */
