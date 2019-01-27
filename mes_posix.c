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

#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "mes.h"
#include "mes_constants.h"

#define STRING(x) g_cells[x].rdc
#define LENGTH(x) g_cells[x].rac
#define TYPE(x) g_cells[x].type
#define CAR(x) g_cells[x].rac
#define CDR(x) g_cells[x].rdc
#define VALUE(x) g_cells[x].rdc

int readchar();
int unreadchar();
struct scm* write_byte (SCM x);
struct scm* current_input_port ();
int fdgetc (int fd);
struct scm* make_string(char const* s, int length);
struct scm* make_string_(char const* s);
int fdungetc (int c, int fd);
SCM make_cell__(long type, SCM car, SCM cdr);
SCM car (SCM x);
SCM cdr (SCM x);
SCM cons (SCM x, SCM y);
long length__(SCM x);
SCM error(SCM key, SCM x);
int eputs(char const* s);
char *itoa (int number);
SCM acons (SCM key, SCM value, SCM alist);

int eputs(char const* s)
{
	write(__stderr, s, strlen(s));
	return 0;
}

char* ntoab(long x, int base, int signed_p)
{
	static char itoa_buf[20];
	char *p = itoa_buf + 11;
	*p-- = 0;
	int sign_p = 0;
	unsigned long u = x;

	if(signed_p && x < 0)
	{
		sign_p = 1;
		u = -x;
	}

	do
	{
		long i = u % base;
		*p-- = i > 9 ? 'a' + i - 10 : '0' + i;
		u = u / base;
	} while(u);

	if(sign_p && *(p + 1) != '0')
	{
		*p-- = '-';
	}

	return p + 1;
}

char* itoa(int x)
{
	return ntoab(x, 10, 1);
}

void __ungetc_init()
{
	if(__ungetc_buf[RLIMIT_NOFILE] == 0)
	{
		memset(__ungetc_buf, -1, (RLIMIT_NOFILE + 1)*sizeof(int));
	}
}

int fdgetc(int fd)
{
	__ungetc_init();
	char c;
	int i = __ungetc_buf[fd];

	if(i >= 0)
	{
		__ungetc_buf[fd] = -1;
	}
	else
	{
		int r = read(fd, &c, 1);

		if(r < 1)
		{
			return -1;
		}

		i = c;
	}

	if(i < 0)
	{
		i += 256;
	}

	return i;
}

int fdputc(int c, int fd)
{
	write(fd, (char*)&c, 1);
	return 0;
}

int fdputs(char const* s, int fd)
{
	int i = strlen(s);
	write(fd, s, i);
	return 0;
}

int fdungetc(int c, int fd)
{
	__ungetc_init();

	if(c == -1)
	{
		return c;
	}
	else if(__ungetc_buf[fd] != -1)
	{
		eputs(" ***MES C LIB*** fdungetc ungetc buffer overflow fd=");
		eputs(itoa(fd));
		eputs("\n");
		exit(1);
	}

	__ungetc_buf[fd] = c;
	return c;
}

int _fdungetc_p(int fd)
{
	return __ungetc_buf[fd] >= 0;
}

int peekchar()
{
	if(__stdin >= 0)
	{
		int c = readchar();
		unreadchar(c);
		return c;
	}

	struct scm* port = bad2good(current_input_port(), g_cells);
	struct scm* string = bad2good(port->cdr, g_cells);
	size_t length = string->length;

	if(!length)
	{
		return -1;
	}

	char const *p = (char*)&g_cells[string->rdc].rdc;
	return p[0];
}

int readchar()
{
	if(__stdin >= 0)
	{
		return fdgetc(__stdin);
	}

	struct scm* port = bad2good(current_input_port(), g_cells);
	struct scm* string = bad2good(port->cdr, g_cells);
	size_t length = string->length;

	if(!length)
	{
		return -1;
	}

	char const *p = (char*)&g_cells[string->rdc].rdc;
	int c = p[0];
	p = p + 1;
	port->rdc = GetSCM(make_string(p, length - 1));
	return c;
}

int unreadchar(int c)
{
	if(__stdin >= 0)
	{
		return fdungetc(c, __stdin);
	}

	struct scm* port = bad2good(current_input_port(), g_cells);
	struct scm* string = bad2good(port->cdr, g_cells);
	size_t length = string->length;
	char *p = (char*)&g_cells[string->rdc].rdc;
	p = p - 1;
	string = bad2good(make_string(p, length + 1), g_cells);
	p = (char*)&g_cells[string->rdc].rdc;
	p[0] = c;
	port->cdr = good2bad(string, g_cells);
	return c;
}

struct scm* peek_byte()
{
	return good2bad(Getstructscm2(make_cell__ (TNUMBER, 0, peekchar()), g_cells), g_cells);
}

struct scm* read_byte()
{
	return good2bad(Getstructscm2(make_cell__ (TNUMBER, 0,readchar()), g_cells), g_cells);
}

struct scm* unread_byte(SCM i)
{
	struct scm* x = Getstructscm2(i, g_cells);
	unreadchar(x->value);
	return good2bad(x, g_cells);
}

