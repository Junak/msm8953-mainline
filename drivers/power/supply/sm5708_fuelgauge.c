/*
 * Copyright (C) 2013 Dongik Sin <dongik.sin@samsung.com>
 *
 * SM5708 battery fuel gauge driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/math64.h>
#include <linux/compiler.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/i2c.h>

/* Definitions of SM5708 Fuelgauge Registers */
#define SM5708_REG_DEVICE_ID		0x00
#define SM5708_REG_CNTL			0x01
#define SM5708_REG_INTFG		0x02
#define SM5708_REG_INTFG_MASK		0x03
#define SM5708_REG_STATUS		0x04
#define SM5708_REG_SOC			0x05
#define SM5708_REG_OCV			0x06
#define SM5708_REG_VOLTAGE		0x07
#define SM5708_REG_CURRENT		0x08
#define SM5708_REG_TEMPERATURE		0x09
#define SM5708_REG_SOC_CYCLE		0x0A

#define SM5708_REG_V_ALARM		0x0C
#define SM5708_REG_T_ALARM		0x0D
#define SM5708_REG_SOC_ALARM		0x0E
#define SM5708_REG_FG_OP_STATUS		0x10
#define SM5708_REG_TOPOFFSOC		0x12
#define SM5708_REG_PARAM_CTRL		0x13
#define SM5708_REG_PARAM_RUN_UPDATE	0x14

#define SM5708_REG_SOC_CYCLE_CFG	0x15
#define SM5708_CYCLE_HIGH_LIMIT_SHIFT	12
#define SM5708_CYCLE_LOW_LIMIT_SHIFT	8
#define SM5708_CYCLE_LIMIT_CNTL_SHIFT	0

#define SM5708_REG_VIT_PERIOD		0x1A
#define SM5708_REG_MIX_RATE		0x1B
#define SM5708_REG_MIX_INIT_BLANK	0x1C
#define SM5708_REG_RESERVED		0x1F

#define SM5708_REG_RCE0			0x20
#define SM5708_REG_RCE1			0x21
#define SM5708_REG_RCE2			0x22
#define SM5708_REG_DTCD			0x23
#define SM5708_REG_AUTO_RS_MAN		0x24
#define SM5708_REG_RS_MIX_FACTOR	0x25
#define SM5708_REG_RS_MAX		0x26
#define SM5708_REG_RS_MIN		0x27
#define SM5708_REG_RS_TUNE		0x28
#define SM5708_REG_RS_MAN		0x29

/* for	cal */
#define SM5708_REG_CURR_CAL		0x2C
#define SM5708_REG_IOCV_MAN		0x2E
#define SM5708_REG_END_V_IDX		0x2F

#define SM5708_REG_IOCV_B_L_MIN		0x30
#define SM5708_REG_IOCV_B_L_MAX		0x35
#define SM5708_REG_IOCV_B_C_MIN		0x36
#define SM5708_REG_IOCV_B_C_MAX		0x3B
#define SM5708_REG_IOCI_B_L_MIN		0x40
#define SM5708_REG_IOCI_B_L_MAX		0x45
#define SM5708_REG_IOCI_B_C_MIN		0x46
#define SM5708_REG_IOCI_B_C_MAX		0x4B

#define SM5708_REG_VOLT_CAL		0x50
#define SM5708_REG_CURR_OFF		0x51
#define SM5708_REG_CURR_P_SLOPE		0x52
#define SM5708_REG_CURR_N_SLOPE		0x53
#define SM5708_REG_CURRLCAL_0		0x54
#define SM5708_REG_CURRLCAL_1		0x55
#define SM5708_REG_CURRLCAL_2		0x56

/* for	debug */
#define SM5708_REG_OCV_STATE		0x80
#define SM5708_REG_CURRENT_EST		0x85
#define SM5708_REG_CURRENT_ERR		0x86
#define SM5708_REG_Q_EST		0x87
#define SM5708_AUX_STAT			0x94

/* etc	*/
#define SM5708_REG_MISC			0x90
#define SM5708_REG_RESET		0x91
#define SM5708_FG_INIT_MARK		0xA000
#define SM5708_FG_PARAM_UNLOCK_CODE	0x3700
#define SM5708_FG_PARAM_LOCK_CODE	0x0000
#define SM5708_FG_TABLE_LEN		0xF  /*real table length -1*/

