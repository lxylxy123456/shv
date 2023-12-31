/*
 * SHV - Small HyperVisor for testing nested virtualization in hypervisors
 * Copyright (C) 2023  Eric Li
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * From: @(#)strtoul.c	8.1 (Berkeley) 6/4/93
 */

/* $FreeBSD: src/sys/libkern/strtoul.c,v 1.6.32.1 2010/02/10 00:26:20 kensmith Exp $ */

#include <xmhf.h>

#define _isspace(c)		((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))
#define _isupper(c)		((c) >= 'A' && (c) <= 'Z')
#define _islower(c)		((c) >= 'a' && (c) <= 'z')
#define _isalpha(c)		(_isupper(c) || _islower(c))
#define _isdigit(c)		((c) >= '0' && (c) <= '9')

#define ULONGLONG_MAX	0xFFFFFFFFFFFFFFFFULL

/*
 * Convert a string to an unsigned long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
unsigned long tb_strtoull(const char *nptr, const char **endptr, int base)
{
	const char *s = nptr;
	unsigned long long acc;
	unsigned char c;
	unsigned long long cutoff;
	int neg = 0, any, cutlim;

	ASSERT(nptr != NULL);
	/*
	 * See strtol for comments as to the logic used.
	 */
	do {
		c = *s++;
	} while (_isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) && c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
	cutoff = (unsigned long long)ULONGLONG_MAX / (unsigned long long)base;
	cutlim = (unsigned long long)ULONGLONG_MAX % (unsigned long long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (_isdigit(c))
			c -= '0';
		else if (_isalpha(c))
			c -= _isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		printf("Error: invalid input to tb_strtoull: %s\n", nptr);
		HALT();
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = any ? s - 1 : nptr;
	return (acc);
}