struct scm* peek_char()
{
	return good2bad(Getstructscm2(make_cell__ (TCHAR, 0, peekchar()), g_cells), g_cells);
}

struct scm* read_char(SCM port)  ///((arity . n))
{
	int fd = __stdin;
	struct scm* p = Getstructscm2(port, g_cells);

	if(p->type == TPAIR && bad2good(p->car, g_cells)->type == TNUMBER)
	{
		__stdin = bad2good(p->car, g_cells)->value;
	}

	struct scm* c = Getstructscm2(make_cell__ (TCHAR, 0, readchar()), g_cells);
	__stdin = fd;
	return good2bad(c, g_cells);
}

struct scm* unread_char(SCM i)
{
	struct scm* x = Getstructscm2(i, g_cells);
	unreadchar(x->value);
	return good2bad(x, g_cells);
}

struct scm* write_char(SCM i)  ///((arity . n))
{
	struct scm* x = Getstructscm2(i, g_cells);
	write_byte(GetSCM2(x, g_cells));
	return good2bad(x, g_cells);
}

struct scm* write_byte(SCM x)  ///((arity . n))
{
	struct scm* y = Getstructscm2(x, g_cells);
	struct scm* c = bad2good(y->car, g_cells);
	struct scm* p = bad2good(y->cdr, g_cells);
	struct scm* pp = bad2good(p->car, g_cells);
	int fd = __stdout;

	if(p->type == TPAIR && pp->type == TNUMBER)
	{
		fd = pp->value;
	}

	if(1 == fd) fd = __stdout;
	if(2 == fd) fd = __stderr;

	write(fd, c->string, 1);
	assert(c->type == TNUMBER || c->type == TCHAR);
	return good2bad(c, g_cells);
}

struct scm* getenv_(SCM s)  ///((name . "getenv"))
{
	struct scm* x = Getstructscm2(s, g_cells);
	char *p = getenv((char*)&g_cells[x->rdc].rdc);
	return p ? good2bad(make_string_(p), g_cells) : Getstructscm(cell_f);
}

struct scm* setenv_(SCM s, SCM v)  ///((name . "setenv"))
{
	struct scm* a = Getstructscm2(s, g_cells);
	struct scm* b = Getstructscm2(v, g_cells);
	setenv((char*)&g_cells[a->rdc].rdc, (char*)&g_cells[b->rdc].rdc, 1);
	return Getstructscm(cell_unspecified);
}

struct scm* access_p(SCM file_name, SCM mode)
{
	struct scm* f = Getstructscm2(file_name, g_cells);
	struct scm* m = Getstructscm2(mode, g_cells);
	return access((char*)&g_cells[f->rdc].rdc, m->value) == 0 ? Getstructscm(cell_t) : Getstructscm(cell_f);
}

struct scm* current_input_port()
{
	if(__stdin >= 0)
	{
		return Getstructscm(make_cell__ (TNUMBER, 0, __stdin));
	}

	struct scm* x = Getstructscm2(g_ports, g_cells);

	while(bad2good(x->car, g_cells)->port != __stdin)
	{
		x = bad2good(x->cdr, g_cells);
	}

	return x->car;
}

// The Mes C Library defines and initializes these in crt1
int mes_open(char const *file_name, int flags, int mode)
{
	__ungetc_init();
	int r = open(file_name, flags, mode);

	if(r > 2)
	{
		__ungetc_buf[r] = -1;
	}

	return r;
}

struct scm* open_input_file(SCM file_name)
{
	struct scm* f = Getstructscm2(file_name, g_cells);
	return Getstructscm(make_cell__ (TNUMBER, 0, mes_open((char*)&g_cells[f->rdc].rdc, O_RDONLY, 0)));
}

struct scm* open_input_string(SCM string)
{
	struct scm* port = Getstructscm2(make_cell__ (TPORT, -length__ (g_ports) - 2, string), g_cells);
	g_ports = cons(GetSCM2(port,g_cells), g_ports);
	return good2bad(port, g_cells);
}

struct scm* set_current_input_port(SCM port)
{
	struct scm* x = Getstructscm2(port, g_cells);

	if(TYPE(port) == TNUMBER)
	{
		__stdin = x->value ? x->value : STDIN;
	}
	else if(TYPE(port) == TPORT)
	{
		__stdin = x->rac;
	}

	return current_input_port();
}

struct scm* current_output_port()
{
	return Getstructscm(make_cell__ (TNUMBER, 0, __stdout));
}

struct scm* current_error_port()
{
	return Getstructscm(make_cell__ (TNUMBER, 0, __stderr));
}

struct scm* open_output_file(SCM x)  ///((arity . n))
{
	struct scm* y = Getstructscm2(x, g_cells);
	struct scm* f = bad2good(y->car, g_cells);
	y = bad2good(y->cdr, g_cells);
	int mode = S_IRUSR | S_IWUSR;

	if(y->type == TPAIR && f->type == TNUMBER)
	{
		mode = f->value;
	}

	return Getstructscm(make_cell__ (TNUMBER, 0, mes_open((char*)&g_cells[f->rdc].rdc, O_WRONLY | O_CREAT | O_TRUNC, mode)));
}

