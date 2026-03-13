// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2025, Marc Lainez <marc.lainez@gmail.com>
 *
 * Qualcomm PMI632 QGauge (QG) fuel gauge driver.
 *
 * The QG hardware block provides battery voltage and current measurement
 * via FIFO-based sampling, open circuit voltage (OCV) measurement during
 * sleep states, and coulomb counting through V/I accumulators.
 *
 * This driver exposes battery state via the power_supply subsystem.
 * SOC is computed in-kernel using OCV-to-capacity lookup tables from
 * the monitored-battery simple-battery node.
 */

#include <linux/errno.h>
#include <linux/iio/consumer.h>
#include <linux/interrupt.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/timekeeping.h>

/* Peripheral type register */
#define QG_PERPH_TYPE			0x04
#define QG_PERPH_TYPE_VALUE		0x0D

/* Status registers */
#define QG_STATUS1			0x08
#define  QG_STATUS1_BATT_PRESENT	BIT(0)
#define  QG_STATUS1_QG_OK		BIT(7)

#define QG_STATUS2			0x09
#define  QG_STATUS2_GOOD_OCV		BIT(1)

#define QG_STATUS3			0x0A
#define  QG_STATUS3_FIFO_RT_COUNT	GENMASK(3, 0)

/* Data control */
#define QG_DATA_CTL1			0x41
#define  QG_DATA_CTL1_MASTER_HOLD	BIT(0)

#define QG_DATA_CTL2			0x42
#define  QG_DATA_CTL2_BURST_AVG_HOLD	BIT(0)

/* FIFO / measurement control */
#define QG_S2_NORMAL_MEAS_CTL2		0x51
#define  QG_S2_FIFO_LENGTH_MASK		GENMASK(5, 3)
#define  QG_S2_FIFO_LENGTH_SHIFT	3
#define  QG_S2_NUM_ACCUM_MASK		GENMASK(2, 0)

/* OCV data registers (16-bit little-endian) */
#define QG_S7_PON_OCV_V		0x70
#define QG_S3_GOOD_OCV_V		0x74

/* FIFO data registers */
#define QG_V_FIFO0			0x90
#define QG_I_FIFO0			0xA0

/* Last ADC data registers */
#define QG_LAST_ADC_V			0xC0
#define QG_LAST_ADC_I			0xC2
#define QG_LAST_BURST_AVG_I		0xC6

/* Sentinel value indicating FIFO entry is not yet written */
#define QG_FIFO_RESET_VAL		0x8000

/* Maximum hardware FIFO depth */
#define QG_MAX_FIFO_LENGTH		8

/*
 * Raw ADC to physical unit conversion.
 *   Voltage: raw * 194637 / 1000 = microvolts
 *   Current: raw * 152588 / 1000 = microamps (signed)
 */
#define QG_V_RAW_TO_UV(raw)	div_u64(194637ULL * (u64)(raw), 1000)
#define QG_I_RAW_TO_UA(raw)	div_s64(152588LL * (s64)(raw), 1000)

/*
 * OCV staleness timeout: if the last measured OCV is older than this
 * many seconds, we consider it stale and re-derive SOC only on demand.
 */
#define QG_OCV_STALE_SECS		180

struct pmi632_qg {
	struct device *dev;
	struct regmap *regmap;
	unsigned int base;

	struct power_supply *psy;
	struct power_supply_battery_info *batt_info;

	struct iio_channel *batt_therm;

	struct mutex lock; /* protects register access sequences */

	/* Cached measurements (updated from IRQs and on-demand reads) */
	unsigned int vbat_uv;
	int ibat_ua;
	unsigned int ocv_uv;
	time64_t ocv_time;
	bool batt_present;
};

/* --- Low-level register helpers --- */

static int qg_read16(struct pmi632_qg *qg, unsigned int reg, u16 *val)
{
	int ret;
	__le16 raw;

	ret = regmap_bulk_read(qg->regmap, qg->base + reg, &raw, sizeof(raw));
	if (ret)
		return ret;

	*val = le16_to_cpu(raw);
	return 0;
}

