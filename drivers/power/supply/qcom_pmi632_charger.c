// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016-2019 The Linux Foundation. All rights reserved.
 * Copyright (c) 2026, Marc Lainez <marc.lainez@gmail.com>
 *
 * This driver is for the SMB5 switch-mode battery charger found in the
 * Qualcomm PMI632 PMIC.  It is modeled on the SMB2 driver
 * (qcom_pmi8998_charger.c by Caleb Connolly) but adapted for the SMB5
 * register layout:
 *
 *   - Charge status enum is reordered (INHIBIT=0 .. DISABLE=7)
 *   - ICL and power-path status live in the DCDC block (+0x100)
 *   - Current steps are 50 mA (not 25 mA)
 *   - Float voltage steps are 10 mV from 3600 mV
 *   - BAT_OV is at BIT(1) in CHARGER_STATUS_2
 *   - Type-C is handled by a separate driver; we don't touch it
 */

#include <linux/bits.h>
#include <linux/devm-helpers.h>
#include <linux/iio/consumer.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_wakeirq.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/types.h>
#include <linux/workqueue.h>

/* ---------- CHGR block (base + 0x000) ----------------------------------- */
#define BATTERY_CHARGER_STATUS_1		0x06
#define BATTERY_CHARGER_STATUS_MASK		GENMASK(2, 0)

#define BATTERY_CHARGER_STATUS_2		0x07
#define CHARGER_ERROR_STATUS_BAT_OV_BIT		BIT(1)
#define BAT_TEMP_STATUS_MASK			GENMASK(3, 2)
#define BAT_TEMP_STATUS_TOO_HOT_BIT		BIT(3)
#define BAT_TEMP_STATUS_TOO_COLD_BIT		BIT(2)

#define BATTERY_CHARGER_STATUS_7		0x0D

#define CHARGING_ENABLE_CMD			0x42
#define CHARGING_ENABLE_CMD_BIT			BIT(0)

#define CHGR_CFG2				0x51
#define RECHG_MASK				GENMASK(2, 1)
#define VBAT_BASED_RECHG_BIT			BIT(2)
#define CHARGER_INHIBIT_BIT			BIT(0)

#define FAST_CHARGE_CURRENT_CFG			0x61
#define FAST_CHARGE_CURRENT_SETTING_MASK	GENMASK(7, 0)

#define FLOAT_VOLTAGE_CFG			0x70
#define FLOAT_VOLTAGE_SETTING_MASK		GENMASK(7, 0)

#define JEITA_EN_CFG				0x90
#define JEITA_EN_HOT_SL_FCV_BIT		BIT(3)
#define JEITA_EN_COLD_SL_FCV_BIT		BIT(2)
#define JEITA_EN_HOT_SL_CCC_BIT		BIT(1)
#define JEITA_EN_COLD_SL_CCC_BIT		BIT(0)

/* ---------- DCDC block (base + 0x100) ----------------------------------- */
#define DCDC_ICL_STATUS				0x107
#define DCDC_POWER_PATH_STATUS			0x10B
#define P_PATH_USBIN_SUSPEND_STS_BIT		BIT(6)
#define P_PATH_USE_USBIN_BIT			BIT(4)
#define P_PATH_POWER_PATH_MASK			GENMASK(2, 1)
#define P_PATH_VALID_INPUT_POWER_SOURCE_STS_BIT	BIT(0)

/* ---------- USBIN block (base + 0x300) ---------------------------------- */
#define APSD_STATUS				0x307
#define APSD_DTC_STATUS_DONE_BIT		BIT(0)

#define APSD_RESULT_STATUS			0x308
#define APSD_RESULT_STATUS_MASK			GENMASK(6, 0)
#define QC_3P0_BIT				BIT(6)
#define QC_2P0_BIT				BIT(5)
#define FLOAT_CHARGER_BIT			BIT(4)
#define DCP_CHARGER_BIT				BIT(3)
#define CDP_CHARGER_BIT				BIT(2)
#define OCP_CHARGER_BIT				BIT(1)
#define SDP_CHARGER_BIT				BIT(0)

