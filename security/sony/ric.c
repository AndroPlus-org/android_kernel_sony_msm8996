/*
 * Sony RIC Security Module
 *
 * Author: Srinavasa, Nagaraju <srinavasa.x.nagaraju@sonymobile.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 */
/*
 * Copyright (C) 2013 Sony Mobile Communications Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/security.h>
#include <linux/module.h>
#include <asm/setup.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/uaccess.h>
#include "ric.h"

static bool security_state;
static bool ric_enable = true;

static struct dentry *ric_dir;
static struct dentry *ric_entry;

static struct security_operations sony_ric_ops = {
	.name =	"sony_ric",
	.sb_mount = sony_ric_mount
};

static int sony_ric_setup(char *s)
{
	unsigned long res = memparse(s, NULL);
	security_state = !!res;
	return 0;
}

early_param("oemandroidboot.security", sony_ric_setup);

#define FOTA_BOOT_REASON 0x6f656d46
#define WARMBOOT_STR "warmboot="
static bool is_fota_boot(void)
{
	char *ptr, buf[11];
	unsigned long val;

	ptr = strnstr(saved_command_line, WARMBOOT_STR, COMMAND_LINE_SIZE);
	if (!ptr)
		return 0;

	ptr += sizeof(WARMBOOT_STR) - 1;
	strlcpy(buf, ptr, sizeof(buf));

	if (!kstrtoul(buf, 16, &val)) {
		if (val == FOTA_BOOT_REASON)
			return 1;
	}

	return 0;
}

int sony_ric_enabled(void)
{
	if (is_fota_boot()) {
		pr_debug("RIC: Skipping due to fota boot\n");
		return 0;
	}

	return ric_enable;
}
EXPORT_SYMBOL(sony_ric_enabled);

static ssize_t ric_read(struct file *filp, char __user *buf,
				size_t count, loff_t *ppos)
{
	char temp[10];
	ssize_t rc;

	if (*ppos != 0)
		return 0;

	snprintf(temp, sizeof(temp), "%d\n", ric_enable);
	rc = simple_read_from_buffer(buf, count, ppos, temp, strlen(temp));
	return rc;
}

static ssize_t ric_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	char kbuf[10];
	int ret;
	unsigned long val;

	if (security_state)
		return -EPERM;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	if (count >= sizeof(kbuf) || count == 0)
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count) != 0)
		return -EFAULT;

	kbuf[count] = '\0';

	ret  = kstrtoul(kbuf, 0, &val);
	if (ret)
		return -EINVAL;

	val = !!val;

	ric_enable = val;

	pr_info("RIC: %s\n", ric_enable ? "enabled" : "disabled");

	return count;
}

static int ric_release(struct inode *inode, struct file *file)
{
	if (security_state) {
		securityfs_remove(ric_entry);
		securityfs_remove(ric_dir);
		ric_entry = NULL;
		ric_dir = NULL;
	}

	return 0;
}

static const struct file_operations ric_enable_ops = {
	.read		= ric_read,
	.write		= ric_write,
	.release	= ric_release,
};

static __init int sony_ric_secfs_init(void)
{
	if (!security_state) {
		ric_dir = securityfs_create_dir("sony_ric", NULL);
		if (!ric_dir || IS_ERR(ric_dir)) {
			pr_err("RIC: failed to create sony_ric dir\n");
			return -EFAULT;
		}
		ric_entry = securityfs_create_file("enable", S_IRUSR | S_IRGRP,
						ric_dir, NULL, &ric_enable_ops);
		if (!ric_entry || IS_ERR(ric_entry)) {
			pr_err("RIC: failed to create secfs fuse entry\n");
			return -EFAULT;
		}

		pr_info("RIC: securityfs entry created\n");
	} else {
		ric_enable = security_state;
		pr_info("RIC: securityfs entry not created\n");
	}
	return 0;
}

fs_initcall(sony_ric_secfs_init);

static __init int sony_ric_init(void)
{
	if (!security_module_enable(&sony_ric_ops))
		return 0;

	if (register_security(&sony_ric_ops))
		pr_err(KERN_INFO "RIC: Sony RIC security ops failed to register\n");

	pr_info("RIC: Sony RIC ops registered!\n");

	return 0;
}

security_initcall(sony_ric_init);
