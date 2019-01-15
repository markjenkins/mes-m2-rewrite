/* -*-comment-start: "//";comment-end:""-*-
 * GNU Mes --- Maxwell Equations of Software
 * Copyright © 2016,2017,2018 Jan (janneke) Nieuwenhuizen <janneke@gnu.org>
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

#include "mes.h"
#include "mes_constants.h"
#include <limits.h>

#define TYPE(x) g_cells[x].type
#define CAR(x) g_cells[x].rac
#define CDR(x) g_cells[x].rdc
#define VALUE(x) g_cells[x].rdc
#define MAKE_NUMBER(n) make_cell__ (TNUMBER, 0, (long)n)

int eputs(char const* s);
SCM error(SCM key, SCM x);
SCM car (SCM x);
SCM cdr (SCM x);
SCM cons (SCM x, SCM y);
SCM make_cell__(long type, SCM car, SCM cdr);

void assert_number(char const* name, SCM x)
{
	if(TYPE(x) != TNUMBER)
	{
		eputs(name);
		error(cell_symbol_not_a_number, x);
	}
}

SCM greater_p(SCM x)  ///((name . ">") (arity . n))
{
	if(x == cell_nil)
	{
		return cell_t;
	}

	assert_number("greater_p", CAR(x));
	long n = VALUE(CAR(x));
	x = CDR(x);

	while(x != cell_nil)
	{
		assert_number("greater_p", CAR(x));

		if(VALUE(car(x)) >= n)
		{
			return cell_f;
		}

		n = VALUE(car(x));
		x = cdr(x);
	}

	return cell_t;
}

SCM less_p(SCM x)  ///((name . "<") (arity . n))
{
	if(x == cell_nil)
	{
		return cell_t;
	}

	assert_number("less_p", CAR(x));
	long n = VALUE(CAR(x));
	x = CDR(x);

	while(x != cell_nil)
	{
		assert_number("less_p", CAR(x));

		if(VALUE(car(x)) <= n)
		{
			return cell_f;
		}

		n = VALUE(car(x));
		x = cdr(x);
	}

	return cell_t;
}

SCM is_p(SCM x)  ///((name . "=") (arity . n))
{
	if(x == cell_nil)
	{
		return cell_t;
	}

	assert_number("is_p", CAR(x));
	long n = VALUE(CAR(x));
	x = cdr(x);

	while(x != cell_nil)
	{
		if(VALUE(car(x)) != n)
		{
			return cell_f;
		}

		x = cdr(x);
	}

	return cell_t;
}

SCM minus(SCM x)  ///((name . "-") (arity . n))
{
	assert_number("minus", CAR(x));
	long n = VALUE(CAR(x));
	x = cdr(x);

	if(x == cell_nil)
	{
		n = -n;
	}

	while(x != cell_nil)
	{
		assert_number("minus", CAR(x));
		n -= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM plus(SCM x)  ///((name . "+") (arity . n))
{
	long n = 0;

	while(x != cell_nil)
	{
		assert_number("plus", CAR(x));
		n += VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM divide(SCM x)  ///((name . "/") (arity . n))
{
	long n = 1;

	if(x != cell_nil)
	{
		assert_number("divide", CAR(x));
		n = VALUE(car(x));
		x = cdr(x);
	}

	while(x != cell_nil)
	{
		assert_number("divide", CAR(x));

		if(!n)
		{
			break;
		}

		n /= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM modulo(SCM a, SCM b)
{
	assert_number("modulo", a);
	assert_number("modulo", b);
	long x = VALUE(a);

	while(x < 0)
	{
		x += VALUE(b);
	}

	x = x ? x % VALUE(b) : 0;
	return MAKE_NUMBER(x);
}

SCM multiply(SCM x)  ///((name . "*") (arity . n))
{
	long n = 1;

	while(x != cell_nil)
	{
		assert_number("multiply", CAR(x));
		n *= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM logand(SCM x)  ///((arity . n))
{
	long n = 0;

	while(x != cell_nil)
	{
		assert_number("multiply", CAR(x));
		n &= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM logior(SCM x)  ///((arity . n))
{
	long n = 0;

	while(x != cell_nil)
	{
		assert_number("logior", CAR(x));
		n |= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM lognot(SCM x)
{
	assert_number("lognot", x);
	long n = ~VALUE(x);
	return MAKE_NUMBER(n);
}

SCM logxor(SCM x)  ///((arity . n))
{
	long n = 0;

	while(x != cell_nil)
	{
		assert_number("logxor", CAR(x));
		n ^= VALUE(car(x));
		x = cdr(x);
	}

	return MAKE_NUMBER(n);
}

SCM ash(SCM n, SCM count)
{
	assert_number("ash", n);
	assert_number("ash", count);
	long cn = VALUE(n);
	long ccount = VALUE(count);
	return MAKE_NUMBER((ccount < 0) ? cn >> -ccount : cn << ccount);
}
