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

#include "switch.h"

#include <stdio.h>
#include <stdint.h>


/* How many microseconds to wait in order to resolve possible bounce. */
#if !defined(SWITCH_DEBOUNCE_US)
# define SWITCH_DEBOUNCE_US 1000
#endif

/* How many events can the queue hold. */
#if !defined(SWITCH_QUEUE_SIZE)
# define SWITCH_QUEUE_SIZE 16
#endif

/* Enable pull up on switch pins. */
#if !defined(SWITCH_PULL_UP)
# define SWITCH_PULL_UP 1
#endif


struct state {
	uint8_t sw_pin;
	uint32_t sw_mtime;
	alarm_id_t sw_alarm;
	bool sw : 1;
	bool configured: 1;
};


/* Switch states. */
static struct state state[NUM_SWITCHES];

/* Interrupt handler event queue. */
static queue_t queue;


/*
 * Alarm handler for `irq_handler_sw`.
 * See below for more information.
 */
static int64_t __no_inline_not_in_flash_func(debounce)(alarm_id_t id, void *arg)
{
	struct state *st = arg;

	/* Alarm has timed out and we got called. Clear the handle. */
	st->sw_alarm = -1;

	/* Read the current state. */
	bool sw = !gpio_get(st->sw_pin);

	/* Filter out repeated events as usual. */
	if (st->sw == sw)
		return 0;

	/*
	 * Update the state. We more-or-less trust this value, since human
	 * would not be able to repeat a switch cycle this fast.
	 *
	 * If they are carefully holding the switch mid-press, they are going
	 * to be able to produce valid strings of random toggles though.
	 */
	st->sw = sw;

	/* Emit the event. */
	struct switch_event event = {
		.num = st - &state[0],
		.sw = sw,
	};
	(void)queue_try_add(&queue, &event);
	return 0;
}


__isr static void __no_inline_not_in_flash_func(irq_handler_sw)(void)
{
	const unsigned event_mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;

	for (int i = 0; i < NUM_SWITCHES; i++) {
		struct state *st = &state[i];

		/* Skip unconfigured switches. */
		if (!st->configured)
			continue;

		unsigned events = gpio_get_irq_event_mask(st->sw_pin);
		if (events & event_mask) {
			/* Acknowledge interrupt. */
			gpio_acknowledge_irq(st->sw_pin, event_mask);

			/* Read current switch state. */
			bool sw = !gpio_get(st->sw_pin);

			/* Filter out repeated events. */
			if (st->sw == sw)
				continue;

			/*
			 * We might have set up an alarm to combat bounce.
			 * If we did, we need to cancel it since another
			 * interrupt (this one) occured sooner.
			 */
			if (st->sw_alarm >= 0) {
				cancel_alarm(st->sw_alarm);
				st->sw_alarm = -1;
			}

			/*
			 * If the switch state changes too fast, it might be
			 * a bounce. Humans are not that fast.
			 */
			uint32_t prev = st->sw_mtime;
			uint32_t now = time_us_32();

			if (now - prev < SWITCH_DEBOUNCE_US) {
				/* We are going to follow up on this change. */
				st->sw_alarm = add_alarm_in_us(SWITCH_DEBOUNCE_US, debounce, st, true);

				/* But we ignore it for now. */
				continue;
			}

			/* There was long enough delay, so we trust this value. */
			st->sw_mtime = now;
			st->sw = sw;

			/* Emit the event. */
			struct switch_event event = {
				.num = i,
				.sw  = st->sw,
			};
			(void)queue_try_add(&queue, &event);
		}
	}
}


void switch_init(void)
{
	queue_init(&queue, sizeof(struct switch_event), SWITCH_QUEUE_SIZE);
	alarm_pool_init_default();
	irq_set_enabled(IO_IRQ_BANK0, true);
}


void switch_config(unsigned num, uint8_t sw_pin)
{
	if (state[num].configured)
		panic("switch_config: switch num=%u already configured", num);

	state[num].configured = true;
	state[num].sw_pin = sw_pin;

	gpio_init(sw_pin);
	gpio_set_dir(sw_pin, false);
#if SWITCH_PULL_UP
	gpio_pull_up(sw_pin);
#else
	gpio_disable_pulls(sw_pin);
#endif

	gpio_add_raw_irq_handler(sw_pin, irq_handler_sw);

	unsigned event_mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
	gpio_set_irq_enabled(sw_pin, event_mask, true);

	printf("switch: Configured: num=%i, pin=%i\n", num, sw_pin);
}


void switch_read_blocking(struct switch_event *event)
{
	(void)queue_remove_blocking(&queue, event);
}