/* start reg addr for table */
#define SM5708_REG_TABLE_START		0xA0

#define SW_RESET_CODE			0x00A6
#define SW_RESET_OTP_CODE		0x01A6
#define RS_MAN_CNTL			0x0800

/* control register value */
#define ENABLE_MIX_MODE			0x8000
#define ENABLE_TEMP_MEASURE		0x4000
#define ENABLE_TOPOFF_SOC		0x2000
#define ENABLE_RS_MAN_MODE		0x0800
#define ENABLE_MANUAL_OCV		0x0400
#define ENABLE_MODE_nENQ4		0x0200

#define ENABLE_SOC_ALARM		0x0008
#define ENABLE_T_H_ALARM		0x0004
#define ENABLE_T_L_ALARM		0x0002
#define ENABLE_V_ALARM			0x0001

#define CNTL_REG_DEFAULT_VALUE		0x2008
#define INIT_CHECK_MASK			0x0010
#define DISABLE_RE_INIT			0x0010
#define SM5708_JIG_CONNECTED		0x0001
#define SM5708_BATTERY_VERSION		0x00F0

#define TOPOFF_SOC_97			0x111
#define TOPOFF_SOC_96			0x110
#define TOPOFF_SOC_95			0x101
#define TOPOFF_SOC_94			0x100
#define TOPOFF_SOC_93			0x011
#define TOPOFF_SOC_92			0x010
#define TOPOFF_SOC_91			0x001
#define TOPOFF_SOC_90			0x000

#define MASK_L_SOC_INT			0x0008
#define MASK_H_TEM_INT			0x0004
#define MASK_L_TEM_INT			0x0002
#define MASK_L_VOL_INT			0x0001

#define MAXVAL(a, b)			(((a) > (b)) ? (a) : (b))
#define MINVAL(a, b)			(((a) < (b)) ? (a) : (b))

#define BULK_START			0x10000
#define BULK_STOP			0x20000
#define APPLY_FACTOR(val, fact)		((val) * (fact) / 1000)
#define DEF_FACTOR(nom, den)		(1000 * (nom) / (den))

struct sm5708_battery_data {
	const u32 *reg_init_seq;
	size_t reg_init_seq_len;
	int rs_value[5];
	int n_temp_poff;
	int n_temp_poff_offset;
	int l_temp_poff;
	int l_temp_poff_offset;
	int value_v_alarm;
	int topoff_soc;
	int top_off;
	int volt_cal;
	int p_curr_cal;
	int n_curr_cal;
	int temp_std;
	int fg_temp_volcal_fact;
	int high_fg_temp_offset_fact;
	int low_fg_temp_offset_fact;
	int high_fg_temp_n_cal_fact;
	int high_fg_temp_p_cal_fact;
	int low_fg_temp_p_cal_fact;
	int low_fg_temp_n_cal_fact;
	int low_temp_p_cal_fact;
	int low_temp_n_cal_fact;
	int capacity;
};

struct sm5708_battery {
	struct i2c_client *client;
	struct power_supply *psy;
	struct device *dev;
	struct regmap *regmap;
	const struct sm5708_battery_data  *data;

	int last_soc;
	int last_voltage;
	int last_ocv;
	int last_current;
	int last_temp;
	int prev_voltage;
	int iocv_error_count;
};

static enum power_supply_property sm5708_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
};

static int sm5708_read_ext(struct sm5708_battery *bat, unsigned int reg,
		int int_mask, int frac_mask, int sign_mask, int mult)
{
	int ret;
	u32 regval;

	ret = regmap_read(bat->regmap, reg, &regval);
	if (ret < 0)
		return ret;

	ret = 0;

	if (int_mask != 0) {
		ret += ((regval & int_mask) >> (ffs(int_mask) - 1)) * mult;
	}
	if (frac_mask != 0) {
		int fshift = ffs(frac_mask) - 1;
		int mask = frac_mask >> fshift;
		ret += ((regval >> fshift) & mask) * mult / (mask + 1);
	}
	if (regval & sign_mask)
		ret = -ret;
	return ret;
}

