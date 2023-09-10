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

u64 g_shv_opt = SHV_OPT;
u64 g_nmi_opt = NMI_OPT;
u64 g_nmi_exp = NMI_EXP;

static const struct {
	u64 *ptr;
	const char *prefix;
} cmdline_vars[] = {
	{.ptr=&g_shv_opt, .prefix="shv_opt="},
	{.ptr=&g_nmi_opt, .prefix="nmi_opt="},
	{.ptr=&g_nmi_exp, .prefix="nmi_exp="},
	{.ptr=NULL, .prefix=NULL},
};

static void match_var(const char *begin_word,const char *end_word)
{
	//printf("Word: %.*s\n", (end_word - begin_word), begin_word);
	for (u32 i = 0; cmdline_vars[i].ptr; i++) {
		const char *prefix = cmdline_vars[i].prefix;
		size_t len = strlen(prefix);
		if (strlen(begin_word) >= len && memcmp(begin_word, prefix, len) == 0) {
			const char *begin_val = begin_word + len;
			const char *end_val;
			*(cmdline_vars[i].ptr) = tb_strtoull(begin_val, &end_val, 0);
		}
	}
}

void parse_cmdline(const char *cmdline)
{
	const char *begin_word = cmdline;
	while (*begin_word) {
		const char *end_word;
		end_word = begin_word;
		while (*end_word != '\0' && !_isspace(*end_word)) {
			end_word++;
		}
		match_var(begin_word, end_word);
		begin_word = end_word;
		while (_isspace(*begin_word)) {
			begin_word++;
		}
	}
}
