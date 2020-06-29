/*
 * sm5708_charger.c - SM5708 Charger driver
 *
 *  Copyright (C) 2015 Silicon Mitus,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/extcon.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/mfd/core.h>
#include <linux/module.h>
#include <linux/interrupt.h>

#include <linux/mfd/sm5708.h>

enum sm5708_boost_limit {
	SM5708_CHG_BST_IQ3LIMIT_2_0A,
	SM5708_CHG_BST_IQ3LIMIT_2_8A,
	SM5708_CHG_BST_IQ3LIMIT_3_5A,
	SM5708_CHG_BST_IQ3LIMIT_4_0A,
};

enum sm5708_manual_reset_time {
	SM5708_MANUAL_RESET_TIME_7s = 0x1,
	SM5708_MANUAL_RESET_TIME_8s,
	SM5708_MANUAL_RESET_TIME_9s,
};

enum sm5708_watchdog_reset_time {
	SM5708_WATCHDOG_RESET_TIME_30s,
	SM5708_WATCHDOG_RESET_TIME_60s,
	SM5708_WATCHDOG_RESET_TIME_90s,
	SM5708_WATCHDOG_RESET_TIME_120s,
};

enum sm5708_topoff_timer {
	SM5708_TOPOFF_TIMER_10m,
	SM5708_TOPOFF_TIMER_20m,
	SM5708_TOPOFF_TIMER_30m,
	SM5708_TOPOFF_TIMER_45m,
};

enum sm5708_bob_freq {
	SM5708_BUCK_BOOST_FREQ_3MHz     = 0x0,
	SM5708_BUCK_BOOST_FREQ_2_4MHz   = 0x1,
	SM5708_BUCK_BOOST_FREQ_1_5MHz   = 0x2,
	SM5708_BUCK_BOOST_FREQ_1_8MHz   = 0x3,
};


struct sm5708_charger {
	struct device *dev;
	struct device *parent;
	struct regmap *regmap;
	struct power_supply *psy;
	struct extcon_dev *extcon;
	struct notifier_block extcon_nb;

	struct work_struct otg_fail_work;
	struct delayed_work otg_work;

	/* for charging operation handling */
	bool charging_enabled;
	int health;

	struct gpio_desc *enable_gpio;

	unsigned int max_input_current;
	unsigned int max_charge_current;
	unsigned int max_float_voltage;
};

static enum power_supply_property sm5708_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
};

static int sm5708_get_param(struct sm5708_charger *charger, int *out,
			    bool micro, u32 reg, u8 mask, u8 shift,
			    u32 min, u32 max, u32 step)
{
	int ret, regval;
	ret = regmap_read(charger->regmap, reg, &regval);
	if (ret < 0)
		return ret;
	*out = (((regval >> shift) & mask) * step + min);
	if (micro)
		*out *= 1000;
	return 0;
}

static int sm5708_set_param(struct sm5708_charger *charger, unsigned int val,
			    u32 reg, u8 mask, u8 shift,
			    u32 min, u32 max, u32 step)
{
	if (val < min || val > max)
		return -EINVAL;
	val = (val - min) / step;
	return regmap_update_bits(charger->regmap, reg,
				  mask << shift, val << shift);
}

#define GETTER_SETTER(param, reg, min, max, step, mask, shift) \
	static int __maybe_unused \
	sm5708_get_##param(struct sm5708_charger *charger, int *out, bool uv) \
	{								\
		return sm5708_get_param(charger, out, uv, reg, mask,	\
				       shift, min, max, step);		\
	}								\
	static int __maybe_unused					\
	sm5708_set_##param(struct sm5708_charger *charger, int value)	\
	{								\
		return sm5708_set_param(charger, value, reg, mask,	\
				  shift, min, max, step);		\
	}

GETTER_SETTER(input_current, SM5708_REG_VBUSCNTL, 100, 3275, 25, 0xff, 0)
GETTER_SETTER(float_voltage, SM5708_REG_CHGCNTL3, 3990, 4350, 10, 0x3f, 0)
GETTER_SETTER(charge_current, SM5708_REG_CHGCNTL2, 100, 3250, 50, 0x3f, 0)
GETTER_SETTER(topoff_current, SM5708_REG_CHGCNTL4, 100, 475, 25, 0xf, 0)
GETTER_SETTER(aicl_threshold, SM5708_REG_CHGCNTL6, 4500, 4800, 100, 0x3, 6)

static unsigned char sm5708_get_status(struct sm5708_charger *charger,
				unsigned char number) // Register number 1-4
{
	unsigned int reg_val;
	int ret;

	if (number < 1 || number > 4)
		return 0;

	ret = regmap_read(charger->regmap, SM5708_REG_STATUS1 + number - 1, &reg_val);
	if (ret < 0)
		return 0;

	return reg_val;
}