#define USBIN_CMD_IL				0x340
#define USBIN_SUSPEND_BIT			BIT(0)

#define CMD_APSD				0x341
#define APSD_RERUN_BIT				BIT(0)

#define USBIN_OPTIONS_1_CFG			0x362
#define HVDCP_EN_BIT				BIT(2)
#define BC1P2_SRC_DETECT_BIT			BIT(3)

#define USBIN_OPTIONS_2_CFG			0x363
#define FLOAT_OPTIONS_MASK			GENMASK(2, 0)
#define FLOAT_DIS_CHGING_CFG_BIT		BIT(2)

#define USBIN_ICL_OPTIONS			0x366
#define CFG_USB3P0_SEL_BIT			BIT(2)
#define USB51_MODE_BIT				BIT(1)
#define USBIN_MODE_CHG_BIT			BIT(0)

#define USBIN_CURRENT_LIMIT_CFG			0x370
#define USBIN_CURRENT_LIMIT_MASK		GENMASK(7, 0)

#define USBIN_AICL_OPTIONS_CFG			0x380
#define SUSPEND_ON_COLLAPSE_USBIN_BIT		BIT(7)
#define USBIN_AICL_START_AT_MAX_BIT		BIT(5)
#define USBIN_AICL_ADC_EN_BIT			BIT(3)
#define USBIN_AICL_EN_BIT			BIT(2)
#define USBIN_HV_COLLAPSE_RESPONSE_BIT		BIT(1)
#define USBIN_LV_COLLAPSE_RESPONSE_BIT		BIT(0)

#define USBIN_5V_AICL_THRESHOLD_CFG		0x381
#define USBIN_5V_AICL_THRESHOLD_CFG_MASK	GENMASK(2, 0)

#define USBIN_CONT_AICL_THRESHOLD_CFG		0x384
#define USBIN_CONT_AICL_THRESHOLD_CFG_MASK	GENMASK(5, 0)

/* ---------- MISC block (base + 0x600) ----------------------------------- */
#define BARK_BITE_WDOG_PET			0x643
#define BARK_BITE_WDOG_PET_BIT			BIT(0)

#define WD_CFG					0x651
#define WATCHDOG_TRIGGER_AFP_EN_BIT		BIT(7)
#define BARK_WDOG_INT_EN_BIT			BIT(6)
#define WDOG_TIMER_EN_ON_PLUGIN_BIT		BIT(1)

#define SNARL_BARK_BITE_WD_CFG			0x653
#define BITE_WDOG_DISABLE_CHARGING_CFG_BIT	BIT(7)
#define BARK_WDOG_TIMEOUT_MASK			GENMASK(3, 2)
#define BITE_WDOG_TIMEOUT_MASK			GENMASK(1, 0)

/* ---------- Scale factors ----------------------------------------------- */
#define SDP_CURRENT_UA				500000
#define CDP_CURRENT_UA				1500000
#define DCP_CURRENT_UA				1500000
#define CURRENT_MAX_UA				DCP_CURRENT_UA

/* PMI632/SMB5 uses 50 mA steps (SMB2 uses 25 mA) */
#define CURRENT_SCALE_FACTOR			50000

/* PMI632/SMB5: float voltage = 3600 mV + (reg_val * 10 mV) */
#define FLOAT_VOLTAGE_BASE_UV			3600000
#define FLOAT_VOLTAGE_STEP_UV			10000

/*
 * SMB5 charge status values — note the different ordering from SMB2.
 * SMB2: TRICKLE=0,PRE=1,FAST=2,FULLON=3,TAPER=4,TERMINATE=5,INHIBIT=6,DISABLE=7
 * SMB5: INHIBIT=0,TRICKLE=1,PRE=2,FULLON=3,TAPER=4,TERMINATE=5,PAUSE=6,DISABLE=7
 */
