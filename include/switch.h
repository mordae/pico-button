/*
 * Copyright (C) 2022 Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <pico/stdlib.h>
#include <pico/util/queue.h>

#include <stdbool.h>
#include <stdint.h>


/* Number of switches. */
#if !defined(NUM_SWITCHES)
# define NUM_SWITCHES 4
#endif


/*
 * Switch event.
 */
struct switch_event {
	/* Switch number. */
	uint8_t num;

	/* State of the switch. */
	bool sw;
};


/*
 * Initialize switches.
 */
void switch_init(void);


/* Configure given switch. */
void switch_config(unsigned num, uint8_t pin);


/* Read next switch event or block. */
void switch_read_blocking(struct switch_event *event);