static int sm5708_get_ocv(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_OCV,
			       0x7800, 0x7ff, 0, 1000000);
}

static int sm5708_get_vbat(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_VOLTAGE,
			       0x3800, 0x7ff, 0, 1000000);
}

static int sm5708_get_curr(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_CURRENT,
			       0x1800, 0x7ff, 0x8000, 1000000);
}

static int sm5708_get_temperature(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_TEMPERATURE,
			       0x7f00, 0xf0, 0x8000, 10);
}

static int sm5708_get_soc_cycle(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_SOC_CYCLE,
			       0x3ff, 0x0, 0x0, 1);
}

static int sm5708_get_device_id(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_DEVICE_ID,
			       0xffff, 0x0, 0x0, 1);
}

static int sm5708_get_soc(struct sm5708_battery *bat)
{
	return sm5708_read_ext(bat, SM5708_REG_SOC,
			       0xff00, 0xff, 0x0, 1);
}

static bool sm5708_fg_check_reg_init_need(struct sm5708_battery *bat)
{
	int ret = sm5708_read_ext(bat, SM5708_REG_FG_OP_STATUS,
			          0xffff, 0x0, 0x0, 1);

	if (ret < 0)
		return false;

	return ((ret & INIT_CHECK_MASK) != DISABLE_RE_INIT);
}

static bool sm5708_is_charging(struct sm5708_battery *bat)
{
	return false;
}

static void sm5708_vbatocv_check(struct sm5708_battery *bat)
{
	int rs_val;
	bool set_auto = false;

	if ((abs(bat->last_current) < 40) ||
	     (sm5708_is_charging(bat) &&
	     (bat->last_current < (bat->data->top_off)) &&
	     (bat->last_current > (bat->data->top_off/3)) &&
	     (bat->last_soc >= 20))) {
		if (abs(bat->last_ocv - bat->last_voltage) > 30 &&
		    bat->iocv_error_count < 5) /* 30mV over */
			bat->iocv_error_count++;
	} else {
		bat->iocv_error_count = 0;
	}

	if (bat->iocv_error_count > 5 &&
	    abs(bat->prev_voltage - bat->last_voltage) > 15) /* 15mV over */
			bat->iocv_error_count = 0;

	if (bat->iocv_error_count > 5) {
		rs_val = bat->data->rs_value[0];
	} else {
		int voltage = MAXVAL(bat->last_voltage, bat->prev_voltage);
		int temp_poff, temp_poff_offset, diff;
		int temp = 20; // TODO

		if (temp > 15) {
			temp_poff = bat->data->n_temp_poff;
			temp_poff_offset = bat->data->n_temp_poff_offset;
		} else {
			temp_poff = bat->data->l_temp_poff;
			temp_poff_offset = bat->data->l_temp_poff_offset;
		}

		diff = temp_poff - voltage;

		set_auto = 0;

		if (diff > 0)
			rs_val = bat->data->rs_value[0];
		else if (diff > temp_poff_offset)
			rs_val = bat->data->rs_value[0] / 2;
		else
			set_auto = true;
	}

	if (set_auto) {
		/* mode change to mix RS auto mode */
		regmap_update_bits(bat->regmap, SM5708_REG_CNTL,
				ENABLE_MIX_MODE | ENABLE_RS_MAN_MODE, ENABLE_MIX_MODE);
	} else {
		/* run update set */
		regmap_write(bat->regmap, SM5708_REG_PARAM_RUN_UPDATE, 1);
		regmap_write(bat->regmap, SM5708_REG_RS_MAN, rs_val);
		regmap_write(bat->regmap, SM5708_REG_PARAM_RUN_UPDATE, 0);

		/* mode change */
		regmap_update_bits(bat->regmap, SM5708_REG_CNTL,
				   ENABLE_MIX_MODE | ENABLE_RS_MAN_MODE,
				   ENABLE_MIX_MODE | ENABLE_RS_MAN_MODE);
	}

	bat->prev_voltage = bat->last_voltage;
	bat->prev_voltage = bat->last_voltage;
}

