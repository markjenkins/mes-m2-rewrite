/* -*-comment-start: "//";comment-end:""-*-
 * GNU Mes --- Maxwell Equations of Software
 * Copyright © 2016,2017,2018 Jan (janneke) Nieuwenhuizen <janneke@gnu.org>
 * Copyright © 2019 Jeremiah Orians
 *
 * This file is part of GNU Mes.
 *
 * GNU Mes is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * GNU Mes is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Mes.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "gcc_req.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// CONSTANT STDIN 0
#define STDIN 0
// CONSTANT STDOUT 1
#define STDOUT 1
// CONSTANT STDERR 2
#define STDERR 2

// CONSTANT FALSE 0
#define FALSE 0
// CONSTANT TRUE 1
#define TRUE 1

// CONSTANT RLIMIT_NOFILE 1024
#define RLIMIT_NOFILE 1024
// CONSTANT FRAME_PROCEDURE 4
#define FRAME_PROCEDURE 4

struct scm
{
	int type;
	union
	{
		SCM car;
		int length;
		SCM macro;
		SCM port;
		SCM ref;
		SCM variable;
	};
	union
	{
		SCM cdr;
		char* cbytes;
		SCM closure;
		SCM continuation;
		int value;
		SCM vector;
		char* string;
		SCM struc;
	};
};

SCM vector_entry(SCM x);


struct scm *g_cells;
struct scm *g_news;

char **environ;
int __stdin;
int __stdout;
int __stderr;
SCM g_continuations;
SCM g_symbols;
SCM g_stack;
SCM *g_stack_array;
int MAX_STRING;
char *g_buf;
long g_free;
int g_debug;
long GC_SAFETY;
long ARENA_SIZE;
long MAX_ARENA_SIZE;
long JAM_SIZE;
long STACK_SIZE;
SCM g_symbol_max;
int* __ungetc_buf;
// a/env
SCM r0;
// param 1
SCM r1;
// save 2
SCM r2;
// continuation
SCM r3;
// current-module
SCM m0;
// macro
SCM g_macros;
SCM g_ports;
