/*
 *  drivers/misc/powerkey_forcecrash.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*
 * Copyright (C) 2015 Sony Mobile Communications Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

#define FORCE_CRASH_TIMEOUT 10
static struct timer_list forcecrash_timer;

#define PKEY_FORCECRASH_DEV_NAME "powerkey_forcecrash"
static struct wake_lock powerkey_lock;

static int forcecrash_on;
module_param(forcecrash_on, int, S_IRUGO | S_IWUSR);

static void forcecrash_timeout(unsigned long data)
{
	panic("Force crash triggered!!!\n");
}

static void forcecrash_timer_setup(bool key_pressed)
{
	if (!forcecrash_on)
		return;

	if (key_pressed) {
		pr_debug("Power key pressed..\n");
		mod_timer(&forcecrash_timer,
				jiffies + FORCE_CRASH_TIMEOUT * HZ);
		wake_lock(&powerkey_lock);
	} else {
		pr_debug("released.\n");
		del_timer(&forcecrash_timer);
		wake_unlock(&powerkey_lock);
	}
}

static void powerkey_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	switch (code) {
	case KEY_POWER:
		forcecrash_timer_setup(value);
		break;
	default:
		break;
	}
}

static const char *powerkey_match_tbl[] = {
	"qpnp_pon",
	NULL
};

static bool powerkey_input_match(struct input_handler *handler,
		struct input_dev *dev)
{
	const char *match = powerkey_match_tbl[0];

	while (*match) {
		size_t len = strlen(match);
		if (!strncmp(dev->name, match, len))
			return true;
		match++;
	}

	pr_info("Ignoring %s handle for %s\n", handler->name, dev->name);

	return false;
}

static int powerkey_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "powerkey_handle";
	pr_info("registering %s handle for %s\n", handle->name, dev->name);

	error = input_register_handle(handle);
	if (error)
		goto err2;

	error = input_open_device(handle);
	if (error)
		goto err1;

	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void powerkey_input_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id input_dev_ids[] = {
	/* Only Powerkey inputs */
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
		.keybit = { [BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER) },
	},
	{ },
};

static struct input_handler powerkey_input_handler = {
	.event		= powerkey_input_event,
	.match		= powerkey_input_match,
	.connect	= powerkey_input_connect,
	.disconnect	= powerkey_input_disconnect,
	.name		= "powerkey_handler",
	.id_table	= input_dev_ids,
};

static int __init powerkey_forcecrash_init(void)
{
	init_timer(&forcecrash_timer);
	forcecrash_timer.function = forcecrash_timeout;
	wake_lock_init(&powerkey_lock, WAKE_LOCK_SUSPEND,
			PKEY_FORCECRASH_DEV_NAME);
	return input_register_handler(&powerkey_input_handler);
}

static void __exit powerkey_forcecrash_exit(void)
{
	del_timer(&forcecrash_timer);
	wake_lock_destroy(&powerkey_lock);
	input_unregister_handler(&powerkey_input_handler);
}

module_init(powerkey_forcecrash_init);
module_exit(powerkey_forcecrash_exit);

MODULE_AUTHOR("Srinivasa Nagaraju <srinavasa.x.nagaraju@sonymobile.com>");
MODULE_DESCRIPTION("Force crash on power key long press of 10 secs");
MODULE_LICENSE("GPL V2");