enum smb5_charger_status {
	INHIBIT_CHARGE = 0,
	TRICKLE_CHARGE,
	PRE_CHARGE,
	FULLON_CHARGE,
	TAPER_CHARGE,
	TERMINATE_CHARGE,
	PAUSE_CHARGE,
	DISABLE_CHARGE,
};

struct smb5_register {
	u16 addr;
	u8 mask;
	u8 val;
};

/**
 * struct smb5_chip - PMI632 SMB5 charger chip data
 * @dev:		Device reference
 * @name:		Platform device name
 * @base:		Base address for charger registers
 * @regmap:		Parent SPMI regmap
 * @batt_info:		Battery data from DT
 * @status_change_work:	Worker to handle plug/unplug events
 * @cable_irq:		USB plugin IRQ
 * @usb_in_i_chan:	USB-in current IIO channel
 * @usb_in_v_chan:	USB-in voltage IIO channel
 * @chg_psy:		Charger power supply
 */
struct smb5_chip {
	struct device *dev;
	const char *name;
	unsigned int base;
	struct regmap *regmap;
	struct power_supply_battery_info *batt_info;

	struct delayed_work status_change_work;
	int cable_irq;

	struct iio_channel *usb_in_i_chan;
	struct iio_channel *usb_in_v_chan;

	struct power_supply *chg_psy;
};

static enum power_supply_property smb5_properties[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_MODEL_NAME,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_USB_TYPE,
};

static int smb5_get_prop_usb_online(struct smb5_chip *chip, int *val)
{
	unsigned int stat;
	int rc;

	/* On SMB5 the power-path status is in the DCDC block (+0x100) */
	rc = regmap_read(chip->regmap,
			 chip->base + DCDC_POWER_PATH_STATUS, &stat);
	if (rc < 0) {
		dev_err(chip->dev, "Couldn't read power path status: %d\n", rc);
		return rc;
	}

	*val = (stat & P_PATH_USE_USBIN_BIT) &&
	       (stat & P_PATH_VALID_INPUT_POWER_SOURCE_STS_BIT);
	return 0;
}

static int smb5_apsd_get_charger_type(struct smb5_chip *chip, int *val)
{
	unsigned int apsd_stat, stat;
	int usb_online = 0;
	int rc;

	rc = smb5_get_prop_usb_online(chip, &usb_online);
	if (!usb_online) {
		*val = POWER_SUPPLY_USB_TYPE_UNKNOWN;
		return rc;
	}

	rc = regmap_read(chip->regmap, chip->base + APSD_STATUS, &apsd_stat);
	if (rc < 0) {
		dev_err(chip->dev, "Failed to read APSD status: %d\n", rc);
		return rc;
	}
	if (!(apsd_stat & APSD_DTC_STATUS_DONE_BIT)) {
		dev_dbg(chip->dev, "APSD not ready\n");
		return -EAGAIN;
	}

	rc = regmap_read(chip->regmap,
			 chip->base + APSD_RESULT_STATUS, &stat);
	if (rc < 0) {
		dev_err(chip->dev, "Failed to read APSD result: %d\n", rc);
		return rc;
	}

	stat &= APSD_RESULT_STATUS_MASK;

	if (stat & CDP_CHARGER_BIT)
		*val = POWER_SUPPLY_USB_TYPE_CDP;
	else if (stat & (DCP_CHARGER_BIT | OCP_CHARGER_BIT |
			 FLOAT_CHARGER_BIT))
		*val = POWER_SUPPLY_USB_TYPE_DCP;
	else
		*val = POWER_SUPPLY_USB_TYPE_SDP;

	return 0;
}

