/*
 * @XMHF_LICENSE_HEADER_START@
 *
 * eXtensible, Modular Hypervisor Framework (XMHF)
 * Copyright (c) 2009-2012 Carnegie Mellon University
 * Copyright (c) 2010-2012 VDG Inc.
 * All Rights Reserved.
 *
 * Developed by: XMHF Team
 *               Carnegie Mellon University / CyLab
 *               VDG Inc.
 *               http://xmhf.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the names of Carnegie Mellon or VDG Inc, nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @XMHF_LICENSE_HEADER_END@
 */

// dbg-event-logger.c
// Event logger for debugging
// Author(s): Eric Li (xiaoyili@andrew.cmu.edu)

#include <xmhf.h>

#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
LRU_NEW_SET(event_log_##name##_set_t, event_log_##name##_line_t, lru_size, \
			index_type, key_type, count_type);
#include <xmhf-debug-event-logger-fields.h>

typedef struct event_log_t {
	u64 last_print_tsc;
	u32 total_count;
#define DEFINE_EVENT_FIELD(name, count_type, ...) \
	count_type count_##name; \
	event_log_##name##_set_t lru_##name;
#include <xmhf-debug-event-logger-fields.h>
} event_log_t;

volatile u32 turn = 0;
event_log_t global_event_log[MAX_VCPU_ENTRIES];

void xmhf_dbg_log_event(void *_vcpu, xmhf_dbg_eventlog_t event, void *key) {
	bool print_flag = false;
	VCPU *vcpu = _vcpu;
	/* Get event log */
	event_log_t *event_log = &global_event_log[vcpu->idx];
	/* Increase count */
	event_log->total_count++;
	switch (event) {
#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
	case XMHF_DBG_EVENTLOG_##name: \
		event_log->count_##name++; \
		{ \
			index_type index; \
			bool cache_hit; \
			event_log_##name##_line_t *line; \
			line = LRU_SET_FIND_EVICT(&event_log->lru_##name, \
									  *(key_type *)key, index, cache_hit); \
			if (!cache_hit) { \
				line->value = 0; \
			} \
			line->value++; \
			(void) index; \
		} \
		break;
#include <xmhf-debug-event-logger-fields.h>
	default:
		HALT_ON_ERRORCOND(0 && "Unknown event");
	}
	/* Decide whether to print */
	{
		if (turn == vcpu->idx) {
			u64 tsc = rdtsc64();
			if (event_log->last_print_tsc == 0) {
				event_log->last_print_tsc = tsc;
			}
			if (tsc > event_log->last_print_tsc + 0x100000000ULL) {
				print_flag = true;
				event_log->last_print_tsc = 0;
				turn++;
				turn %= g_midtable_numentries;
			}
		}
	}
	/* Print result */
	if (print_flag) {
		printf("CPU(0x%02x): %s begin\n", vcpu->id, __func__);
		printf("CPU(0x%02x):  total_count = %d\n", vcpu->id,
			   event_log->total_count);
		event_log->total_count = 0;
#define DEFINE_EVENT_FIELD(name, count_type, count_fmt, lru_size, index_type, \
						   key_type, key_fmt, ...) \
		printf("CPU(0x%02x):  count_" #name " = " count_fmt "\n", vcpu->id, \
			   event_log->count_##name); \
		event_log->count_##name = 0; \
		{ \
			index_type index; \
			event_log_##name##_line_t *line; \
			LRU_FOREACH(index, line, &event_log->lru_##name) { \
				if (line->valid) { \
					printf("CPU(0x%02x):   " key_fmt " count " count_fmt "\n", \
						   vcpu->id, line->key, line->value); \
					line->valid = 0; \
				} \
			} \
		}
#include <xmhf-debug-event-logger-fields.h>
		printf("CPU(0x%02x): %s end\n", vcpu->id, __func__);
	}
	(void) key;
}

