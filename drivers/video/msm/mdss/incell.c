/*
 * Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
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
 * Copyright (C) 2015 Sony Mobile Communications Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 */

#include <linux/incell.h>
#include <linux/fb.h>

#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/mdss_io_util.h>

#include "mdss_fb.h"
#include "mdss_mdp.h"
#include "mdss_dsi.h"
#include "mdss_dsi_panel_driver.h"
#include "mdss_dsi_panel_debugfs.h"

struct incell_ctrl *incell = NULL;
struct incell_ctrl incell_buf;

struct incell_ctrl *incell_get_info(void)
{
	return incell;
}

int incell_get_power_status(incell_pw_status *power_status)
{
	struct incell_ctrl *incell = incell_get_info();

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return INCELL_ERROR;
	}

	pr_debug("%s: status = %d\n", __func__, (int)(incell->state));

	switch (incell->state) {
	case INCELL_STATE_SLE000_P1:
	case INCELL_STATE_SLE001_P1:
	case INCELL_STATE_SLE010_P1:
	case INCELL_STATE_SLE011_P1:
	case INCELL_STATE_SLE100_P1:
	case INCELL_STATE_SLE101_P1:
	case INCELL_STATE_SLE110_P1:
	case INCELL_STATE_SLE111_P1:
		power_status->display_power = INCELL_POWER_ON;
		power_status->touch_power = INCELL_POWER_ON;
		break;
	case INCELL_STATE_SLE000_P0:
	case INCELL_STATE_SLE001_P0:
	case INCELL_STATE_SLE010_P0:
	case INCELL_STATE_SLE011_P0:
	case INCELL_STATE_SLE100_P0:
	case INCELL_STATE_SLE101_P0:
	case INCELL_STATE_SLE110_P0:
	case INCELL_STATE_SLE111_P0:
	default:
		power_status->display_power = INCELL_POWER_OFF;
		power_status->touch_power = INCELL_POWER_OFF;
		break;
	}

	return INCELL_OK;
}

static int incell_driver_power_off(struct fb_info *info)
{
	int ret = INCELL_ERROR;
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	struct mdss_mdp_ctl *ctl = NULL;
	struct mdss_panel_data *pdata = NULL;
	struct msm_fb_data_type *mfd = NULL;
	struct incell_ctrl *incell = incell_get_info();

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return ret;
	}

	if (!mdata) {
		pr_err("%s: Invalid mdata\n", __func__);
		return ret;
	}

	ctl = mdata->ctl_off;
	if (!ctl) {
		pr_err("%s: Invalid ctl data\n", __func__);
		return ret;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("%s: Invalid panel data\n", __func__);
		return ret;
	}

	mfd = (struct msm_fb_data_type *)info->par;
	if (!mfd) {
		pr_err("%s: Invalid msm data\n", __func__);
		return ret;
	}

	if (!mdss_fb_is_power_on(mfd)) {
		mdss_dsi_panel_driver_power_off_ctrl(incell);
		mdss_dsi_panel_driver_state_change_off(incell);

		ret = mdss_dsi_panel_driver_power_off(pdata);
		if (ret) {
			pr_err("%s: Failed to power off ret=%d\n",
					__func__, ret);
			ret = INCELL_ERROR;
			return ret;
		}
	} else {
		ret = info->fbops->fb_blank(FB_BLANK_POWERDOWN, info);
		if (ret) {
			pr_err("%s: fb_blank(blank) FAIL ret=%d\n",
					__func__, ret);
			ret = INCELL_ERROR;
			return ret;
		}
	}

	ret = INCELL_OK;
	return ret;
}