static int smb5_get_prop_status(struct smb5_chip *chip, int *val)
{
	unsigned char stat[2];
	int usb_online = 0;
	int rc;

	rc = smb5_get_prop_usb_online(chip, &usb_online);
	if (!usb_online) {
		*val = POWER_SUPPLY_STATUS_DISCHARGING;
		return rc;
	}

	rc = regmap_bulk_read(chip->regmap,
			      chip->base + BATTERY_CHARGER_STATUS_1, stat, 2);
	if (rc < 0) {
		dev_err(chip->dev, "Failed to read charging status: %d\n", rc);
		return rc;
	}

	if (stat[1] & CHARGER_ERROR_STATUS_BAT_OV_BIT) {
		*val = POWER_SUPPLY_STATUS_NOT_CHARGING;
		return 0;
	}

	switch (stat[0] & BATTERY_CHARGER_STATUS_MASK) {
	case TRICKLE_CHARGE:
	case PRE_CHARGE:
	case FULLON_CHARGE:
	case TAPER_CHARGE:
		*val = POWER_SUPPLY_STATUS_CHARGING;
		return 0;
	case DISABLE_CHARGE:
	case PAUSE_CHARGE:
		*val = POWER_SUPPLY_STATUS_NOT_CHARGING;
		return 0;
	case TERMINATE_CHARGE:
	case INHIBIT_CHARGE:
		*val = POWER_SUPPLY_STATUS_FULL;
		return 0;
	default:
		*val = POWER_SUPPLY_STATUS_UNKNOWN;
		return 0;
	}
}

static int smb5_get_current_limit(struct smb5_chip *chip, unsigned int *val)
{
	int rc;

	rc = regmap_read(chip->regmap, chip->base + DCDC_ICL_STATUS, val);
	if (rc >= 0)
		*val *= CURRENT_SCALE_FACTOR;
	return rc;
}

static int smb5_set_current_limit(struct smb5_chip *chip, unsigned int val)
{
	unsigned char val_raw;

	if (val > 3000000) {
		dev_err(chip->dev,
			"Can't set current limit higher than 3000000uA\n");
		return -EINVAL;
	}
	val_raw = val / CURRENT_SCALE_FACTOR;

	return regmap_write(chip->regmap,
			    chip->base + USBIN_CURRENT_LIMIT_CFG, val_raw);
}

static void smb5_status_change_work(struct work_struct *work)
{
	unsigned int charger_type, current_ua;
	int usb_online = 0;
	int count, rc;
	struct smb5_chip *chip;

	chip = container_of(work, struct smb5_chip, status_change_work.work);

	smb5_get_prop_usb_online(chip, &usb_online);
	if (!usb_online)
		return;

	for (count = 0; count < 3; count++) {
		dev_dbg(chip->dev, "get charger type retry %d\n", count);
		rc = smb5_apsd_get_charger_type(chip, &charger_type);
		if (rc != -EAGAIN)
			break;
		msleep(100);
	}

	if (rc < 0 && rc != -EAGAIN) {
		dev_err(chip->dev, "get charger type failed: %d\n", rc);
		return;
	}

	if (rc < 0) {
		rc = regmap_update_bits(chip->regmap,
					chip->base + CMD_APSD,
					APSD_RERUN_BIT, APSD_RERUN_BIT);
		schedule_delayed_work(&chip->status_change_work,
				      msecs_to_jiffies(1000));
		dev_dbg(chip->dev, "APSD not ready, rerunning\n");
		return;
	}

	switch (charger_type) {
	case POWER_SUPPLY_USB_TYPE_CDP:
		current_ua = CDP_CURRENT_UA;
		break;
	case POWER_SUPPLY_USB_TYPE_DCP:
		current_ua = DCP_CURRENT_UA;
		break;
	case POWER_SUPPLY_USB_TYPE_SDP:
	default:
		current_ua = SDP_CURRENT_UA;
		break;
	}

	smb5_set_current_limit(chip, current_ua);
	power_supply_changed(chip->chg_psy);
}