/**
 * qg_master_hold() - Assert/release the FIFO master hold.
 *
 * The master hold freezes the FIFO data registers so they can be read
 * coherently. The hardware requires a 0->1 transition to assert, and
 * writing 0 to release.
 */
static int qg_master_hold(struct pmi632_qg *qg, bool hold)
{
	int ret;

	/* Clear first (required for 0->1 transition to latch) */
	ret = regmap_update_bits(qg->regmap, qg->base + QG_DATA_CTL1,
				 QG_DATA_CTL1_MASTER_HOLD, 0);
	if (ret)
		return ret;

	if (hold)
		ret = regmap_update_bits(qg->regmap, qg->base + QG_DATA_CTL1,
					 QG_DATA_CTL1_MASTER_HOLD,
					 QG_DATA_CTL1_MASTER_HOLD);

	return ret;
}

/* --- Measurement reading functions --- */

static int qg_read_battery_present(struct pmi632_qg *qg, bool *present)
{
	unsigned int val;
	int ret;

	ret = regmap_read(qg->regmap, qg->base + QG_STATUS1, &val);
	if (ret)
		return ret;

	*present = !!(val & QG_STATUS1_BATT_PRESENT);
	return 0;
}

static int qg_read_vbat(struct pmi632_qg *qg, unsigned int *vbat_uv)
{
	u16 raw;
	int ret;

	ret = qg_read16(qg, QG_LAST_ADC_V, &raw);
	if (ret)
		return ret;

	*vbat_uv = QG_V_RAW_TO_UV(raw);
	return 0;
}

static int qg_read_ibat(struct pmi632_qg *qg, int *ibat_ua)
{
	u16 raw;
	int ret;

	/*
	 * Hold the burst average register to get a coherent reading,
	 * then release after reading.
	 */
	ret = regmap_update_bits(qg->regmap, qg->base + QG_DATA_CTL2,
				 QG_DATA_CTL2_BURST_AVG_HOLD,
				 QG_DATA_CTL2_BURST_AVG_HOLD);
	if (ret)
		return ret;

	ret = qg_read16(qg, QG_LAST_BURST_AVG_I, &raw);

	/* Always release the hold, even if the read failed */
	regmap_update_bits(qg->regmap, qg->base + QG_DATA_CTL2,
			   QG_DATA_CTL2_BURST_AVG_HOLD, 0);

	if (ret)
		return ret;

	*ibat_ua = QG_I_RAW_TO_UA(sign_extend32(raw, 15));
	return 0;
}

static int qg_read_ocv(struct pmi632_qg *qg, unsigned int reg,
			unsigned int *ocv_uv)
{
	u16 raw;
	int ret;

	ret = qg_read16(qg, reg, &raw);
	if (ret)
		return ret;

	*ocv_uv = QG_V_RAW_TO_UV(raw);
	return 0;
}

static int qg_read_temperature(struct pmi632_qg *qg, int *temp_decidegc)
{
	int ret, val;

	if (!qg->batt_therm)
		return -ENODEV;

	ret = iio_read_channel_processed(qg->batt_therm, &val);
	if (ret < 0)
		return ret;

	/* SPMI ADC5 batt-therm returns millidegrees C; convert to decidegrees */
	*temp_decidegc = val / 100;

	return 0;
}

/**
 * qg_read_fifo_vbat() - Read voltage samples from the hardware FIFO.
 *
 * Asserts master hold, reads available FIFO entries, averages them,
 * and stores the result. This gives a better voltage estimate than
 * the single last-ADC register.
 */