static int sm5708_cal_carc (struct sm5708_battery *bat)
{
	int p_curr_cal = 0, n_curr_cal = 0, p_delta_cal = 0, n_delta_cal = 0;
	int p_fg_delta_cal = 0, n_fg_delta_cal = 0, curr_offset = 0;
	int fg_delta_volcal = 0, pn_volt_slope = 0, volt_offset = 0;
	int temp_gap, fg_temp_gap;

	sm5708_vbatocv_check(bat);

	if (bat->last_current > 0 || (bat->last_current < -2000))
		regmap_write(bat->regmap, SM5708_REG_RS_MIX_FACTOR, 327);
	else
		regmap_write(bat->regmap, SM5708_REG_RS_MIX_FACTOR, 326);

	fg_temp_gap = (bat->last_temp/10) - bat->data->temp_std;

	volt_offset = sm5708_read_ext(bat, SM5708_REG_VOLT_CAL, 0xff, 0, 0, 1);

	fg_delta_volcal = APPLY_FACTOR(fg_temp_gap, bat->data->fg_temp_volcal_fact);
	pn_volt_slope = (bat->data->volt_cal & 0xFF00) + (fg_delta_volcal << 8);

	regmap_write(bat->regmap, SM5708_REG_VOLT_CAL, pn_volt_slope | volt_offset);

	curr_offset = sm5708_read_ext(bat, SM5708_REG_CURR_OFF, 0x7f, 0, 0x80, 1);

	if (fg_temp_gap < 0)
		curr_offset += APPLY_FACTOR(-fg_temp_gap, bat->data->low_fg_temp_offset_fact);
	else
		curr_offset += APPLY_FACTOR(fg_temp_gap, bat->data->high_fg_temp_offset_fact);

	regmap_write(bat->regmap, SM5708_REG_CURR_OFF,
			curr_offset < 0 ? -curr_offset | 0x80 : curr_offset);

	n_curr_cal = bat->data->n_curr_cal;
	p_curr_cal = bat->data->p_curr_cal;

	if (fg_temp_gap > 0) {
		p_fg_delta_cal = APPLY_FACTOR(fg_temp_gap, bat->data->high_fg_temp_p_cal_fact);
		n_fg_delta_cal = APPLY_FACTOR(fg_temp_gap, bat->data->high_fg_temp_n_cal_fact);
	} else if (fg_temp_gap < 0) {
		fg_temp_gap = -fg_temp_gap;
		p_fg_delta_cal = APPLY_FACTOR(fg_temp_gap, bat->data->low_fg_temp_p_cal_fact);
		n_fg_delta_cal = APPLY_FACTOR(fg_temp_gap, bat->data->low_fg_temp_n_cal_fact);
	}
	p_curr_cal += p_fg_delta_cal;
	n_curr_cal += n_fg_delta_cal;

	// TODO: get temperature from battery's thermal zone
	temp_gap = 25 - bat->data->temp_std;
	if (temp_gap < 0) {
		temp_gap = -temp_gap;
		p_delta_cal = APPLY_FACTOR(temp_gap, bat->data->low_temp_p_cal_fact);
		n_delta_cal = APPLY_FACTOR(temp_gap, bat->data->low_temp_n_cal_fact);
	}
	p_curr_cal += p_delta_cal;
	n_curr_cal += n_delta_cal;

	regmap_write(bat->regmap, SM5708_REG_CURR_P_SLOPE, p_curr_cal);
	regmap_write(bat->regmap, SM5708_REG_CURR_N_SLOPE, n_curr_cal);

	return 0;
}