static int smb5_get_iio_chan(struct smb5_chip *chip, struct iio_channel *chan,
			     int *val)
{
	int rc;
	union power_supply_propval status;

	rc = power_supply_get_property(chip->chg_psy, POWER_SUPPLY_PROP_STATUS,
				       &status);
	if (rc < 0 || status.intval != POWER_SUPPLY_STATUS_CHARGING) {
		*val = 0;
		return 0;
	}

	if (IS_ERR(chan))
		return PTR_ERR(chan);

	return iio_read_channel_processed(chan, val);
}

static int smb5_get_prop_health(struct smb5_chip *chip, int *val)
{
	unsigned int stat;
	int rc;

	rc = regmap_read(chip->regmap,
			 chip->base + BATTERY_CHARGER_STATUS_2, &stat);
	if (rc < 0) {
		dev_err(chip->dev, "Couldn't read charger status: %d\n", rc);
		return rc;
	}

	if (stat & CHARGER_ERROR_STATUS_BAT_OV_BIT) {
		*val = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		return 0;
	}

	/*
	 * Do not check BAT_TEMP_STATUS_TOO_HOT/COLD bits here.
	 * Hardware JEITA action is disabled in smb5_init_seq[] and the
	 * JEITA comparator thresholds are left at their (wrong) defaults,
	 * so these status bits are not meaningful.  Battery temperature
	 * health is handled by the fuel gauge driver via its ADC reading.
	 */

	*val = POWER_SUPPLY_HEALTH_GOOD;
	return 0;
}

static int smb5_get_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     union power_supply_propval *val)
{
	struct smb5_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_MANUFACTURER:
		val->strval = "Qualcomm";
		return 0;
	case POWER_SUPPLY_PROP_MODEL_NAME:
		val->strval = chip->name;
		return 0;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return smb5_get_current_limit(chip, &val->intval);
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return smb5_get_iio_chan(chip, chip->usb_in_i_chan,
					&val->intval);
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		return smb5_get_iio_chan(chip, chip->usb_in_v_chan,
					&val->intval);
	case POWER_SUPPLY_PROP_ONLINE:
		return smb5_get_prop_usb_online(chip, &val->intval);
	case POWER_SUPPLY_PROP_STATUS:
		return smb5_get_prop_status(chip, &val->intval);
	case POWER_SUPPLY_PROP_HEALTH:
		return smb5_get_prop_health(chip, &val->intval);
	case POWER_SUPPLY_PROP_USB_TYPE:
		return smb5_apsd_get_charger_type(chip, &val->intval);
	default:
		dev_err(chip->dev, "invalid property: %d\n", psp);
		return -EINVAL;
	}
}

static int smb5_set_property(struct power_supply *psy,
			     enum power_supply_property psp,
			     const union power_supply_propval *val)
{
	struct smb5_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return smb5_set_current_limit(chip, val->intval);
	default:
		dev_err(chip->dev, "No setter for property: %d\n", psp);
		return -EINVAL;
	}
}

static int smb5_property_is_writable(struct power_supply *psy,
				     enum power_supply_property psp)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		return 1;
	default:
		return 0;
	}
}

static irqreturn_t smb5_handle_batt_overvoltage(int irq, void *data)
{
	struct smb5_chip *chip = data;
	unsigned int status;

	regmap_read(chip->regmap,
		    chip->base + BATTERY_CHARGER_STATUS_2, &status);

	if (status & CHARGER_ERROR_STATUS_BAT_OV_BIT) {
		dev_err(chip->dev, "battery overvoltage detected\n");
		power_supply_changed(chip->chg_psy);
	}

	return IRQ_HANDLED;
}

static irqreturn_t smb5_handle_usb_plugin(int irq, void *data)
{
	struct smb5_chip *chip = data;

	power_supply_changed(chip->chg_psy);
	schedule_delayed_work(&chip->status_change_work,
			      msecs_to_jiffies(1500));

	return IRQ_HANDLED;
}

static irqreturn_t smb5_handle_usb_icl_change(int irq, void *data)
{
	struct smb5_chip *chip = data;

	power_supply_changed(chip->chg_psy);

	return IRQ_HANDLED;
}