static int qg_read_fifo_vbat(struct pmi632_qg *qg)
{
	unsigned int count_reg;
	unsigned int fifo_count;
	u64 vbat_sum = 0;
	unsigned int valid = 0;
	u16 raw;
	int ret, i;

	mutex_lock(&qg->lock);

	ret = qg_master_hold(qg, true);
	if (ret)
		goto unlock;

	ret = regmap_read(qg->regmap, qg->base + QG_STATUS3, &count_reg);
	if (ret)
		goto release;

	fifo_count = count_reg & QG_STATUS3_FIFO_RT_COUNT;
	if (fifo_count > QG_MAX_FIFO_LENGTH)
		fifo_count = QG_MAX_FIFO_LENGTH;

	for (i = 0; i < fifo_count; i++) {
		ret = qg_read16(qg, QG_V_FIFO0 + i * 2, &raw);
		if (ret)
			goto release;

		if (raw == QG_FIFO_RESET_VAL)
			continue;

		vbat_sum += QG_V_RAW_TO_UV(raw);
		valid++;
	}

	if (valid)
		qg->vbat_uv = div_u64(vbat_sum, valid);

release:
	qg_master_hold(qg, false);
unlock:
	mutex_unlock(&qg->lock);
	return ret;
}

/**
 * qg_update_ocv() - Read the latest good OCV if available.
 *
 * Checks STATUS2 for the GOOD_OCV bit; if set, reads the S3 good OCV
 * register and updates the cached OCV with a timestamp.
 *
 * The STATUS2 register is "sticky" - reading it clears the GOOD_OCV bit.
 */
static int qg_update_ocv(struct pmi632_qg *qg)
{
	unsigned int status2;
	unsigned int ocv_uv;
	int ret;

	ret = regmap_read(qg->regmap, qg->base + QG_STATUS2, &status2);
	if (ret)
		return ret;

	/* Clear the sticky register by writing back */
	regmap_write(qg->regmap, qg->base + QG_STATUS2, 0);

	if (!(status2 & QG_STATUS2_GOOD_OCV))
		return 0;

	ret = qg_read_ocv(qg, QG_S3_GOOD_OCV_V, &ocv_uv);
	if (ret)
		return ret;

	qg->ocv_uv = ocv_uv;
	qg->ocv_time = ktime_get_seconds();

	return 0;
}

static int qg_get_capacity(struct pmi632_qg *qg, int *capacity)
{
	int temp_decidegc;
	int ret;

	if (!qg->batt_info)
		return -ENODATA;

	/*
	 * If OCV is stale, fall back to using current voltage as a
	 * rough approximation. This is less accurate under load but
	 * better than reporting nothing.
	 */
	if (!qg->ocv_uv)
		return -ENODATA;

	/* Try to get temperature for temperature-compensated lookup */
	ret = qg_read_temperature(qg, &temp_decidegc);
	if (ret)
		temp_decidegc = 250; /* default 25.0 degC */

	*capacity = power_supply_batinfo_ocv2cap(qg->batt_info,
						 qg->ocv_uv,
						 temp_decidegc);

	return 0;
}

/* --- IRQ handlers --- */

static irqreturn_t qg_fifo_done_irq(int irq, void *data)
{
	struct pmi632_qg *qg = data;

	qg_read_fifo_vbat(qg);
	power_supply_changed(qg->psy);

	return IRQ_HANDLED;
}

static irqreturn_t qg_good_ocv_irq(int irq, void *data)
{
	struct pmi632_qg *qg = data;

	mutex_lock(&qg->lock);
	qg_update_ocv(qg);
	mutex_unlock(&qg->lock);

	power_supply_changed(qg->psy);

	return IRQ_HANDLED;
}

static irqreturn_t qg_batt_missing_irq(int irq, void *data)
{
	struct pmi632_qg *qg = data;
	bool present;

	if (!qg_read_battery_present(qg, &present))
		qg->batt_present = present;

	power_supply_changed(qg->psy);

	return IRQ_HANDLED;
}

/* --- Power supply interface --- */

static int qg_get_property(struct power_supply *psy,
			   enum power_supply_property psp,
			   union power_supply_propval *val)
{
	struct pmi632_qg *qg = power_supply_get_drvdata(psy);
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = qg->batt_present;
		return 0;

	case POWER_SUPPLY_PROP_STATUS:
		if (!qg->batt_present) {
			val->intval = POWER_SUPPLY_STATUS_UNKNOWN;
			return 0;
		}
		ret = power_supply_am_i_supplied(psy);
		if (ret < 0 && ret != -ENODEV)
			return ret;
		if (ret > 0)
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
		return 0;

