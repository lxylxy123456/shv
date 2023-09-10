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

#include <xmhf.h>

#define _isspace(c)		((c) == ' ' || ((c) >= '\t' && (c) <= '\r'))

void parse_cmdline(const char *cmdline)
{
	const char *begin_word = cmdline;
	while (*begin_word) {
		const char *end_word;
		end_word = begin_word;
		while (*end_word != '\0' && !_isspace(*end_word)) {
			end_word++;
		}
		printf("Word: %.*s\n", (end_word - begin_word), begin_word);
		begin_word = end_word;
		while (_isspace(*begin_word)) {
			begin_word++;
		}
	}
}