static irqreturn_t smb5_handle_wdog_bark(int irq, void *data)
{
	struct smb5_chip *chip = data;
	int rc;

	power_supply_changed(chip->chg_psy);

	rc = regmap_write(chip->regmap,
			  chip->base + BARK_BITE_WDOG_PET,
			  BARK_BITE_WDOG_PET_BIT);
	if (rc < 0)
		dev_err(chip->dev, "Couldn't pet the dog: %d\n", rc);

	return IRQ_HANDLED;
}

static const struct power_supply_desc smb5_psy_desc = {
	.name = "pmi632_charger",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = BIT(POWER_SUPPLY_USB_TYPE_SDP) |
		     BIT(POWER_SUPPLY_USB_TYPE_CDP) |
		     BIT(POWER_SUPPLY_USB_TYPE_DCP) |
		     BIT(POWER_SUPPLY_USB_TYPE_UNKNOWN),
	.properties = smb5_properties,
	.num_properties = ARRAY_SIZE(smb5_properties),
	.get_property = smb5_get_property,
	.set_property = smb5_set_property,
	.property_is_writeable = smb5_property_is_writable,
};

/*
 * Hardware init sequence for PMI632 SMB5.
 *
 * Compared to SMB2, we skip registers that don't exist on SMB5:
 *   - AICL_RERUN_TIME_CFG
 *   - TYPE_C_INTRPT_ENB_SOFTWARE_CTRL (Type-C handled by separate driver)
 *   - TYPE_C_CFG
 *   - OTG_CFG (different offset, not needed for sink-only)
 *   - FG_UPDATE_CFG_2_SEL
 *   - PRE_CHARGE_CURRENT_CFG
 *   - OTG_ENG_OTG_CFG / DC_ENG_SSUPPLY_CFG2
 *   - STAT_CFG
 */
static const struct smb5_register smb5_init_seq[] = {
	/*
	 * Disable hardware JEITA temperature compensation.  Without
	 * properly configured JEITA thresholds the PMIC's internal
	 * comparator can latch a spurious "too hot" status, which
	 * causes the health property to report OVERHEAT even when the
	 * battery temperature is normal.
	 */
	{ .addr = JEITA_EN_CFG,
	  .mask = JEITA_EN_HOT_SL_FCV_BIT | JEITA_EN_COLD_SL_FCV_BIT |
		  JEITA_EN_HOT_SL_CCC_BIT | JEITA_EN_COLD_SL_CCC_BIT,
	  .val = 0 },
	/* Disable HVDCP — we only handle BC1.2 charger types */
	{ .addr = USBIN_OPTIONS_1_CFG, .mask = HVDCP_EN_BIT, .val = 0 },
	/* Enable charging */
	{ .addr = CHARGING_ENABLE_CMD,
	  .mask = CHARGING_ENABLE_CMD_BIT,
	  .val = CHARGING_ENABLE_CMD_BIT },
	/*
	 * Use VBAT-based recharge and enable charger inhibit.
	 * On SMB5, CHGR_CFG2 upper bits differ from SMB2 —
	 * only the recharge and inhibit bits are safe to touch.
	 */
	{ .addr = CHGR_CFG2,
	  .mask = RECHG_MASK | CHARGER_INHIBIT_BIT,
	  .val = VBAT_BASED_RECHG_BIT | CHARGER_INHIBIT_BIT },
	/* Set SDP default to 500mA USB 2.0 port */
	{ .addr = USBIN_ICL_OPTIONS,
	  .mask = USB51_MODE_BIT | USBIN_MODE_CHG_BIT,
	  .val = USB51_MODE_BIT },
	/* Disable watchdog */
	{ .addr = SNARL_BARK_BITE_WD_CFG, .mask = 0xff, .val = 0 },
	{ .addr = WD_CFG,
	  .mask = WATCHDOG_TRIGGER_AFP_EN_BIT | WDOG_TIMER_EN_ON_PLUGIN_BIT |
		  BARK_WDOG_INT_EN_BIT,
	  .val = 0 },
	/* AICL thresholds */
	{ .addr = USBIN_5V_AICL_THRESHOLD_CFG,
	  .mask = USBIN_5V_AICL_THRESHOLD_CFG_MASK,
	  .val = 0x3 },
	{ .addr = USBIN_CONT_AICL_THRESHOLD_CFG,
	  .mask = USBIN_CONT_AICL_THRESHOLD_CFG_MASK,
	  .val = 0x3 },
	/* Enable AICL */
	{ .addr = USBIN_AICL_OPTIONS_CFG,
	  .mask = USBIN_AICL_START_AT_MAX_BIT | USBIN_AICL_ADC_EN_BIT |
		  USBIN_AICL_EN_BIT | SUSPEND_ON_COLLAPSE_USBIN_BIT |
		  USBIN_HV_COLLAPSE_RESPONSE_BIT |
		  USBIN_LV_COLLAPSE_RESPONSE_BIT,
	  .val = USBIN_HV_COLLAPSE_RESPONSE_BIT |
		 USBIN_LV_COLLAPSE_RESPONSE_BIT | USBIN_AICL_EN_BIT },
	/*
	 * Limit fast-charge current to 1A as a safe default.
	 * SMB5 uses 50 mA steps: 1000000 / 50000 = 20.
	 */
	{ .addr = FAST_CHARGE_CURRENT_CFG,
	  .mask = FAST_CHARGE_CURRENT_SETTING_MASK,
	  .val = 1000000 / CURRENT_SCALE_FACTOR },
};