	case POWER_SUPPLY_PROP_HEALTH:
		if (!qg->batt_present) {
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
			return 0;
		}
		if (qg->batt_info) {
			if (qg->vbat_uv < qg->batt_info->voltage_min_design_uv)
				val->intval = POWER_SUPPLY_HEALTH_DEAD;
			else if (qg->vbat_uv > qg->batt_info->voltage_max_design_uv)
				val->intval = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
			else
				val->intval = POWER_SUPPLY_HEALTH_GOOD;
		} else {
			val->intval = POWER_SUPPLY_HEALTH_UNKNOWN;
		}
		return 0;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		if (!qg->batt_present)
			return -ENODATA;
		/* Refresh from last ADC register */
		ret = qg_read_vbat(qg, &qg->vbat_uv);
		if (ret)
			return ret;
		val->intval = qg->vbat_uv;
		return 0;

	case POWER_SUPPLY_PROP_CURRENT_NOW:
		if (!qg->batt_present)
			return -ENODATA;
		ret = qg_read_ibat(qg, &qg->ibat_ua);
		if (ret)
			return ret;
		val->intval = qg->ibat_ua;
		return 0;

	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		if (!qg->batt_present)
			return -ENODATA;
		if (ktime_get_seconds() - qg->ocv_time > QG_OCV_STALE_SECS)
			return -ENODATA;
		val->intval = qg->ocv_uv;
		return 0;

	case POWER_SUPPLY_PROP_CAPACITY:
		if (!qg->batt_present)
			return -ENODATA;
		ret = qg_get_capacity(qg, &val->intval);
		return ret;

	case POWER_SUPPLY_PROP_TEMP:
		if (!qg->batt_present)
			return -ENODATA;
		ret = qg_read_temperature(qg, &val->intval);
		return ret;

	default:
		return -EINVAL;
	}
}

static enum power_supply_property qg_properties[] = {
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP,
};

static const struct power_supply_desc qg_psy_desc = {
	.name = "qg-battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = qg_properties,
	.num_properties = ARRAY_SIZE(qg_properties),
	.get_property = qg_get_property,
};

/* --- Probe and driver registration --- */