static int incell_driver_send_power_on_seq(struct mdss_panel_data *pdata)
{
	int ret = INCELL_ERROR;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_specific_pdata *spec_pdata = NULL;

	ctrl_pdata = container_of(pdata,
			struct mdss_dsi_ctrl_pdata, panel_data);
	spec_pdata = ctrl_pdata->spec_pdata;

	if (!ctrl_pdata || !spec_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		goto end;
	}

	ret = mdss_dsi_panel_driver_power_on(pdata);
	if (ret) {
		pr_err("%s: Failed to power on ret=%d\n", __func__, ret);
		ret = INCELL_ERROR;
		goto end;
	}

	if (mdss_dsi_pinctrl_set_state(ctrl_pdata, true))
		pr_debug("%s: reset enable: pinctrl not enabled\n",
								__func__);
	mdss_dsi_panel_reset(pdata, 1);

	if (incell->incell_intf_operation == INCELL_TOUCH_RUN &&
		incell->intf_mode == INCELL_DISPLAY_HW_RESET) {
		mdss_dsi_panel_driver_reset_touch(pdata, 1);
		mdss_dsi_panel_driver_state_change_on(incell);
	} else {
		if (!ctrl_pdata->on)
			goto end;

		ret = ctrl_pdata->on(pdata);
		if (ret) {
			pr_err("%s: Failed to send on ret=%d\n",
				__func__, ret);
			ret = INCELL_ERROR;
			goto end;
		}

		if (!ctrl_pdata->post_panel_on)
			goto end;

		ret = ctrl_pdata->post_panel_on(pdata);
		if (ret) {
			pr_err("%s: Failed to send post-on ret=%d\n",
				__func__, ret);
			ret = INCELL_ERROR;
			goto end;
		}
	}
	ret = INCELL_OK;
end:
	return ret;
}

static int incell_driver_send_power_on_fb(struct fb_info *info)
{
	int ret = INCELL_ERROR;

	if (!(info->fbops->fb_open) || !(info->fbops->fb_blank)
		|| !(info->fbops->fb_release)) {
		pr_err("%s: Invalid operations\n", __func__);
		goto end;
	}

	ret = info->fbops->fb_blank(FB_BLANK_UNBLANK, info);
	if (ret) {
		pr_err("%s: fb_blank(blank) FAIL ret=%d\n",
				__func__, ret);
		ret = INCELL_ERROR;
		goto end;
	}
	ret = INCELL_OK;
end:
	return ret;
}

static int incell_driver_power_on(struct fb_info *info)
{
	int ret = INCELL_ERROR;
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	struct mdss_mdp_ctl *ctl = NULL;
	struct mdss_panel_data *pdata = NULL;
	struct msm_fb_data_type *mfd = NULL;
	struct fb_specific_data *spec_mfd = NULL;

	if (!mdata) {
		pr_err("%s: Invalid mdata\n", __func__);
		goto end;
	}

	ctl = mdata->ctl_off;
	if (!ctl) {
		pr_err("%s: Invalid ctl data\n", __func__);
		goto end;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("%s: Invalid panel data\n", __func__);
		goto end;
	}

	mfd = (struct msm_fb_data_type *)info->par;
	if (!mfd) {
		pr_err("%s: Invalid msm data\n", __func__);
		goto end;
	}
	spec_mfd = &mfd->spec_mfd;

	if (mdss_fb_is_power_on(mfd))
		ret = incell_driver_send_power_on_seq(pdata);
	else
		ret = incell_driver_send_power_on_fb(info);

	if (spec_mfd->off_sts)
		spec_mfd->off_sts = false;

end:
	return ret;
}

static int incell_display_hw_reset(struct incell_ctrl *incell,
				struct fb_info *info)
{
	int ret = INCELL_ERROR;
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	struct mdss_mdp_ctl *ctl = NULL;
	struct mdss_panel_data *pdata = NULL;

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		goto end;
	}

	if (!mdata) {
		pr_err("%s: Invalid mdata\n", __func__);
		goto end;
	}

	ctl = mdata->ctl_off;
	if (!ctl) {
		pr_err("%s: Invalid ctl data\n", __func__);
		goto end;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("%s: Invalid panel data\n", __func__);
		goto end;
	}

	/* Power off if LCD is on. */
	switch (incell->state) {
	case INCELL_STATE_SLE000_P1:
	case INCELL_STATE_SLE001_P1:
	case INCELL_STATE_SLE010_P1:
	case INCELL_STATE_SLE011_P1:
	case INCELL_STATE_SLE100_P1:
	case INCELL_STATE_SLE101_P1:
	case INCELL_STATE_SLE110_P1:
	case INCELL_STATE_SLE111_P1:
		/*
		 * It calls directly power off to DSI layer,
		 * the case of FB off.
		 */
		ret = incell_driver_power_off(info);
		break;
	default:
		pr_debug("%s: Skip LCD off %d\n", __func__,
					((int)(incell->state)));
		break;
	}

	switch (incell->state) {
	case INCELL_STATE_SLE000_P0:
	case INCELL_STATE_SLE001_P0:
	case INCELL_STATE_SLE010_P0:
	case INCELL_STATE_SLE011_P0:
		pr_debug("%s: LCD on in DSI layer. sts:%d\n", __func__,
					((int)(incell->state)));
		ret = incell_driver_send_power_on_seq(pdata);
		break;
	default:
		pr_debug("%s: LCD on in FB layer. sts:%d\n", __func__,
					((int)(incell->state)));
		ret = incell_driver_send_power_on_fb(info);
		break;
	}

	ret = INCELL_OK;
