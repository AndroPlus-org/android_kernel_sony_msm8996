/* Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
/*
 * NOTE: This file has been modified by Sony Mobile Communications Inc.
 * Modifications are Copyright (c) 2016 Sony Mobile Communications Inc,
 * and licensed under the license of the file.
 */

#ifndef __REGULATOR_QPNP_LABIBB_REGULATOR_H__
#define __REGULATOR_QPNP_LABIBB_REGULATOR_H__

#define SOMC_LABIBB_REGULATOR_ORG_IMPL

#include <linux/regulator/driver.h>

#ifdef SOMC_LABIBB_REGULATOR_ORG_IMPL
/** This API is used to set precharge of LAB regulator
 * regulator: the reglator device
 * time: precharge time
 * en: precharge control enable or not
 */
int qpnp_lab_set_precharge(struct regulator *regulator, u32 time, bool en);

/** This API is used to set soft-start of LAB regulator
 * regulator: the reglator device
 * time: soft start time
 */
int qpnp_lab_set_soft_start(struct regulator *regulator, u32 time);

/** This API is used to set pull-down of LAB regulator
 * regulator: the reglator device
 * en: pull-down enable or not
 * strength: strength pull-down
 */
int qpnp_lab_set_pull_down(struct regulator *regulator, u8 strength);

/** This API is used to set current max of LAB regulator
 * regulator: the reglator device
 * limit: current max of LAB regulator
 */
int qpnp_lab_set_current_max(struct regulator *regulator, u32 limit);

/** This API is used to set soft-start of IBB regulator
 * regulator: the reglator device
 * time: soft start time
 */
int qpnp_ibb_set_soft_start(struct regulator *regulator, u32 time);

/** This API is used to set pull-down of IBB regulator
 * regulator: the reglator device
 * en: pull-down enable or not
 * strength: strength pull-down
 */
int qpnp_ibb_set_pull_down(struct regulator *regulator, u8 strength);

/** This API is used to set current max of IBB regulator
 * regulator: the reglator device
 * limit: current max of IBB regulator
 */
int qpnp_ibb_set_current_max(struct regulator *regulator, u32 limit);
#else
static inline int qpnp_lab_set_precharge(struct regulator *regulator,
						u32 time, bool en)
{
	return -ENODEV;
}

static inline int qpnp_lab_set_soft_start(struct regulator *regulator,
						u32 time)
{
	return -ENODEV;
}

static inline int qpnp_lab_set_pull_down(struct regulator *regulator,
						u8 strength)
{
	return -ENODEV;
}

static inline int qpnp_lab_set_current_max(struct regulator *regulator,
						u32 limit)
{
	return -ENODEV;
}

static inline int qpnp_ibb_set_soft_start(struct regulator *regulator,
						u32 time)
{
	return -ENODEV;
}

static inline int qpnp_ibb_set_pull_down(struct regulator *regulator,
						u8 strength)
{
	return -ENODEV;
}

static inline int qpnp_ibb_set_current_max(struct regulator *regulator,
						u32 limit)
{
	return -ENODEV;
}
#endif /* SOMC_LABIBB_REGULATOR_ORG_IMPL */

#endif /* __REGULATOR_QPNP_LABIBB_REGULATOR_H__ */
