/* include/linux/sim_detect.h
 *
 * Author: Atsushi Iyogi <atsushi.x.iyogi@sonymobile.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */
/*
 * Copyright (C) 2015 Sony Mobile Communications Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_SIM_DETECT_H
#define _LINUX_SIM_DETECT_H

struct device;

struct sim_detect_gpio_event {
	int index;
	int gpio;
	int active_low;
	const char *desc;
	int debounce_interval;
	unsigned int irq;
};

struct sim_detect_platform_data {
	int n_events;
	struct sim_detect_gpio_event *events;
};

#endif