struct scm* set_current_output_port(SCM port)
{
	struct scm* p  = Getstructscm2(port, g_cells);
	__stdout = p->value ? p->value : STDOUT;
	return current_output_port();
}

struct scm* set_current_error_port(SCM port)
{
	struct scm* p = Getstructscm2(port, g_cells);
	__stderr = p->value ? p->value : STDERR;
	return current_error_port();
}

struct scm* chmod_(SCM file_name, SCM mode)  ///((name . "chmod"))
{
	struct scm* f = Getstructscm2(file_name, g_cells);
	struct scm* m = Getstructscm2(mode, g_cells);
	chmod((char*)&g_cells[f->rdc].rdc, m->value);
	return Getstructscm(cell_unspecified);
}

struct scm*  isatty_p(SCM port)
{
	struct scm* p = Getstructscm2(port, g_cells);
	return isatty(p->value) ? Getstructscm(cell_t) : Getstructscm(cell_f);
}

struct scm* primitive_fork()
{
	return Getstructscm(make_cell__ (TNUMBER, 0, fork()));
}

struct scm* execl_(SCM file_name, SCM args)  ///((name . "execl"))
{
	char *c_argv[1000];           // POSIX minimum 4096
	int i = 0;

	if(length__(args) > 1000)
	{
		error(cell_symbol_system_error, cons(file_name, cons(GetSCM2(make_string_("too many arguments"), g_cells), cons(file_name, args))));
	}

	c_argv[i++] = (char*)&g_cells[STRING (file_name)].rdc;

	while(args != cell_nil)
	{
		assert(TYPE(CAR(args)) == TSTRING);
		c_argv[i++] = (char*)&g_cells[STRING (CAR(args))].rdc;
		args = CDR(args);

		if(g_debug > 2)
		{
			eputs("arg[");
			eputs(itoa(i));
			eputs("]: ");
			eputs(c_argv[i - 1]);
			eputs("\n");
		}
	}

	c_argv[i] = 0;
	return Getstructscm(make_cell__ (TNUMBER, 0, execv(c_argv[0], c_argv)));
}

struct scm* waitpid_(SCM pid, SCM options)
{
	int status;
	int child = waitpid(VALUE(pid), &status, VALUE(options));
	return Getstructscm(cons(make_cell__ (TNUMBER, 0, child), make_cell__ (TNUMBER, 0, status)));
}

#if __x86_64__
/* Nanoseconds on 64-bit systems with POSIX timers.  */
#define TIME_UNITS_PER_SECOND 1000000000
#else
/* Milliseconds for everyone else.  */
#define TIME_UNITS_PER_SECOND 1000
#endif

struct timespec g_start_time;
struct scm* init_time(SCM a)  ///((internal))
{
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &g_start_time);
	return Getstructscm(acons(cell_symbol_internal_time_units_per_second, make_cell__ (TNUMBER, 0, TIME_UNITS_PER_SECOND), a));
}

struct scm* current_time()
{
	return Getstructscm(make_cell__ (TNUMBER, 0, time(0)));
}

struct scm* gettimeofday_()  ///((name . "gettimeofday"))
{
	struct timeval time;
	gettimeofday(&time, 0);
	return Getstructscm(cons(make_cell__ (TNUMBER, 0, time.tv_sec), make_cell__ (TNUMBER, 0, time.tv_usec)));
}

long seconds_and_nanoseconds_to_long(long s, long ns)
{
	return s * TIME_UNITS_PER_SECOND + ns / (1000000000 / TIME_UNITS_PER_SECOND);
}

struct scm* get_internal_run_time()
{
	struct timespec ts;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
	long time = seconds_and_nanoseconds_to_long(ts.tv_sec - g_start_time.tv_sec, ts.tv_nsec - g_start_time.tv_nsec);
	return Getstructscm(make_cell__ (TNUMBER, 0, time));
}

struct scm* getcwd_()  ///((name . "getcwd"))
{
	char buf[PATH_MAX];
	return good2bad(make_string_(getcwd(buf, PATH_MAX)), g_cells);
}

struct scm* dup_(SCM port)  ///((name . "dup"))
{
	return Getstructscm(make_cell__ (TNUMBER, 0, dup(VALUE(port))));
}

struct scm* dup2_(SCM old, SCM new)  ///((name . "dup2"))
{
	dup2(VALUE(old), VALUE(new));
	return Getstructscm(cell_unspecified);
}

struct scm* delete_file(SCM file_name)
{
	unlink((char*)&g_cells[STRING (file_name)].rdc);
	return Getstructscm(cell_unspecified);
}

int open_boot(char *prefix, char const *boot, char const *location)
{
	strcpy(prefix + strlen(prefix), boot);

	if(g_debug > 1)
	{
		eputs("mes: reading boot-0 [");
		eputs(location);
		eputs("]: ");
		eputs(prefix);
		eputs("\n");
	}

	int fd = (int)mes_open(prefix, O_RDONLY, 0);

	if(g_debug && fd > 0)
	{
		eputs("mes: read boot-0: ");
		eputs(prefix);
		eputs("\n");
	}

	return fd;
}