end:
	return ret;
}

static int incell_display_off(struct incell_ctrl *incell,
				struct fb_info *info)
{
	int ret = INCELL_ERROR;

	switch (incell->state) {
	case INCELL_STATE_SLE000_P0:
	case INCELL_STATE_SLE001_P0:
	case INCELL_STATE_SLE010_P0:
	case INCELL_STATE_SLE011_P0:
	case INCELL_STATE_SLE100_P0:
	case INCELL_STATE_SLE101_P0:
	case INCELL_STATE_SLE110_P0:
	case INCELL_STATE_SLE111_P0:
		pr_err("%s: LCD is already off. sts:%d\n",
			__func__, ((int)(incell->state)));
		ret = INCELL_EALREADY;
		break;
	default:
		pr_debug("%s: incell panel sts:%d\n", __func__,
						((int)(incell->state)));
		break;
	}

	if (ret == INCELL_EALREADY) {
		pr_err("%s: Already power off ret=%d\n", __func__, ret);
		return ret;
	}

	ret = incell_driver_power_off(info);
	return ret;
}

int incell_control_mode(incell_intf_mode mode, bool force)
{
	int ret = INCELL_ERROR;
	struct fb_info *info = NULL;
	struct msm_fb_data_type *mfd = NULL;
	struct fb_specific_data *spec_mfd = NULL;
	struct mdss_data_type *mdata = mdss_mdp_get_mdata();
	struct mdss_mdp_ctl *ctl = NULL;
	struct mdss_panel_data *pdata = NULL;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_specific_pdata *spec_pdata = NULL;
	struct incell_ctrl *incell = incell_get_info();

	pr_notice("%s: START - %s:%s\n", __func__,
		((mode == INCELL_DISPLAY_HW_RESET) ? "INCELL_DISPLAY_HW_RESET" :
		((mode == INCELL_DISPLAY_OFF) ? "INCELL_DISPLAY_OFF" :
						"INCELL_DISPLAY_ON")),
		((force) ? "force" : "unforce"));

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return ret;
	}

	info = registered_fb[0];
	if (!info) {
		pr_err("%s: Invalid fb data\n", __func__);
		return ret;
	}

	if (!(info->fbops->fb_blank) || !(info->fbops->fb_release)
		|| !(info->fbops->fb_open)) {
		pr_err("%s: Invalid operations\n", __func__);
		return ret;
	}

	if (!mdata) {
		pr_err("%s: Invalid mdata\n", __func__);
		return ret;
	}

	ctl = mdata->ctl_off;
	if (!ctl) {
		pr_err("%s: Invalid ctl data\n", __func__);
		return ret;
	}

	pdata = ctl->panel_data;
	if (!pdata) {
		pr_err("%s: Invalid panel data\n", __func__);
		return ret;
	}

	mfd = (struct msm_fb_data_type *)info->par;
	if (!mfd) {
		pr_err("%s: Invalid msm data\n", __func__);
		return ret;
	}
	spec_mfd = &mfd->spec_mfd;

	ctrl_pdata = container_of(pdata,
				struct mdss_dsi_ctrl_pdata, panel_data);
	spec_pdata = ctrl_pdata->spec_pdata;

	if (!ctrl_pdata || !spec_pdata) {
		pr_err("%s: Invalid input data\n", __func__);
		return ret;
	}

	/*
	 * It returns "INCELL_ALREADY_LOCKED"
	 * the case of not setting "INCELL_FORCE" flag.
	 */
	if (mdss_dsi_panel_driver_is_power_lock(incell->state)) {
		if (force == INCELL_UNFORCE) {
			ret = INCELL_ALREADY_LOCKED;
			pr_err("%s: Already power locked ret=%d\n",
							__func__, ret);
			return ret;
		}
	}

	if (incell->worker_state != INCELL_WORKER_OFF) {
		ret = INCELL_EBUSY;
		pr_err("%s: worker scheduling ret=%d\n", __func__, ret);
		return ret;
	}

	if (incell->incell_intf_operation == INCELL_TOUCH_RUN) {
		ret = INCELL_EBUSY;
		pr_err("%s: touch I/F not finished ret=%d\n", __func__, ret);
		return ret;
	}

	incell->incell_intf_operation = INCELL_TOUCH_RUN;

	if (!mutex_trylock(&info->lock)) {
		incell->incell_intf_operation = INCELL_TOUCH_IDLE;
		pr_err("%s: mutex_locked ret=%d\n", __func__, ret);
		ret = INCELL_EBUSY;
		return ret;
	}

	incell->intf_mode = mode;

	switch (mode) {
	case INCELL_DISPLAY_ON:
		incell->intf_mode = INCELL_DISPLAY_HW_RESET;
	case INCELL_DISPLAY_HW_RESET:
		spec_mfd->off_sts = true;
		ret = incell_display_hw_reset(incell, info);
		spec_mfd->off_sts = false;
		break;
	case INCELL_DISPLAY_OFF:
		spec_mfd->off_sts = true;
		ret = incell_display_off(incell, info);
		break;
	default:
		pr_err("%s: Invalid mode for touch interface %d\n",
			__func__, (int)(mode));
		break;
	}

	mutex_unlock(&info->lock);
	incell->incell_intf_operation = INCELL_TOUCH_IDLE;
	pr_notice("%s: FINISH - incell.status:%d\n", __func__,
			((int)(incell->state)));
	return ret;
}