static int sm5708_set_topoff_timer(struct sm5708_charger *charger,
				enum sm5708_topoff_timer topoff_timer)
{
	return regmap_update_bits(charger->regmap,
		SM5708_REG_CHGCNTL7, 0x3 << 3, topoff_timer << 3);
}

static int sm5708_set_autostop(struct sm5708_charger *charger,
				bool enable)
{
	return regmap_update_bits(charger->regmap, SM5708_REG_CHGCNTL3, 0x1 << 6, enable << 6);
}

static int sm5708_select_freq(struct sm5708_charger *charger,
			      enum sm5708_bob_freq freq_index)
{
	return regmap_update_bits(charger->regmap, SM5708_REG_CHGCNTL4, 0x3 << 4, freq_index << 4);
}

static int sm5708_set_aicl(struct sm5708_charger *charger, bool enable)
{
	return regmap_update_bits(charger->regmap, SM5708_REG_CHGCNTL6, 0x1 << 5, enable << 5);
}

static int sm5708_set_boost_limit(struct sm5708_charger *charger,
				  enum sm5708_boost_limit index)
{
	return regmap_update_bits(charger->regmap, SM5708_REG_CHGCNTL5, 0x3, index);
}

static int sm5708_set_autoset(struct sm5708_charger *charger, bool enable)
{
	return regmap_update_bits(charger->regmap, SM5708_REG_CHGCNTL6, 0x1 << 1, enable << 1);
}

static void sm5708_set_charging_enabled(struct sm5708_charger *charger, int enabled)
{
	charger->charging_enabled = enabled;

	if (charger->enable_gpio)
		gpiod_set_value(charger->enable_gpio, !enabled);

	sm5708_request_mode(charger->parent, SM5708_CHARGE, enabled);
}

static int sm5708_get_charger_status(struct sm5708_charger *charger)
{
	unsigned char status2 = sm5708_get_status(charger, 2);

	if (status2 & (SM5708_TOPOFF_STATUS2 | SM5708_DONE_STATUS2))
		return POWER_SUPPLY_STATUS_FULL;
	else if (status2 & SM5708_CHGON_STATUS2)
		return POWER_SUPPLY_STATUS_CHARGING;
	else if (sm5708_get_status(charger, 1) & SM5708_VBUSPOK_STATUS1 &&
		!charger->charging_enabled)
		return  POWER_SUPPLY_STATUS_NOT_CHARGING;
	else
		return POWER_SUPPLY_STATUS_DISCHARGING;
}

static int sm5708_get_charge_type(struct sm5708_charger *charger)
{
	int temp;

	switch (sm5708_get_charger_status(charger)) {
		case POWER_SUPPLY_STATUS_FULL:
			return POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		case POWER_SUPPLY_STATUS_CHARGING:
			sm5708_get_input_current(charger, &temp, false);
			if (temp > 500)
				return POWER_SUPPLY_CHARGE_TYPE_FAST;
			else
				return POWER_SUPPLY_CHARGE_TYPE_STANDARD;
		default:
			break;
	}
	return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

static int sm5708_get_health(struct sm5708_charger *charger)
{
	unsigned char status1 = sm5708_get_status(charger, 1);

	if (status1 & SM5708_VBUSPOK_STATUS1)
		return POWER_SUPPLY_HEALTH_GOOD;
	else if (status1 & SM5708_VBUSOVP_STATUS1)
		return POWER_SUPPLY_HEALTH_OVERVOLTAGE;
	else if (status1 & SM5708_VBUSUVLO_STATUS1)
		return POWER_SUPPLY_HEALTH_DEAD;
	else
		return POWER_SUPPLY_HEALTH_UNKNOWN;
}


static int sm5708_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *val)
{
	struct sm5708_charger *charger = power_supply_get_drvdata(psy);

	switch(psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = !!(sm5708_get_status(charger, 1) & SM5708_VBUSPOK_STATUS1);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = !(sm5708_get_status(charger, 2) & SM5708_NOBAT_STATUS2);
		break;
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = sm5708_get_charger_status(charger);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = sm5708_get_charge_type(charger);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = sm5708_get_health(charger);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT_MAX:
		val->intval = charger->max_charge_current * 1000;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = charger->max_input_current * 1000;
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		return sm5708_get_charge_current(charger, &val->intval, true);
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		return sm5708_get_float_voltage(charger, &val->intval, true);
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return sm5708_get_input_current(charger, &val->intval, true);
	default:
		return -EINVAL;
	}

	return 0;
}

