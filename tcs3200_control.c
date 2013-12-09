/*
 * tcs3200_control.c
 *
 *  Created on: Dec 5, 2013
 *      Author: flo
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include "common.h"
#include "config.h"

/*
 * enable / disable the tcs3200
 */
void tcs_enable(struct tcs_dev *tcs) {

	atomic_set(&tcs->enabled, 1);
	/* active low */
	gpio_set_value(TCS_ENABLE_PIN, 0);
}

void tcs_disable(struct tcs_dev *tcs) {

	atomic_set(&tcs->enabled, 0);
	/* active low */
	gpio_set_value(TCS_ENABLE_PIN, 1);
}

/*
 * select color channel
 */
int tcs_setup_color(enum tcs_color c) {

	switch(c) {
	//TODO: check return values
	case RED:
		gpio_set_value(TCS_S2_PIN, 0);
		gpio_set_value(TCS_S3_PIN, 0);
		break;
	case GREEN:
		gpio_set_value(TCS_S2_PIN, 1);
		gpio_set_value(TCS_S3_PIN, 1);
		break;
	case BLUE:
		gpio_set_value(TCS_S2_PIN, 0);
		gpio_set_value(TCS_S3_PIN, 1);
		break;
	case WHITE:
		gpio_set_value(TCS_S2_PIN, 1);
		gpio_set_value(TCS_S3_PIN, 0);
		break;
	default:
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "invalid color");
		return -1;
	}
	return 0;
}

/*
 * setup current to frequency conversion
 * handles power down mode as well
 * see datasheet for typical values
 */
int tcs_setup_output(enum tcs_output_frequency f) {

	switch(f) {
	//TODO: check return values
	case PWR_DOWN:
		gpio_set_value(TCS_S0_PIN, 0);
		gpio_set_value(TCS_S1_PIN, 0);
		break;
	case LOW:
		gpio_set_value(TCS_S0_PIN, 0);
		gpio_set_value(TCS_S1_PIN, 1);
		break;
	case MED:
		gpio_set_value(TCS_S0_PIN, 1);
		gpio_set_value(TCS_S1_PIN, 0);
		break;
	case HIGH:
		gpio_set_value(TCS_S0_PIN, 1);
		gpio_set_value(TCS_S1_PIN, 1);
		break;
	default:
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "invalid output setting");
		return -1;
	}
	return 0;
}

/*
 * init tcs3200 control
 */
int __init tcs_control_init(struct tcs_dev *tcs) {

	atomic_set(&tcs->enabled, 0);
	/* init enable pin (active low)*/
	if(gpio_request_one(TCS_ENABLE_PIN, GPIOF_INIT_HIGH, "TCS3200_EN")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register enable pin");
		goto fail_enable;
	}
	/* init current to frequency converter pins */
	if(gpio_request_one(TCS_S0_PIN, GPIOF_INIT_LOW, "TCS3200_S0")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register S0 pin");
		goto fail_s0;
	}
	if(gpio_request_one(TCS_S1_PIN, GPIOF_INIT_LOW, "TCS3200_S1")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register S1 pin");
		goto fail_s1;
	}
	/* init color mux pins */
	if(gpio_request_one(TCS_S2_PIN, GPIOF_INIT_LOW, "TCS3200_S2")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register S2 pin");
		goto fail_s2;
	}
	if(gpio_request_one(TCS_S3_PIN, GPIOF_INIT_LOW, "TCS3200_S3")) {
		printk(KERN_ERR "%s:%s:%s\n", KBUILD_MODNAME, __FUNCTION__, "failed to register S3 pin");
		goto fail_s3;
	}
	tcs_setup_output(PWR_DOWN);
	tcs_disable(tcs);
	return 0;

fail_s3:
	gpio_free(TCS_S2_PIN);
fail_s2:
	gpio_free(TCS_S1_PIN);
fail_s1:
	gpio_free(TCS_S0_PIN);
fail_s0:
	gpio_free(TCS_ENABLE_PIN);
fail_enable:
	return -1;
}

void __exit tcs_control_exit(void) {

	gpio_free(TCS_S3_PIN);
	gpio_free(TCS_S2_PIN);
	gpio_free(TCS_S1_PIN);
	gpio_free(TCS_S0_PIN);
	gpio_free(TCS_ENABLE_PIN);
}