static void incell_panel_power_worker(struct work_struct *work)
{
	struct incell_ctrl *incell = incell_get_info();
	struct fb_info *info = registered_fb[0];

	pr_notice("%s: START - incell.status:%d\n", __func__,
			((int)(incell->state)));

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return;
	}

	if (!info) {
		pr_err("%s: Invalid fb data\n", __func__);
		return;
	}

	mutex_lock(&info->lock);
	incell->worker_state = INCELL_WORKER_ON;

	if (incell->state == INCELL_STATE_SLE000_P1)
		incell_driver_power_off(info);
	else if (incell->state == INCELL_STATE_SLE100_P0 ||
			incell->state == INCELL_STATE_SLE101_P0)
		incell_driver_power_on(info);

	incell->worker_state = INCELL_WORKER_OFF;
	mutex_unlock(&info->lock);

	pr_notice("%s: FINISH - incell.status:%d\n", __func__,
			((int)(incell->state)));
}

static void incell_panel_power_worker_scheduling(incell_pw_lock lock,
						struct incell_ctrl *incell)
{
	incell_state state = incell->state;

	if (lock == INCELL_DISPLAY_POWER_LOCK)
		return;

	if (state != INCELL_STATE_SLE000_P1 &&
		state != INCELL_STATE_SLE100_P0 &&
		state != INCELL_STATE_SLE101_P0)
		return;

	incell->worker_state = INCELL_WORKER_PENDING;
	schedule_work(&incell->incell_work);

	pr_notice("%s: incell worker scheduled - incell.status:%d\n", __func__,
			((int)(incell->state)));
}

void incell_panel_power_worker_canceling(struct incell_ctrl *incell)
{
	struct fb_info *info = registered_fb[0];
	struct msm_fb_data_type *mfd = NULL;
	struct fb_specific_data *spec_mfd = NULL;

	cancel_work_sync(&incell->incell_work);

	pr_notice("%s: incell worker canceled - incell.status:%d\n", __func__,
			((int)(incell->state)));

	if (!info) {
		pr_err("%s: Invalid fb data\n", __func__);
		goto end;
	}

	mfd = (struct msm_fb_data_type *)info->par;
	if (!mfd) {
		pr_err("%s: Invalid msm data\n", __func__);
		goto end;
	}

	spec_mfd = &mfd->spec_mfd;
	if (spec_mfd->off_sts)
		spec_mfd->off_sts = false;

end:
	incell->worker_state = INCELL_WORKER_OFF;
}