static int sm5708_set_property(struct power_supply *psy,
				enum power_supply_property psp, const union power_supply_propval *val)
{
	struct sm5708_charger *charger = power_supply_get_drvdata(psy);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		sm5708_set_charging_enabled(charger, val->intval == POWER_SUPPLY_STATUS_CHARGING);
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if ((val->intval / 1000) > charger->max_input_current)
			return -EINVAL;
		sm5708_set_input_current(charger, val->intval / 1000);
		break;
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		if ((val->intval / 1000) > charger->max_charge_current)
			return -EINVAL;
		sm5708_set_charge_current(charger, val->intval / 1000);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		if ((val->intval / 1000) > charger->max_float_voltage)
			return -EINVAL;
		sm5708_set_float_voltage(charger, val->intval / 1000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static irqreturn_t sm5708_done_irq(int irq, void *data)
{
	struct sm5708_charger *charger = data;

	power_supply_changed(charger->psy);

	return IRQ_HANDLED;
}

static irqreturn_t sm5708_vbus_irq(int irq, void *data)
{
	struct sm5708_charger *charger = data;
	int health = sm5708_get_health(charger);

	if (charger->health != health) {
		charger->health = health;
		power_supply_changed(charger->psy);
	}

	return IRQ_HANDLED;
}

static irqreturn_t sm5708_otgfail_irq(int irq, void *data)
{
	struct sm5708_charger *charger = data;

	if (sm5708_get_status(charger, 3) & SM5708_OTGFAIL_STATUS3)
		schedule_work(&charger->otg_fail_work);

	return IRQ_HANDLED;
}

static void sm5708_otg_fail_work(struct work_struct *work)
{
	struct sm5708_charger *charger = container_of(work,
			struct sm5708_charger, otg_fail_work);

	cancel_delayed_work_sync(&charger->otg_work);
	sm5708_request_mode(charger->parent, SM5708_OTG, false);
}

static void sm5708_otg_work(struct work_struct *work)
{
	struct sm5708_charger *charger = container_of(work,
			struct sm5708_charger, otg_work.work);

	if (sm5708_get_status(charger, 1) & SM5708_VBUSPOK_STATUS1) {
		sm5708_set_input_current(charger, charger->max_input_current);
		sm5708_set_charge_current(charger, charger->max_charge_current);
	} else {
		sm5708_request_mode(charger->parent, SM5708_CHARGE, false);
		sm5708_request_mode(charger->parent, SM5708_OTG, true);
	}
}

static void sm5708_charger_initialize(struct sm5708_charger *charger)
{
	sm5708_set_topoff_current(charger, 300);
	sm5708_set_topoff_timer(charger, SM5708_TOPOFF_TIMER_45m);
	sm5708_set_autostop(charger, 1);
	sm5708_set_float_voltage(charger, charger->max_float_voltage);
	sm5708_set_aicl_threshold(charger, 4500);
	sm5708_set_aicl(charger, 1);
	sm5708_set_autoset(charger, 0);
	sm5708_set_boost_limit(charger, SM5708_CHG_BST_IQ3LIMIT_3_5A);
	sm5708_select_freq(charger, SM5708_BUCK_BOOST_FREQ_1_5MHz);
}

static int sm5708_extcon_notifier(struct notifier_block *nb,
				  unsigned long event, void *ptr)
{
	struct sm5708_charger *charger = container_of(nb,
			struct sm5708_charger, extcon_nb);

	if (extcon_get_state(charger->extcon, EXTCON_USB_HOST)) {
		schedule_delayed_work(&charger->otg_work, 200);
	} else if (extcon_get_state(charger->extcon, EXTCON_USB)) {
		cancel_delayed_work_sync(&charger->otg_work);
		sm5708_request_mode(charger->parent, SM5708_OTG, false);
		sm5708_request_mode(charger->parent, SM5708_CHARGE, false);
	} else {
		cancel_delayed_work_sync(&charger->otg_work);
		sm5708_set_input_current(charger, 500);
		sm5708_set_charge_current(charger, 500);
		sm5708_request_mode(charger->parent, SM5708_OTG, false);
		sm5708_request_mode(charger->parent, SM5708_CHARGE, false);
	}

	if (extcon_get_state(charger->extcon, EXTCON_CHG_USB_SDP)) {
		sm5708_set_input_current(charger, 500);
		sm5708_set_charge_current(charger, 500);
	} else if (extcon_get_state(charger->extcon, EXTCON_CHG_USB_DCP)) {
		sm5708_set_input_current(charger, charger->max_input_current);
		sm5708_set_charge_current(charger, charger->max_charge_current);
	}

	return NOTIFY_DONE;

}

static int sm5708_parse_dt(struct sm5708_charger *charger)
{
	struct device_node *np = charger->dev->of_node;
	int ret;

	if (of_property_read_u32(np, "float-voltage", &charger->max_float_voltage))
		charger->max_float_voltage = 4200;
	if (of_property_read_u32(np, "input-current", &charger->max_input_current))
		charger->max_input_current = 1500;
	if (of_property_read_u32(np, "charge-current", &charger->max_charge_current))
		charger->max_charge_current = 1800;

	charger->enable_gpio = devm_gpiod_get(charger->dev, "enable", GPIOD_OUT_LOW);
	if (IS_ERR(charger->enable_gpio)) {
		ret = PTR_ERR(charger->enable_gpio);
		dev_err(charger->dev, "Failed to get enable-gpios: %d\n", ret);
		return ret;
	}

	if (of_property_read_bool(np, "extcon")) {
		charger->extcon = extcon_get_edev_by_phandle(charger->dev, 0);
		if (IS_ERR(charger->extcon))
			return PTR_ERR(charger->extcon);

		charger->extcon_nb.notifier_call = sm5708_extcon_notifier;

		ret = devm_extcon_register_notifier_all(charger->dev, charger->extcon,
							&charger->extcon_nb);

		if (ret < 0)
			return ret;

		sm5708_extcon_notifier(&charger->extcon_nb, 0, NULL);
	}

	return 0;
}

static int sm5708_request_irq(struct sm5708_charger *charger, int irq, irq_handler_t handler)
{
	int ret;
	struct sm5708_chip *chip = (struct sm5708_chip*) dev_get_drvdata(charger->parent);
	int virq = regmap_irq_get_virq(chip->irq_data, irq);
	if (virq <= 0)
		return -EINVAL;

	ret = devm_request_threaded_irq(charger->dev, virq, NULL,
			handler, IRQF_ONESHOT, "sm5708-charger", charger);
	if (ret) {
		dev_err(charger->dev, "failed to request irq %d: %d\n", irq, ret);
		return ret;
	}
	return 0;
}

static const struct power_supply_desc sm5708_charger_desc = {
	.name		= "sm5708-charger",
	.type		= POWER_SUPPLY_TYPE_USB_DCP,
	.get_property	= sm5708_get_property,
	.set_property	= sm5708_set_property,
	.properties	= sm5708_properties,
	.num_properties	= ARRAY_SIZE(sm5708_properties),
};

static int sm5708_charger_probe(struct platform_device *pdev)
{
	struct sm5708_charger *charger;
	struct power_supply_config psy_config = { 0 };
	int ret = 0;

	charger = devm_kzalloc(&pdev->dev, sizeof(struct sm5708_charger), GFP_KERNEL);
	if (IS_ERR_OR_NULL(charger))
		return -ENOMEM;

	charger->dev = &pdev->dev;
	charger->parent = charger->dev->parent;
	charger->regmap = dev_get_regmap(charger->dev->parent, NULL);

	INIT_DELAYED_WORK(&charger->otg_work, sm5708_otg_work);
	INIT_WORK(&charger->otg_fail_work, sm5708_otg_fail_work);

	platform_set_drvdata(pdev, charger);
	psy_config.drv_data = charger;

	ret = sm5708_parse_dt(charger);
	if (ret < 0)
		return ret;

	sm5708_charger_initialize(charger);

	charger->psy = devm_power_supply_register(&pdev->dev,
			&sm5708_charger_desc, &psy_config);
	if (IS_ERR(charger->psy))
		return PTR_ERR(charger->psy);

	sm5708_request_irq(charger, SM5708_VBUSPOK_IRQ, sm5708_vbus_irq);
	sm5708_request_irq(charger, SM5708_VBUSOVP_IRQ, sm5708_vbus_irq);
	sm5708_request_irq(charger, SM5708_VBUSUVLO_IRQ, sm5708_vbus_irq);
	sm5708_request_irq(charger, SM5708_TOPOFF_IRQ, sm5708_done_irq);
	sm5708_request_irq(charger, SM5708_DONE_IRQ, sm5708_done_irq);
	sm5708_request_irq(charger, SM5708_OTGFAIL_IRQ, sm5708_otgfail_irq);

	return 0;
}

static const struct of_device_id sm5708_charger_ids[] = {
	{ .compatible = "siliconmitus,sm5708-charger" },
	{ },
};
MODULE_DEVICE_TABLE(of, sm5708_charger_ids);

static struct platform_driver sm5708_charger_driver = {
	.driver = {
		.name = "sm5708-charger",
		.owner = THIS_MODULE,
		.of_match_table	= sm5708_charger_ids,
	},
	.probe = sm5708_charger_probe,
};

module_platform_driver(sm5708_charger_driver);
MODULE_DESCRIPTION("SM5708 Charger Driver");
MODULE_LICENSE("GPL v2");