static int smb5_init_hw(struct smb5_chip *chip)
{
	int rc, i;

	for (i = 0; i < ARRAY_SIZE(smb5_init_seq); i++) {
		dev_dbg(chip->dev, "%d: writing 0x%02x to 0x%04x\n", i,
			smb5_init_seq[i].val,
			chip->base + smb5_init_seq[i].addr);
		rc = regmap_update_bits(chip->regmap,
					chip->base + smb5_init_seq[i].addr,
					smb5_init_seq[i].mask,
					smb5_init_seq[i].val);
		if (rc < 0)
			return dev_err_probe(chip->dev, rc,
					     "init command %d failed\n", i);
	}

	return 0;
}

static int smb5_init_irq(struct smb5_chip *chip, int *irq, const char *name,
			 irqreturn_t (*handler)(int irq, void *data))
{
	int irqnum;
	int rc;

	irqnum = platform_get_irq_byname(to_platform_device(chip->dev), name);
	if (irqnum < 0)
		return irqnum;

	rc = devm_request_threaded_irq(chip->dev, irqnum, NULL, handler,
				       IRQF_ONESHOT, name, chip);
	if (rc < 0)
		return dev_err_probe(chip->dev, rc,
				     "Couldn't request irq %s\n", name);

	if (irq)
		*irq = irqnum;

	return 0;
}

