/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _h_eval
#define _h_eval

struct SEE_string;
struct SEE_value;
struct SEE_interpreter;
struct SEE_input;

void SEE_Global_eval(struct SEE_interpreter *i, struct SEE_input *input, 
	struct SEE_value *res);

struct SEE_object *SEE_Function_new(struct SEE_interpreter *i, 
	struct SEE_string *name, struct SEE_input *param_input, 
	struct SEE_input *body_input);

#endif _h_eval
