/* Copyright (c) 2003, David Leonard. All rights reserved. */
/* $Id$ */

#ifndef _SEE_h_function_
#define _SEE_h_function_

struct SEE_interpreter;
struct SEE_string;
struct SEE_scope;
struct SEE_native;
struct context;

/* Linked list of variable declarations, or formal parameter names */
struct var {
	struct SEE_string *name;
	struct var *next;		/* linked list of vars */
};

struct function {
	int nparams;
	struct SEE_string **params;
	void *body;			/* FunctionBody_node */
	struct SEE_string *name;	/* optional function name */
	struct SEE_native *common;		/* common to joined functions */
	struct SEE_object *cache;	/* used by SEE_Function_create() */
	struct function *next;		/* linked list of functions */
};

struct function *SEE_function_make(struct SEE_interpreter *i,
	struct SEE_string *name, struct var *vars, void *node);
void SEE_function_put_args(struct context *i, struct function *func,
	int argc, struct SEE_value **argv);

/* obj_Function.c */
struct SEE_object *SEE_function_inst_create(struct SEE_interpreter *i,
	struct function *func, struct SEE_scope *scope);

#endif /* _SEE_h_function_ */
