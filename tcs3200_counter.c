/*
 * tcs3200_counter.c
 *
 *  Created on: Dec 5, 2013
 *      Author: flo
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include "common.h"
#include "config.h"

static enum hrtimer_restart tcs3200_timer(struct hrtimer *timer) {
	struct tcs_dev *tcs;
	enum STATE next;

	tcs = container_of(timer, struct tcs_dev, timer);
	tcs_disable(tcs);
	switch(tcs->state) {
	case READ_WHITE_HEAD:
		tcs_setup_color(RED);
		next = READ_RED;
		break;
	case READ_RED:
		tcs_setup_color(GREEN);
		next = READ_GREEN;
		break;
	case READ_GREEN:
		tcs_setup_color(BLUE);
		next = READ_BLUE;
		break;
	case READ_BLUE:
		tcs_setup_color(WHITE);
		next = READ_WHITE_TAIL;
		break;
	case READ_WHITE_TAIL:
		next = READ_DONE;
		break;
	default:
		printk(KERN_ERR "%s:%s:%s:%d\n", KBUILD_MODNAME, __FUNCTION__, "invalid state:", tcs->state);
	case READ_DONE:
		tcs_setup_output(PWR_DOWN);
		wake_up_interruptible(&tcs->waitq);
		return HRTIMER_NORESTART;
	}
	tcs->state = next;
	tcs_enable(tcs);
	hrtimer_forward_now(&tcs->timer,
			ns_to_ktime(tcs->dwell));
	return HRTIMER_RESTART;
}

static irqreturn_t tcs3200_irq(int irq, void *in) {
	struct tcs_dev *tcs = (struct tcs_dev *)in;
	struct tcs3200_measurement *m;

	if(atomic_read(&tcs->enabled)) {
		m = &tcs->measurement;
		switch(tcs->state) {
		case READ_WHITE_HEAD:
			++m->white_head;
			break;
		case READ_RED:
			++m->red;
			break;
		case READ_GREEN:
			++m->green;
			break;
		case READ_BLUE:
			++m->blue;
			break;
		case READ_WHITE_TAIL:
			++m->white_tail;
			break;
		case READ_DONE:
			break;
		default:
			printk(KERN_ERR "%s:%s:%s:%d\n", KBUILD_MODNAME, __FUNCTION__, "invalid state:", tcs->state);
		}
	}
	else
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "ignoring unexpected interrupt");
	return IRQ_HANDLED;
}

int tcs_counter_init(struct tcs_dev *tcs) {

	tcs->dwell = ZZZ;
	hrtimer_init(&tcs->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	tcs->timer.function = &tcs3200_timer;
	tcs->state = READ_WHITE_HEAD;

	/* interrupt on falling edge */
	if(gpio_request_one(TCS_OUT_PIN, GPIOF_IN, "TCS3200_INT")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to request interrupt pin");
		goto fail_gpio;
	}
	if((tcs->irq = gpio_to_irq(TCS_OUT_PIN)) < 0) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register interrupt pin");
		goto fail_gpio2int;
	}
	if(request_any_context_irq(tcs->irq, tcs3200_irq,
			IRQF_TRIGGER_FALLING,
			"tcs3200", tcs)) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register interrupt handler");
		goto fail_irq;
	}
	printk(KERN_INFO "tcs3200: using interrupt %d for frequency measurement\n", tcs->irq);
	return 0;

fail_irq:
fail_gpio2int:
	gpio_free(TCS_OUT_PIN);
fail_gpio:
	return -1;
}

void tcs_counter_exit(struct tcs_dev *tcs) {

	tcs_disable(tcs);
	hrtimer_cancel(&tcs->timer);
	free_irq(tcs->irq, tcs);
	gpio_free(TCS_OUT_PIN);
}

int tcs_start_measurement(struct tcs_dev *tcs) {
	struct tcs3200_measurement *m = &tcs->measurement;

	memset(m, 0, sizeof(struct tcs3200_measurement));
	tcs_setup_output(MED);
	tcs_setup_color(WHITE);
	tcs->state = READ_WHITE_HEAD;
	tcs_enable(tcs);
	hrtimer_start(&tcs->timer,
			ns_to_ktime(tcs->dwell),
			HRTIMER_MODE_REL);

	return 0;
}

int tcs_stop_measurement(struct tcs_dev *tcs) {

	tcs_setup_output(PWR_DOWN);
	tcs_disable(tcs);
	return 0;
}