int sm5708_calculate_iocv(struct sm5708_battery *bat)
{
	bool only_lb = false;
	int roop_start = 0, roop_max = 0, i = 0;
	int v_buffer[6] = {0, 0, 0, 0, 0, 0};
	int i_buffer[6] = {0, 0, 0, 0, 0, 0};
	int i_vset_margin = 0x67;
	int lb_v_avg = 0, cb_v_avg = 0, lb_v_set = 0;
	int cb_v_set = 0;
	int lb_i_n_v_max = 0, cb_i_n_v_max = 0;
	int ret = 0;

	regmap_read(bat->regmap, SM5708_REG_END_V_IDX, &ret);
	if (!(ret & 0x10))
		only_lb = true;

	for (roop_start = SM5708_REG_IOCV_B_C_MIN, roop_max = 6;
	     roop_start == SM5708_REG_IOCV_B_C_MIN;
	     roop_start = SM5708_REG_IOCV_B_L_MIN, roop_max = MINVAL(6, (ret & 0xf))) {
		{
			int v_max = -0xffff, v_min = 0xffff, v_sum = 0, v_ret;
			int i_max = -0xffff, i_min = 0xffff, i_sum = 0, i_ret;
			for (i = roop_start; i < roop_start + roop_max; i++) {
				v_buffer[i-roop_start] = v_ret = sm5708_read_ext(bat, i,
										0xffff, 0, 0, 1);
				i_buffer[i-roop_start] = i_ret = sm5708_read_ext(bat, i + 0x10,
										0x3fff, 0, 0x4000, 1);

				v_max = MAXVAL(v_max, v_ret);
				i_max = MAXVAL(i_max, i_ret);
				v_min = MINVAL(v_min, v_ret);
				i_min = MINVAL(i_min, i_ret);
				v_sum += v_ret;
				i_sum += i_ret;

				if (abs(i_ret) > i_vset_margin && i_ret <= 0) {
					if (roop_start == SM5708_REG_IOCV_B_L_MIN)
						lb_i_n_v_max = MAXVAL(lb_i_n_v_max, v_ret);
					else
						cb_i_n_v_max = MAXVAL(cb_i_n_v_max, v_ret);
				}
			}

			v_sum -= v_max - v_min;
			i_sum -= i_max - i_min;

			if (roop_start == SM5708_REG_IOCV_B_L_MIN)
				lb_v_avg = v_sum / (roop_max - 2);
			else
				cb_v_avg = v_sum / (roop_max - 2);
		}

		/* lb_vset start */
		if (roop_start == SM5708_REG_IOCV_B_L_MIN) {
			int i;
			for (i = 1, lb_v_set = lb_v_avg;
			     i < 4 && abs(i_buffer[roop_max-i]) < i_vset_margin; i++)
				lb_v_set = MAXVAL(lb_v_set, v_buffer[roop_max-i]);

			lb_v_set = MAXVAL(lb_i_n_v_max, lb_v_set);
		} else {
			int i, use_cb = 0;
			for (i = 1, cb_v_set = cb_v_avg;
			     i < 4 && abs(i_buffer[roop_max-i]) < i_vset_margin; i++) {
				use_cb = 1;
				cb_v_set = MAXVAL(cb_v_set, v_buffer[roop_max-i]);
			}

			cb_v_set = MAXVAL(cb_i_n_v_max, cb_v_set);
			if (use_cb)
				return cb_v_set;
		}
	}

	return lb_v_set;
}



static const u32 a6plte_reg_init[] = {
	SM5708_REG_RESET,	    SM5708_FG_INIT_MARK,
	SM5708_REG_PARAM_CTRL,	    SM5708_FG_PARAM_UNLOCK_CODE,
	SM5708_REG_RCE0,	    1249,
	SM5708_REG_RCE1,	    998,
	SM5708_REG_RCE2,	    471,
	SM5708_REG_DTCD,	    1,
	SM5708_REG_AUTO_RS_MAN,	    122,
	SM5708_REG_VIT_PERIOD,	    13574,
	SM5708_REG_PARAM_CTRL,	    SM5708_FG_PARAM_UNLOCK_CODE | SM5708_FG_TABLE_LEN,
	BULK_START | SM5708_REG_TABLE_START,
	0x1400, 0x1b33, 0x1ccd, 0x1d6b, 0x1d97, 0x1ddb, 0x1e28, 0x1e75,
	0x1ecd, 0x1f62, 0x1fa8, 0x1fd3, 0x2064, 0x20e9, 0x225a, 0x2400,
	0x0000, 0x0031, 0x00d2, 0x015e, 0x0358, 0x04be, 0x078a, 0x0bbc,
	0x0e88, 0x10a5, 0x120b, 0x1371, 0x14d7, 0x16f3, 0x1b6b, 0x1b72,
	BULK_STOP,
	SM5708_REG_RS_MIX_FACTOR,   326,
	SM5708_REG_RS_MAX,	    14336,
	SM5708_REG_RS_MIN,	    122,
	SM5708_REG_MIX_RATE,	    1027,
	SM5708_REG_MIX_INIT_BLANK,  4,
	SM5708_REG_VOLT_CAL,	    0x8000,
	SM5708_REG_CURR_OFF,	    0,
	SM5708_REG_CURR_P_SLOPE,    138,
	SM5708_REG_CURR_N_SLOPE,    138,
	SM5708_REG_MISC,	    96,
	SM5708_REG_TOPOFFSOC,	    3,
	/* INIT_last -	control register set */
	SM5708_REG_CNTL,	    ENABLE_MIX_MODE | ENABLE_TEMP_MEASURE | ENABLE_MANUAL_OCV,
	SM5708_REG_PARAM_CTRL,	    SM5708_FG_PARAM_LOCK_CODE | SM5708_FG_TABLE_LEN,
};