static int smb5_probe(struct platform_device *pdev)
{
	struct power_supply_config supply_config = {};
	struct power_supply_desc *desc;
	struct smb5_chip *chip;
	int rc, irq, vfloat_reg;

	chip = devm_kzalloc(&pdev->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &pdev->dev;
	chip->name = pdev->name;

	chip->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!chip->regmap)
		return dev_err_probe(chip->dev, -ENODEV,
				     "failed to locate the regmap\n");

	rc = device_property_read_u32(chip->dev, "reg", &chip->base);
	if (rc < 0)
		return dev_err_probe(chip->dev, rc,
				     "Couldn't read base address\n");

	chip->usb_in_v_chan = devm_iio_channel_get(chip->dev, "usbin_v");
	if (IS_ERR(chip->usb_in_v_chan))
		return dev_err_probe(chip->dev, PTR_ERR(chip->usb_in_v_chan),
				     "Couldn't get usbin_v IIO channel\n");

	chip->usb_in_i_chan = devm_iio_channel_get(chip->dev, "usbin_i");
	if (IS_ERR(chip->usb_in_i_chan))
		return dev_err_probe(chip->dev, PTR_ERR(chip->usb_in_i_chan),
				     "Couldn't get usbin_i IIO channel\n");

	rc = smb5_init_hw(chip);
	if (rc < 0)
		return rc;

	supply_config.drv_data = chip;
	supply_config.of_node = pdev->dev.of_node;

	desc = devm_kzalloc(chip->dev, sizeof(smb5_psy_desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	memcpy(desc, &smb5_psy_desc, sizeof(smb5_psy_desc));
	desc->name = devm_kasprintf(chip->dev, GFP_KERNEL, "%s-charger",
				    (const char *)device_get_match_data(chip->dev));
	if (!desc->name)
		return -ENOMEM;

	chip->chg_psy =
		devm_power_supply_register(chip->dev, desc, &supply_config);
	if (IS_ERR(chip->chg_psy))
		return dev_err_probe(chip->dev, PTR_ERR(chip->chg_psy),
				     "failed to register power supply\n");

	rc = power_supply_get_battery_info(chip->chg_psy, &chip->batt_info);
	if (rc)
		return dev_err_probe(chip->dev, rc,
				     "Failed to get battery info\n");

	rc = devm_delayed_work_autocancel(chip->dev, &chip->status_change_work,
					  smb5_status_change_work);
	if (rc)
		return dev_err_probe(chip->dev, rc,
				     "Failed to init status change work\n");

	/* Program float voltage from battery data */
	vfloat_reg = (chip->batt_info->voltage_max_design_uv -
		      FLOAT_VOLTAGE_BASE_UV) / FLOAT_VOLTAGE_STEP_UV;
	if (vfloat_reg < 0)
		vfloat_reg = 0;
	rc = regmap_update_bits(chip->regmap,
				chip->base + FLOAT_VOLTAGE_CFG,
				FLOAT_VOLTAGE_SETTING_MASK, vfloat_reg);
	if (rc < 0)
		return dev_err_probe(chip->dev, rc,
				     "Couldn't set float voltage\n");

	rc = smb5_init_irq(chip, &irq, "bat-ov",
			   smb5_handle_batt_overvoltage);
	if (rc < 0)
		return rc;

	rc = smb5_init_irq(chip, &chip->cable_irq, "usb-plugin",
			   smb5_handle_usb_plugin);
	if (rc < 0)
		return rc;

	rc = smb5_init_irq(chip, &irq, "usbin-icl-change",
			   smb5_handle_usb_icl_change);
	if (rc < 0)
		return rc;

	rc = smb5_init_irq(chip, &irq, "wdog-bark",
			   smb5_handle_wdog_bark);
	if (rc < 0)
		return rc;

	rc = dev_pm_set_wake_irq(chip->dev, chip->cable_irq);
	if (rc < 0)
		return dev_err_probe(chip->dev, rc,
				     "Couldn't set wake irq\n");

	platform_set_drvdata(pdev, chip);

	/* Kick off initial charger state detection */
	schedule_delayed_work(&chip->status_change_work, 0);

	return 0;
}

static const struct of_device_id smb5_match_id_table[] = {
	{ .compatible = "qcom,pmi632-charger", .data = "pmi632" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, smb5_match_id_table);

static struct platform_driver qcom_spmi_smb5 = {
	.probe = smb5_probe,
	.driver = {
		.name = "qcom-pmi632-charger",
		.of_match_table = smb5_match_id_table,
	},
};

module_platform_driver(qcom_spmi_smb5);

MODULE_AUTHOR("Marc Lainez <marc.lainez@gmail.com>");
MODULE_DESCRIPTION("Qualcomm PMI632 SMB5 Charger Driver");
MODULE_LICENSE("GPL");