static int incell_power_lock(incell_state *state)
{
	int ret = INCELL_OK;

	switch (*state) {
	case INCELL_STATE_SLE010_P0:
	case INCELL_STATE_SLE010_P1:
	case INCELL_STATE_SLE011_P0:
	case INCELL_STATE_SLE011_P1:
	case INCELL_STATE_SLE110_P0:
	case INCELL_STATE_SLE110_P1:
	case INCELL_STATE_SLE111_P0:
	case INCELL_STATE_SLE111_P1:
		pr_err("%s: Power state already locked", __func__);
		ret = INCELL_ALREADY_LOCKED;
		break;
	case INCELL_STATE_SLE000_P0:
		*state = INCELL_STATE_SLE010_P0;
		break;
	case INCELL_STATE_SLE000_P1:
		*state = INCELL_STATE_SLE010_P1;
		break;
	case INCELL_STATE_SLE001_P0:
		*state = INCELL_STATE_SLE011_P0;
		break;
	case INCELL_STATE_SLE001_P1:
		*state = INCELL_STATE_SLE011_P1;
		break;
	case INCELL_STATE_SLE100_P0:
		*state = INCELL_STATE_SLE110_P0;
		break;
	case INCELL_STATE_SLE100_P1:
		*state = INCELL_STATE_SLE110_P1;
		break;
	case INCELL_STATE_SLE101_P0:
		*state = INCELL_STATE_SLE111_P0;
		break;
	case INCELL_STATE_SLE101_P1:
		*state = INCELL_STATE_SLE111_P1;
		break;
	default:
		pr_err("%s: Unknown power status %d\n", __func__,
			(int)(*state));
		ret = INCELL_ERROR;
		break;
	}

	return ret;
}

static int incell_power_unlock(incell_state *state)
{
	int ret = INCELL_OK;

	switch (*state) {
	case INCELL_STATE_SLE000_P0:
	case INCELL_STATE_SLE000_P1:
	case INCELL_STATE_SLE001_P0:
	case INCELL_STATE_SLE001_P1:
	case INCELL_STATE_SLE100_P0:
	case INCELL_STATE_SLE100_P1:
	case INCELL_STATE_SLE101_P0:
	case INCELL_STATE_SLE101_P1:
		pr_err("%s: Power state already unlocked", __func__);
		ret = INCELL_ALREADY_UNLOCKED;
		break;
	case INCELL_STATE_SLE010_P0:
		*state = INCELL_STATE_SLE000_P0;
		break;
	case INCELL_STATE_SLE010_P1:
		*state = INCELL_STATE_SLE000_P1;
		break;
	case INCELL_STATE_SLE011_P0:
		*state = INCELL_STATE_SLE001_P0;
		break;
	case INCELL_STATE_SLE011_P1:
		*state = INCELL_STATE_SLE001_P1;
		break;
	case INCELL_STATE_SLE110_P0:
		*state = INCELL_STATE_SLE100_P0;
		break;
	case INCELL_STATE_SLE110_P1:
		*state = INCELL_STATE_SLE100_P1;
		break;
	case INCELL_STATE_SLE111_P0:
		*state = INCELL_STATE_SLE101_P0;
		break;
	case INCELL_STATE_SLE111_P1:
		*state = INCELL_STATE_SLE101_P1;
		break;
	default:
		pr_err("%s: Unkniown power status %d\n", __func__,
			(int)(*state));
		ret = INCELL_ERROR;
		break;
	}

	return ret;
}

int incell_power_lock_ctrl(incell_pw_lock lock,
		incell_pw_status *power_status)
{
	int ret = INCELL_ERROR;
	struct incell_ctrl *incell = incell_get_info();

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return ret;
	}

	if (incell->worker_state != INCELL_WORKER_OFF) {
		ret = INCELL_EBUSY;
		pr_err("%s: worker scheduling ret=%d\n", __func__, ret);
		return ret;
	}

	if (incell->incell_intf_operation == INCELL_TOUCH_RUN) {
		ret = INCELL_EBUSY;
		pr_err("%s: touch I/F not finished ret=%d\n", __func__, ret);
		return ret;
	}

	incell->incell_intf_operation = INCELL_TOUCH_RUN;

	pr_debug("%s: status:%d --->\n", __func__, ((int)(incell->state)));

	if (lock == INCELL_DISPLAY_POWER_LOCK)
		ret = incell_power_lock(&(incell->state));
	else
		ret = incell_power_unlock(&(incell->state));

	pr_debug("%s: ---> status:%d\n", __func__, ((int)(incell->state)));

	incell_get_power_status(power_status);
	incell_panel_power_worker_scheduling(lock, incell);

	incell->incell_intf_operation = INCELL_TOUCH_IDLE;

	return ret;
}