static struct sm5708_battery_data a6plte_battery_info = {
	.rs_value			= { 0x7a, 0x147, 0x146, 0x3800, 0x7a },
	.n_temp_poff			= 3400,
	.n_temp_poff_offset		= 50,
	.l_temp_poff			= 3350,
	.l_temp_poff_offset		= 50,
	.value_v_alarm			= 3200,
	.topoff_soc			= 3,
	.top_off			= 350,
	.volt_cal			= 0x8000,
	.p_curr_cal			= 138,
	.n_curr_cal			= 138,
	.temp_std			= 25,
	.fg_temp_volcal_fact		= DEF_FACTOR(1, 15),
	.high_fg_temp_offset_fact	= DEF_FACTOR(1, 11),
	.low_fg_temp_offset_fact	= DEF_FACTOR(-1, 8),
	.high_fg_temp_n_cal_fact	= DEF_FACTOR(-1, 11),
	.high_fg_temp_p_cal_fact	= DEF_FACTOR(1, 6),
	.low_fg_temp_p_cal_fact		= DEF_FACTOR(1, 6),
	.low_fg_temp_n_cal_fact		= DEF_FACTOR(1, 9),
	.low_temp_p_cal_fact		= DEF_FACTOR(2, 1),
	.low_temp_n_cal_fact		= DEF_FACTOR(2, 1),
	.capacity			= 3500,
};

static bool sm5708_fg_reg_init(struct sm5708_battery *bat, int is_surge)
{
	int i, j, value;
	u32 reg = 0;
	const u32 *seq = bat->data->reg_init_seq;
	u32 len = bat->data->reg_init_seq_len;

	if (len < 1)
		return -EINVAL;

	for (i = 0; i < (len - 1);) {
		reg = seq[i];

		if (reg & BULK_START) {
			reg &= 0xffff;
			for (j = i + 1; j < len; j++) {
				if (seq[j] & BULK_STOP)
					i = j + 1;
				else
					regmap_write(bat->regmap, reg++, seq[j]);
			}
		} else {
			regmap_write(bat->regmap, reg, seq[i + 1]);
			i += 2;
		}
	}

	/* surge reset defence */
	if (is_surge) {
		value = (bat->last_ocv << 8) / 125;
	} else {
		value = sm5708_calculate_iocv(bat);
	}

	regmap_write(bat->regmap, SM5708_REG_IOCV_MAN, value);

	msleep(20);

	regmap_write(bat->regmap, SM5708_REG_RESERVED, (1 << 4) & SM5708_BATTERY_VERSION);

	return 1;
}

static bool sm5708_battery_init(struct sm5708_battery *bat, bool is_surge)
{
	int ret;

	ret = sm5708_get_device_id(bat);

	dev_info(bat->dev, "Device ID %x\n", ret);

	regmap_write(bat->regmap, SM5708_REG_SOC_CYCLE_CFG, 0
			| 3 << SM5708_CYCLE_LIMIT_CNTL_SHIFT
			| 7 << SM5708_CYCLE_HIGH_LIMIT_SHIFT
			| 1 << SM5708_CYCLE_LOW_LIMIT_SHIFT);

	if (sm5708_fg_check_reg_init_need(bat)) {
		sm5708_fg_reg_init(bat, is_surge);
	}

	return true;
}