static int pmi632_qg_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pmi632_qg *qg;
	struct power_supply_config psy_cfg = {};
	unsigned int perph_type;
	int ret, irq;

	qg = devm_kzalloc(dev, sizeof(*qg), GFP_KERNEL);
	if (!qg)
		return -ENOMEM;

	qg->dev = dev;
	mutex_init(&qg->lock);

	qg->regmap = dev_get_regmap(dev->parent, NULL);
	if (!qg->regmap)
		return dev_err_probe(dev, -ENODEV,
				     "Failed to get parent regmap\n");

	ret = device_property_read_u32(dev, "reg", &qg->base);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read reg property\n");

	/* Verify peripheral type */
	ret = regmap_read(qg->regmap, qg->base + QG_PERPH_TYPE, &perph_type);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read peripheral type\n");

	if (perph_type != QG_PERPH_TYPE_VALUE)
		return dev_err_probe(dev, -ENODEV,
				     "Unexpected peripheral type: 0x%02x\n",
				     perph_type);

	/* Read initial battery presence */
	ret = qg_read_battery_present(qg, &qg->batt_present);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read battery presence\n");

	/* Read PON (power-on) OCV as initial estimate */
	ret = qg_read_ocv(qg, QG_S7_PON_OCV_V, &qg->ocv_uv);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read PON OCV\n");

	qg->ocv_time = ktime_get_seconds();

	/* Also read initial voltage */
	ret = qg_read_vbat(qg, &qg->vbat_uv);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to read battery voltage\n");

	/* Get optional battery thermistor IIO channel */
	qg->batt_therm = devm_iio_channel_get(dev, "batt-therm");
	if (IS_ERR(qg->batt_therm)) {
		ret = PTR_ERR(qg->batt_therm);
		if (ret == -EPROBE_DEFER)
			return ret;
		dev_dbg(dev, "No batt-therm IIO channel: %d\n", ret);
		qg->batt_therm = NULL;
	}

	/* Register power supply */
	psy_cfg.drv_data = qg;
	psy_cfg.of_node = dev->of_node;

	qg->psy = devm_power_supply_register(dev, &qg_psy_desc, &psy_cfg);
	if (IS_ERR(qg->psy))
		return dev_err_probe(dev, PTR_ERR(qg->psy),
				     "Failed to register power supply\n");

	/* Get battery info from monitored-battery */
	ret = power_supply_get_battery_info(qg->psy, &qg->batt_info);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Failed to get battery info\n");

	/* Request IRQs */
	irq = platform_get_irq_byname(pdev, "fifo-done");
	if (irq >= 0) {
		ret = devm_request_threaded_irq(dev, irq, NULL,
						qg_fifo_done_irq,
						IRQF_ONESHOT,
						"pmi632-qg-fifo", qg);
		if (ret)
			return dev_err_probe(dev, ret,
					     "Failed to request fifo-done IRQ\n");
	}

	irq = platform_get_irq_byname(pdev, "good-ocv");
	if (irq >= 0) {
		ret = devm_request_threaded_irq(dev, irq, NULL,
						qg_good_ocv_irq,
						IRQF_ONESHOT,
						"pmi632-qg-ocv", qg);
		if (ret)
			return dev_err_probe(dev, ret,
					     "Failed to request good-ocv IRQ\n");
	}

	irq = platform_get_irq_byname(pdev, "batt-missing");
	if (irq >= 0) {
		ret = devm_request_threaded_irq(dev, irq, NULL,
						qg_batt_missing_irq,
						IRQF_ONESHOT,
						"pmi632-qg-batt", qg);
		if (ret)
			return dev_err_probe(dev, ret,
					     "Failed to request batt-missing IRQ\n");
	}

	platform_set_drvdata(pdev, qg);

	dev_info(dev, "PMI632 QG: batt=%s OCV=%u uV VBAT=%u uV\n",
		 qg->batt_present ? "present" : "absent",
		 qg->ocv_uv, qg->vbat_uv);

	return 0;
}

static int pmi632_qg_suspend(struct device *dev)
{
	struct pmi632_qg *qg = dev_get_drvdata(dev);

	/*
	 * The QG hardware FSM will automatically transition to sleep
	 * states (S3) and measure OCV. Nothing special is needed here
	 * beyond ensuring we pick up the new OCV on resume.
	 */
	dev_dbg(qg->dev, "Suspending, last OCV=%u uV\n", qg->ocv_uv);

	return 0;
}

static int pmi632_qg_resume(struct device *dev)
{
	struct pmi632_qg *qg = dev_get_drvdata(dev);

	/*
	 * After resume, the hardware may have measured a good OCV during
	 * the sleep state. Check for it and update our cached value.
	 * Also refresh the battery voltage.
	 */
	mutex_lock(&qg->lock);
	qg_update_ocv(qg);
	mutex_unlock(&qg->lock);

	qg_read_vbat(qg, &qg->vbat_uv);

	dev_dbg(qg->dev, "Resumed, OCV=%u uV VBAT=%u uV\n",
		qg->ocv_uv, qg->vbat_uv);

	return 0;
}

static DEFINE_SIMPLE_DEV_PM_OPS(pmi632_qg_pm_ops,
				pmi632_qg_suspend, pmi632_qg_resume);

static const struct of_device_id pmi632_qg_of_match[] = {
	{ .compatible = "qcom,pmi632-qg" },
	{}
};
MODULE_DEVICE_TABLE(of, pmi632_qg_of_match);

static struct platform_driver pmi632_qg_driver = {
	.driver = {
		.name = "pmi632-qg",
		.of_match_table = pmi632_qg_of_match,
		.pm = pm_sleep_ptr(&pmi632_qg_pm_ops),
	},
	.probe = pmi632_qg_probe,
};
module_platform_driver(pmi632_qg_driver);

MODULE_DESCRIPTION("Qualcomm PMI632 QGauge fuel gauge driver");
MODULE_AUTHOR("Marc Lainez <marc.lainez@gmail.com>");
MODULE_LICENSE("GPL");