static void incell_ewu_mode_state_on(incell_state *state)
{
	switch (*state) {
	case INCELL_STATE_SLE000_P0:
		*state = INCELL_STATE_SLE001_P0;
		break;
	case INCELL_STATE_SLE000_P1:
		*state = INCELL_STATE_SLE001_P1;
		break;
	case INCELL_STATE_SLE010_P0:
		*state = INCELL_STATE_SLE011_P0;
		break;
	case INCELL_STATE_SLE010_P1:
		*state = INCELL_STATE_SLE011_P1;
		break;
	case INCELL_STATE_SLE100_P0:
		*state = INCELL_STATE_SLE101_P0;
		break;
	case INCELL_STATE_SLE100_P1:
		*state = INCELL_STATE_SLE101_P1;
		break;
	case INCELL_STATE_SLE110_P0:
		*state = INCELL_STATE_SLE111_P0;
		break;
	case INCELL_STATE_SLE110_P1:
		*state = INCELL_STATE_SLE111_P1;
		break;
	default:
		pr_err("%s: Already EWU enabled\n", __func__);
		break;
	}
}

static void incell_ewu_mode_state_off(incell_state *state)
{
	switch (*state) {
	case INCELL_STATE_SLE001_P0:
		*state = INCELL_STATE_SLE000_P0;
		break;
	case INCELL_STATE_SLE001_P1:
		*state = INCELL_STATE_SLE000_P1;
		break;
	case INCELL_STATE_SLE011_P0:
		*state = INCELL_STATE_SLE010_P0;
		break;
	case INCELL_STATE_SLE011_P1:
		*state = INCELL_STATE_SLE010_P1;
		break;
	case INCELL_STATE_SLE101_P0:
		*state = INCELL_STATE_SLE100_P0;
		break;
	case INCELL_STATE_SLE101_P1:
		*state = INCELL_STATE_SLE100_P1;
		break;
	case INCELL_STATE_SLE111_P0:
		*state = INCELL_STATE_SLE110_P0;
		break;
	case INCELL_STATE_SLE111_P1:
		*state = INCELL_STATE_SLE110_P1;
		break;
	default:
		pr_err("%s: Already EWU disabled\n", __func__);
		break;
	}
}

void incell_ewu_mode_ctrl(incell_ewu_mode ewu)
{
	struct incell_ctrl *incell = incell_get_info();

	pr_notice("%s: START - %s\n", __func__,
		((ewu == INCELL_DISPLAY_EWU_DISABLE) ?
		"INCELL_DISPLAY_EWU_DISABLE" : "INCELL_DISPLAY_EWU_ENABLE"));

	if (!incell) {
		pr_err("%s: Invalid incell data\n", __func__);
		return;
	}

	pr_debug("%s: EWU mode is %s\n", __func__,
			ewu == INCELL_DISPLAY_EWU_ENABLE ? "on":"off");

	pr_debug("%s: status:%d --->\n", __func__, ((int)(incell->state)));

	if ((ewu == INCELL_DISPLAY_EWU_ENABLE))
		incell_ewu_mode_state_on(&(incell->state));
	else
		incell_ewu_mode_state_off(&(incell->state));

	pr_notice("%s: FINISH - incell.status:%d\n", __func__,
			((int)(incell->state)));
}

void incell_driver_init(void)
{
	memset(&incell_buf, 0, sizeof(struct incell_ctrl));
	incell = &incell_buf;

	incell->state = INCELL_STATE_SLE000_P0;
	incell->incell_intf_operation = INCELL_TOUCH_IDLE;

	pr_debug("%s: state:%d touch operation:%d\n", __func__,
		(int)(incell->state), (int)(incell->incell_intf_operation));

	incell->worker_state = INCELL_WORKER_OFF;
	INIT_WORK(&incell->incell_work, incell_panel_power_worker);
}