void sm5708_battery_reset(struct sm5708_battery *bat, bool is_surge)
{
	unsigned int code = is_surge ? SW_RESET_OTP_CODE : SW_RESET_CODE;
	int ret, retries = 3;
	do {
		ret = regmap_write(bat->regmap, SM5708_REG_RESET, code);
		msleep(50);
	} while (ret < 0 && retries--);

	msleep(800);

	sm5708_battery_init(bat, is_surge);
}


static int sm5708_battery_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct sm5708_battery *bat = power_supply_get_drvdata(psy);
	int curr;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = sm5708_get_vbat(bat);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		val->intval = sm5708_get_ocv(bat);
		break;
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		val->intval = sm5708_get_soc_cycle(bat);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = sm5708_get_curr(bat);
		break;
	case POWER_SUPPLY_PROP_TEMP:
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		val->intval = sm5708_get_temperature(bat);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		bat->last_current = sm5708_get_curr(bat) / 1000;
		bat->last_voltage = sm5708_get_vbat(bat) / 1000;
		bat->last_ocv = sm5708_get_ocv(bat) / 1000;
		bat->last_temp = sm5708_get_temperature(bat);
		bat->last_soc = sm5708_get_soc(bat);

		sm5708_cal_carc(bat);

		val->intval = clamp(sm5708_get_soc(bat), 0, 100);
		break;
	case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		val->intval = bat->data->capacity * 1000;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = bat->data->capacity * 10 * sm5708_get_soc(bat);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		curr = sm5708_get_curr(bat);
		if (power_supply_am_i_supplied(psy)) {
			if (sm5708_get_soc(bat) > 95)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if (curr > 5000)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
		} else {
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static const struct regmap_config sm5708_battery_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 16,
	.val_format_endian = REGMAP_ENDIAN_LITTLE,
	.max_register	= SM5708_REG_TABLE_START + 32,
};

static const struct power_supply_desc sm5708_battery_desc = {
	.name		= "sm5708-battery",
	.type		= POWER_SUPPLY_TYPE_BATTERY,
	.get_property	= sm5708_battery_get_property,
	.properties	= sm5708_battery_properties,
	.num_properties	= ARRAY_SIZE(sm5708_battery_properties),
};

static int sm5708_battery_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct sm5708_battery *battery;
	struct power_supply_config psy_config = { 0 };

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	battery = devm_kzalloc(&client->dev, sizeof(*battery), GFP_KERNEL);
	if (!battery)
		return -ENOMEM;

	i2c_set_clientdata(client, battery);
	psy_config.drv_data = battery;

	battery->dev = &client->dev;
	battery->client = client;
	battery->regmap = devm_regmap_init_i2c(client,
			&sm5708_battery_regmap_config);
	battery->data = (struct sm5708_battery_data*) of_device_get_match_data(battery->dev);

	if (!sm5708_battery_init(battery, false)) {
		dev_err(&client->dev, "Failed to Initialize battery\n");
		return -EINVAL;
	}

	battery->psy = devm_power_supply_register(&client->dev,
			&sm5708_battery_desc, &psy_config);
	if (IS_ERR(battery->psy))
		return PTR_ERR(battery->psy);

	return 0;
}

static const struct i2c_device_id sm5708_id[] = {
	{ "sm5708-battery" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sm5708_id);

static const struct of_device_id sm5708_of_match[] = {
	{ .compatible = "sm5708-battery,a6plte", .data = &a6plte_battery_info },
	{ },
};
MODULE_DEVICE_TABLE(of, sm5708_of_match);

static struct i2c_driver sm5708_fuelgauge_driver = {
	.driver = {
		   .name = "sm5708-battery",
		   .owner = THIS_MODULE,
		   .of_match_table = sm5708_of_match,
	},
	.probe	    = sm5708_battery_probe,
	.id_table   = sm5708_id,
};

module_i2c_driver(sm5708_fuelgauge_driver);

MODULE_DESCRIPTION("Siliconmitus SM5708 Fuel Gauge Driver");
MODULE_LICENSE("GPL");
