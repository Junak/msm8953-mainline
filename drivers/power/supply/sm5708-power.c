// SPDX-License-Identifier: GPL-2-Only

#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/extcon.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/led-class-flash.h>
#include <linux/linear_range.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/power_supply.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/thermal.h>
#include <linux/units.h>
#include <linux/time.h>
#include <linux/usb/role.h>
#include <linux/workqueue.h>
#include <linux/version.h>

#include <media/v4l2-flash-led-class.h>

#define mA(mAmp) ((mAmp) * 1000L)
#define mV(mVolt) ((mVolt) * 1000L)

#define PMIC_REG_INT(n)		(0 + (n))
#define PMIC_REG_INTMASK(n)   	(4 + (n))
#define PMIC_REG_STATUS(n)		(8 + (n))

#define PMIC_REG_CNTL			0x0c
#define CNTL_MODE_MASK			GENMASK(2, 0)
#define BOOST_ILIM(uA) \
	((0xfffa5000 >> (clamp(((uA) + 99000) / 100000, 0, 15) << 1)) & 3)

#define BOOST_VSEL(uV) clamp((uV - mV(4000))/mV(100), 0L, 11L)

static const struct linear_range
input_ua_range = LINEAR_RANGE(mA(100), 0, 0x7f, mA(25));

#define PMIC_REG_VBUSCNTL		0x0d
#define VBUSCNTL_INP_ISEL_MASK		GENMASK(6, 0)
#define VBUSCNTL_INP_ISEL_RANGE		(&input_ua_range)
#define VBUSCNTL_INP_ISEL_500MA		((500-100) / 25)
#define VBUSCNTL_INP_ISEL_300MA		((300-100) / 25)
#define INPUT_ISEL_MA(val)	((clamp(val, 100, 3275) - 100) / 25)

static const struct linear_range
charge_ua_range = LINEAR_RANGE(mA(100), 0, 0x3f, mA(50)); // mA(3250));

#define PMIC_REG_CHGCNTL1     	0x0f
#define PMIC_REG_CHGCNTL2		0x10
#define CHGCNTL2_CHG_ISEL_MASK		GENMASK(5, 0)
#define CHGCNTL2_CHG_ISEL_RANGE		(&charge_ua_range)
#define CHARGE_ISEL_MA(val)	((clamp(val, 100, 3250) - 100) / 50)

static const struct linear_range
float_uv_range = LINEAR_RANGE(mV(3990), 0, 0x3f, mV(10)); // mV(4620));

#define PMIC_REG_CHGCNTL3		0x12
#define CHGCNTL3_FLOAT_VSEL_MASK	GENMASK(5, 0)
#define CHGCNTL3_FLOAT_VSEL_RANGE	(&float_uv_range)
#define FLOAT_VSEL_MV(val)	((clamp(val, 3990, 4620) - 3990) / 10)
#define CHGCNTL3_AUTOSTOP_EN		BIT(6)

static const struct linear_range
topoff_ua_range = LINEAR_RANGE(mA(100), 0, 0xf, mA(25)); //mA(475));

#define PMIC_REG_CHGCNTL4		0x13
#define CHGCNTL4_TOPOFF_ISEL_MASK	GENMASK(3, 0)
#define CHGCNTL4_TOPOFF_ISEL_RANGE	(&topoff_ua_range)
#define CHGCNTL4_FREQ_MASK		GENMASK(5, 4)
#define CHGCNTL4_FREQ(v)		((v) << 4)
#define TOPOFF_ISEL_MA(val)	((clamp(val, 100, 475) - 100) / 25)

#define PMIC_REG_CHGCNTL5		0x14
#define CHGCNTL5_IQ3LIMIT_MASK		GENMASK(1, 0)
#define CHGCNTL5_IQ3LIMIT(v)		((v) << 0)
#define CHGCNTL5_OTG_ISEL_MASK		GENMASK(3, 2)
#define CHGCNTL5_OTG_ISEL_VAL_0P5A	0
#define CHGCNTL5_OTG_ISEL_VAL_0P7A	1
#define CHGCNTL5_OTG_ISEL_VAL_0P9A	2
#define CHGCNTL5_OTG_ISEL_VAL_1P5A	3

#define PMIC_REG_CHGCNTL6		0x15
#define CHGCNTL6_AUTOSET_EN		BIT(1)
#define CHGCNTL6_AICL_EN		BIT(5)
#define CHGCNTL6_AICL_TH_VSEL_MASK	GENMASK(7, 6)
#define CHGCNTL6_AICL_TH_VSEL(v)	((v) << 6)

#define PMIC_REG_CHGCNTL7		0x16
#define CHGCNTL7_TOPOFFTMR_MASK		GENMASK(4, 3)
#define CHGCNTL7_TOPOFFTMR(v)		((v) << 3)

#define PMIC_REG_FLEDCNTL5		0x1f
#define PMIC_REG_FLEDCNTL6		0x20
#define FLEDCNTL6_BOOST_VSEL_MASK	GENMASK(3, 0)
#define FLEDCNTL6_BOOST_VSEL_ENCODE(uV) clamp((uV - mV(4000))/mV(100), 0L, 11L)

#define PMIC_LED_BASE(n)		(0x17 + (n) * 4)

#define PMIC_REG_DEVICEID		0x37

#define MONITOR_PERIOD_MS		(10 * MILLI)
#define SM5708_NUM_LEDS			2

#define PMIC_DBG(fmt, ...) dev_dbg(pmic->dev, "%s:" fmt, __func__, ##__VA_ARGS__)
#define LED_DBG(fmt, ...) dev_dbg(led->flash.led_cdev.dev, "%s:" fmt, __func__, ##__VA_ARGS__)

#define PSY_BATTERY_NAME "sm5708-battery"

enum sm5708_sts0_bits {
	/* status and interrupt (mask) flags */
	S0_VBUSPOK,	S0_VBUSUVLO,	S0_VBUSOVP,	S0_VBUSLIMIT,
};

enum sm5708_sts1_bits {
	S1_AICL,	S1_BATOVP,	S1_NOBAT,	S1_CHGON,
	S1_Q4FULLON,	S1_TOPOFF,	S1_DONE,	S1_WDTMROFF,
};

enum sm5708_sts2_bits {
	S2_THEMREG,	S2_THEMSHDN,	S2_OTGFAIL,	S2_DISLIMIT,
	S2_PRETMROFF,	S2_FASTTMROFF,	S2_LOWBATT,	S2_NENQ4,
};

enum sm5708_sts3_bits {
	S3_FLED1SHORT,	S3_FLED1OPEN,	S3_FLED2SHORT,	S3_FLED2OPEN,
	S3_BOOSTPOK_NG,	S3_BOOSTPOK,	S3_ABSTMR1OFF,	S3_SBPS,
};

enum sm5708_op_mode {
	MODE_SUSPEND = 0,
	/* MODE_FACTORY = 1, */
	MODE_CHARGE_INHIBIT = 4,
	MODE_CHARGE,
	MODE_FLASH_BOOST,
	MODE_USB_OTG,
};

enum sm5708_mode_req {
	MODE_REQ_CHARGE		= BIT(0),
	MODE_REQ_INPUT		= BIT(1),
	MODE_REQ_FLASH		= BIT(2),
	MODE_REQ_TORCH		= BIT(3),
	MODE_REQ_BOOST_MASK	= MODE_REQ_TORCH | MODE_REQ_FLASH,
	MODE_REQ_OTG		= BIT(4),
};

enum sm5708_freq_sel {
	FREQ_3000K, FREQ_2400K, FREQ_1500K, FREQ_1800K,
};

enum sm5708_iq3limit_sel {
	IQ3_2000mA, IQ3_2800mA, IQ3_3500mA, IQ3_4000mA,
};

enum sm5708_aicl_th_volt_sel {
	AICL_TH_45OOMV, AICL_TH_46OOMV, AICL_TH_47OOMV, AICL_TH_48OOMV,
};

enum sm5708_topoff_timer_sel {
	TOPOFFTMR_10M, TOPOFFTMR_20M, TOPOFFTMR_30M, TOPOFFTMR_45M,
};

struct sm5708_bat;
struct sm5708_pmic;
struct sm5708_drvdata {
	struct sm5708_bat *bat;
	struct sm5708_pmic *pmic;
};

struct sm5708_fled {
	struct led_classdev_flash flash;
	struct delayed_work timeout_work;
	struct v4l2_flash *v4l2_flash;
	struct sm5708_pmic *pmic;
	struct regmap *rmap;
	s64 budget;
	u64 last_mark;
	u32 torch_current, flash_current;
	u32 flash_current_hw;
	u32 flash_timeout, last_current;
	u32 max_avg_current;
	u8 index, base, mode;
};

struct sm5708_pmic {
	struct sm5708_drvdata drvdata;
	struct delayed_work aicl_work, mon_work, cable_work;
	struct power_supply_battery_info *info;
	struct usb_role_switch *role_sw;
	struct gpio_desc *chg_en_gpio;
	enum sm5708_op_mode cur_mode;
	struct notifier_block usb_nb;
	struct workqueue_struct *wq;
	struct extcon_dev *extcon;
	struct power_supply *psy;
	struct regmap *rmap;
	struct device *dev;
	struct mutex lock;
	u32 max_input_current;
	int last_health;
	u32 mode_req;

	bool online;
	bool host;
	bool health_ok;
	bool suspended;

	struct sm5708_fled leds[SM5708_NUM_LEDS];
};

#define to_sm5708_pmic(ptr, member) \
	container_of(ptr, struct sm5708_pmic, member)

#define SM5708_READ_LINEAR(pmic, reg, field, _out) ({			\
	u32 _regval;							\
	int ret = regmap_read(pmic->rmap, PMIC_REG_##reg, &_regval);	\
	if (!ret) {							\
		ret = linear_range_get_value(reg##_##field##_RANGE,	\
			FIELD_GET(reg##_##field##_MASK, _regval), &_regval);	\
		*_out = _regval;					\
	} \
	ret; })


#define SM5708_WRITE_LINEAR(pmic, reg, field, val) ({			\
	u32 sel;							\
	bool found;							\
	int ret;							\
	ret = linear_range_get_selector_low(reg##_##field##_RANGE,	\
			val, &sel, &found);				\
	if (!found)							\
		ret = -EINVAL;						\
	else if (!ret)							\
		ret = regmap_update_bits(pmic->rmap, PMIC_REG_##reg,	\
			reg##_##field##_MASK,				\
			FIELD_PREP(reg##_##field##_MASK, sel));		\
	ret; })


static enum power_supply_property sm5708_pmic_properties[] = {
	POWER_SUPPLY_PROP_CHARGE_BEHAVIOUR,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
};

static inline bool sm5708_can_charge(struct sm5708_pmic* pmic)
{
	return pmic->health_ok && !pmic->suspended;
}

static int sm5708_pmic_update_mode_unlocked(struct sm5708_pmic* pmic,
		enum sm5708_mode_req mask, enum sm5708_mode_req state)
{
	unsigned boost_ilim = BOOST_ILIM(mA(500));
	unsigned boost_vsel = BOOST_VSEL(mV(4500));;
	struct regmap *rmap = pmic->rmap;
	enum sm5708_op_mode mode = MODE_SUSPEND;
	unsigned req_new;
	int ret;

	lockdep_assert_held(&pmic->lock);

	req_new = (pmic->mode_req & ~mask) | (state & mask);

	PMIC_DBG("reqested mode: %x -> %x (health ok %d, suspend %d)\n",
			pmic->mode_req, req_new,
			pmic->health_ok, pmic->suspended);

	if (pmic->online) {
		if (sm5708_can_charge(pmic) && (req_new & MODE_REQ_CHARGE))
			mode = MODE_CHARGE;
		else if (req_new & MODE_REQ_INPUT)
			mode = MODE_CHARGE_INHIBIT;
	}

	if (mode == MODE_SUSPEND) {
		if (req_new & MODE_REQ_OTG) {
			if (req_new & MODE_REQ_FLASH)
				boost_ilim = BOOST_ILIM(mA(900));
			boost_vsel = BOOST_VSEL(mV(5000));
			mode = MODE_USB_OTG;
		} else if (req_new & MODE_REQ_BOOST_MASK) {
			mode = MODE_FLASH_BOOST;
			if (req_new & MODE_REQ_FLASH)
				boost_ilim = BOOST_ILIM(mA(900));
		}
	}

	gpiod_set_value(pmic->chg_en_gpio, mode == MODE_CHARGE);

	if (mode == MODE_FLASH_BOOST || mode == MODE_USB_OTG) {
		ret = regmap_update_bits(rmap, PMIC_REG_CHGCNTL5,
					 CHGCNTL5_OTG_ISEL_MASK,
					 FIELD_PREP(CHGCNTL5_OTG_ISEL_MASK, boost_ilim));
		ret = ret ?: regmap_update_bits(rmap, PMIC_REG_FLEDCNTL6,
					 FLEDCNTL6_BOOST_VSEL_MASK,
					 FIELD_PREP(FLEDCNTL6_BOOST_VSEL_MASK, boost_vsel));
		if (ret)
			goto fail;
	}

	if (mode == pmic->cur_mode)
		goto out_no_changes;

	ret = regmap_update_bits(rmap, PMIC_REG_CHGCNTL4, CHGCNTL4_FREQ_MASK,
			FIELD_PREP(CHGCNTL4_FREQ_MASK, (mode == MODE_CHARGE) ? FREQ_3000K : FREQ_1500K));

	if (mode == MODE_CHARGE && pmic->cur_mode == MODE_USB_OTG) {
		if (!regmap_update_bits(rmap, PMIC_REG_CNTL, CNTL_MODE_MASK,
					FIELD_PREP(CNTL_MODE_MASK, MODE_SUSPEND)))
			pmic->cur_mode = MODE_SUSPEND;

		msleep(100);
	}

	ret = ret ?: regmap_update_bits(rmap, PMIC_REG_CNTL, CNTL_MODE_MASK,
				FIELD_PREP(CNTL_MODE_MASK, mode));
	if (ret)
		goto fail;

	PMIC_DBG("mode: %x -> %x\n", pmic->cur_mode, mode);

	pmic->cur_mode = mode;
out_no_changes:
	pmic->mode_req = req_new;
	return 0;

fail:
	dev_err(pmic->dev, "failed to set mode %d\n", mode);
	gpiod_set_value(pmic->chg_en_gpio, 0);
	if (!regmap_update_bits(rmap, PMIC_REG_CNTL, CNTL_MODE_MASK,
				FIELD_PREP(CNTL_MODE_MASK, MODE_SUSPEND)))
		pmic->cur_mode = MODE_SUSPEND;

	return ret;
}

static int sm5708_pmic_update_mode(struct sm5708_pmic* pmic,
			      enum sm5708_mode_req mask, enum sm5708_mode_req val)
{
	int ret;
	mutex_lock(&pmic->lock);
	ret = sm5708_pmic_update_mode_unlocked(pmic, mask, val);
	mutex_unlock(&pmic->lock);
	return ret;
}

static int sm5708_pmic_get_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       union power_supply_propval *out)
{
	struct sm5708_pmic *pmic = power_supply_get_drvdata(psy);
	u8 sts[2];
	int ret, *val = &out->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_CHARGE_BEHAVIOUR:
		if (pmic->mode_req & MODE_REQ_CHARGE)
			*val = POWER_SUPPLY_CHARGE_BEHAVIOUR_AUTO;
		else if (pmic->mode_req & (MODE_REQ_INPUT))
			*val = POWER_SUPPLY_CHARGE_BEHAVIOUR_INHIBIT_CHARGE;
		else
			*val = POWER_SUPPLY_CHARGE_BEHAVIOUR_FORCE_DISCHARGE;
		return 0;
	case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		return SM5708_READ_LINEAR(pmic, CHGCNTL4, TOPOFF_ISEL, val);
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		return SM5708_READ_LINEAR(pmic, CHGCNTL2, CHG_ISEL, val);
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		return SM5708_READ_LINEAR(pmic, CHGCNTL3, FLOAT_VSEL, val);
	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		return SM5708_READ_LINEAR(pmic, VBUSCNTL, INP_ISEL, val);
	case POWER_SUPPLY_PROP_ONLINE:
		*val = pmic->online;
		return 0;
	case POWER_SUPPLY_PROP_STATUS:
		ret = regmap_bulk_read(pmic->rmap, PMIC_REG_STATUS(0), sts, sizeof(sts));
		if (ret)
			return ret;

		if (sts[1] & BIT(S1_DONE))
			*val = POWER_SUPPLY_STATUS_FULL;
		if ((sts[1] & BIT(S1_CHGON)) && (sts[0] & BIT(S0_VBUSPOK)))
			*val = POWER_SUPPLY_STATUS_CHARGING;
		else
			*val = POWER_SUPPLY_STATUS_DISCHARGING;
		return 0;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = regmap_bulk_read(pmic->rmap, PMIC_REG_STATUS(0), sts, sizeof(sts[0]));
		if (ret)
			return ret;

		if (sts[0] & BIT(S0_VBUSOVP))
			*val = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
		else if (sts[0] & BIT(S0_VBUSPOK))
			*val = POWER_SUPPLY_HEALTH_GOOD;
		else
			*val = POWER_SUPPLY_HEALTH_UNKNOWN;
		return 0;
	default:
		return -EINVAL;
	};

	return 0;
}

static bool init_charge_on = 1, init_discharge = 0;
static bool enable_debug = false;
static int sm5708_pmic_set_property(struct power_supply *psy,
			       enum power_supply_property psp,
			       const union power_supply_propval *propval)
{
	struct sm5708_pmic *pmic= power_supply_get_drvdata(psy);
	u32 mask = MODE_REQ_INPUT | MODE_REQ_CHARGE, val = 0;

	if (psp != POWER_SUPPLY_PROP_CHARGE_BEHAVIOUR)
		return -EINVAL;

	switch (propval->intval) {
	case POWER_SUPPLY_CHARGE_BEHAVIOUR_AUTO:
		val = MODE_REQ_CHARGE | MODE_REQ_INPUT;
		break;
	case POWER_SUPPLY_CHARGE_BEHAVIOUR_INHIBIT_CHARGE:
		val = MODE_REQ_INPUT;
		break;
	case POWER_SUPPLY_CHARGE_BEHAVIOUR_FORCE_DISCHARGE:
		break;
	default:
		return -EINVAL;
	}

	return sm5708_pmic_update_mode(pmic, mask, val);
}

static irqreturn_t sm5708_pmic_irq_handler(int irq, void *data)
{
	struct sm5708_pmic *pmic = data;
	u8 regs[4];
	int ret;

	ret = regmap_bulk_read(pmic->rmap, PMIC_REG_INT(0), &regs, sizeof(regs));
	if (ret < 0)
		return IRQ_NONE;

	if (regs[0])
		regmap_set_bits(pmic->rmap, PMIC_REG_INTMASK(0), regs[0]);
	if (regs[1])
		regmap_set_bits(pmic->rmap, PMIC_REG_INTMASK(1), regs[1]);

	if (enable_debug)
		dev_dbg_ratelimited(pmic->dev, "IRQ: %x %x\n", regs[0], regs[1]);

	if (regs[0] & (BIT(S0_VBUSPOK)|BIT(S0_VBUSUVLO)))
		queue_delayed_work(pmic->wq, &pmic->cable_work, msecs_to_jiffies(10));
	if (regs[0] & (BIT(S0_VBUSOVP)) || regs[1] & (BIT(S1_DONE)))
		power_supply_changed(pmic->psy);

	return IRQ_HANDLED;
}

static void sm5708_pmic_cable_work(struct work_struct *work)
{
	struct sm5708_pmic *pmic = to_sm5708_pmic(work, cable_work.work);
	int usb, sdp, dcp, ret, status0, host, err;
	bool online = false, changed = false;
	u32 input_current;

	usb = extcon_get_state(pmic->extcon, EXTCON_USB);
	sdp = extcon_get_state(pmic->extcon, EXTCON_CHG_USB_SDP);
	dcp = extcon_get_state(pmic->extcon, EXTCON_CHG_USB_DCP);
	host = extcon_get_state(pmic->extcon, EXTCON_USB_HOST);

	PMIC_DBG("cable usb %d sdp %d dcp %d host %d", usb, sdp, dcp, host);

	err = min(min(dcp, sdp), min(usb, host));
	if (err < 0) {
		dev_err(pmic->dev, "failed to get extcon state: %d\n", err);
		return;
	}

	if (usb || dcp || host || dcp) {
		regmap_read_poll_timeout(pmic->rmap, PMIC_REG_STATUS(0),
				status0, status0 & BIT(S0_VBUSPOK), 1000, 50000);
		online = !!(status0 & BIT(S0_VBUSPOK));
	}

	host = pmic->role_sw ? host : 0;

	mutex_lock(&pmic->lock);
	if (pmic->online != online || pmic->host != host) {
		pmic->online = online;
		pmic->host = host;

		ret = sm5708_pmic_update_mode_unlocked(pmic, MODE_REQ_OTG,
				host ? MODE_REQ_OTG : 0);
		if (ret) {
			dev_err(pmic->dev, "failed to update mode: %d\n", ret);
		} else {
			usb_role_switch_set_role(pmic->role_sw,
					host ? USB_ROLE_HOST :
					(sdp ? USB_ROLE_DEVICE : USB_ROLE_NONE));
		}

		changed = true;

		if (!pmic->online)
			pmic->health_ok = false;

		if (pmic->online)
			queue_delayed_work(pmic->wq, &pmic->mon_work, msecs_to_jiffies(0));
		else
			cancel_delayed_work(&pmic->mon_work);
	}

	PMIC_DBG("online %d changed %d", online, changed);

	input_current = pmic->max_input_current;
	if (sdp)
		input_current = min(input_current, mA(900));

	SM5708_WRITE_LINEAR(pmic, VBUSCNTL, INP_ISEL, input_current);

	mutex_unlock(&pmic->lock);

	if (changed)
		power_supply_changed(pmic->psy);
}

static void sm5708_pmic_check_health(struct sm5708_pmic *pmic)
{
	struct power_supply_battery_info *info = pmic->info;
	struct power_supply *bat;
	union power_supply_propval health;
	u32 volt, curr, term;
	bool charge_allow = true;
	int ret;

	bat = power_supply_get_by_name(PSY_BATTERY_NAME);
	if (IS_ERR_OR_NULL(bat) || !info) {
		dev_err(pmic->dev, "failed to get battery PSY: %ld\n", PTR_ERR(bat));
		goto fail_disable;
	}

	ret = power_supply_get_property(bat, POWER_SUPPLY_PROP_HEALTH, &health);
	power_supply_put(bat);
	if (ret)
		goto fail_disable;

	if (pmic->health_ok && pmic->last_health == health.intval)
		return;

	curr = info->constant_charge_current_max_ua;
	volt = info->constant_charge_voltage_max_uv;
	term = info->charge_term_current_ua;

	switch (health.intval) {
	case POWER_SUPPLY_HEALTH_GOOD:
		break;
	case POWER_SUPPLY_HEALTH_HOT:
	case POWER_SUPPLY_HEALTH_COLD:
		charge_allow = false;
		fallthrough;
	case POWER_SUPPLY_HEALTH_WARM:
	case POWER_SUPPLY_HEALTH_COOL:
		volt -= mV(200);
		curr /= 3;
		term /= 2;
		break;
	default:
		goto fail_disable;
	}

	volt = max(volt, float_uv_range.min);
	term = max(term, topoff_ua_range.min);
	curr = max(curr, charge_ua_range.min);

	ret = SM5708_WRITE_LINEAR(pmic, CHGCNTL2, CHG_ISEL, curr);
	ret = ret ?: SM5708_WRITE_LINEAR(pmic, CHGCNTL3, FLOAT_VSEL, volt);
	ret = ret ?: SM5708_WRITE_LINEAR(pmic, CHGCNTL4, TOPOFF_ISEL, term);
	if (ret) {
		dev_err(pmic->dev, "failed to set charge current/voltage: %d\n", ret);
		goto fail_disable;
	}

	mutex_lock(&pmic->lock);
	pmic->last_health = health.intval;
	pmic->health_ok = charge_allow;
	sm5708_pmic_update_mode_unlocked(pmic, 0, 0);
	mutex_unlock(&pmic->lock);
	power_supply_changed(pmic->psy);

	return;

fail_disable:
	/* Block charging */
	mutex_lock(&pmic->lock);
	pmic->last_health = POWER_SUPPLY_HEALTH_UNKNOWN;
	pmic->health_ok = false;
	sm5708_pmic_update_mode_unlocked(pmic, 0, 0);
	mutex_unlock(&pmic->lock);
	power_supply_changed(pmic->psy);
}

static void sm5708_pmic_monitor_work(struct work_struct *work)
{
	struct sm5708_pmic *pmic = to_sm5708_pmic(work, mon_work.work);

	PMIC_DBG("Running monitor work\n");

	sm5708_pmic_check_health(pmic);

	if (pmic->online && (pmic->mode_req & MODE_REQ_CHARGE))
		queue_delayed_work(pmic->wq, &pmic->mon_work,
				   msecs_to_jiffies(MONITOR_PERIOD_MS));
}

static int sm5708_pmic_property_is_writeable(struct power_supply *psy, enum power_supply_property psp)
{
	return psp == POWER_SUPPLY_PROP_CHARGE_BEHAVIOUR;
}

static const struct power_supply_desc sm5708_charger_desc = {
	.name		= "sm5708-charger",
	.type		= POWER_SUPPLY_TYPE_USB,
	.properties	= sm5708_pmic_properties,
	.num_properties	= ARRAY_SIZE(sm5708_pmic_properties),
	.get_property	= sm5708_pmic_get_property,
	.set_property	= sm5708_pmic_set_property,
	.charge_behaviours = BIT(POWER_SUPPLY_CHARGE_BEHAVIOUR_AUTO)
			   | BIT(POWER_SUPPLY_CHARGE_BEHAVIOUR_INHIBIT_CHARGE)
			   | BIT(POWER_SUPPLY_CHARGE_BEHAVIOUR_FORCE_DISCHARGE),
	.property_is_writeable = sm5708_pmic_property_is_writeable,
};

static const struct {
	u8 reg, mask, val;
} sm5708_pmic_reg_init[] = {
	{ PMIC_REG_CNTL, CNTL_MODE_MASK, MODE_SUSPEND << 0 },
	{ PMIC_REG_CHGCNTL4, CHGCNTL4_FREQ_MASK, CHGCNTL4_FREQ(FREQ_1500K) },
	{ PMIC_REG_CHGCNTL5, CHGCNTL5_IQ3LIMIT_MASK,
			       CHGCNTL5_IQ3LIMIT(IQ3_3500mA) },
	{ PMIC_REG_CHGCNTL7, CHGCNTL7_TOPOFFTMR_MASK,
			       CHGCNTL7_TOPOFFTMR(TOPOFFTMR_45M) },
	{ PMIC_REG_CHGCNTL6,
		CHGCNTL6_AICL_EN | CHGCNTL6_AUTOSET_EN | CHGCNTL6_AICL_TH_VSEL_MASK,
		CHGCNTL6_AICL_EN | CHGCNTL6_AICL_TH_VSEL(AICL_TH_45OOMV), },
	{ PMIC_REG_CHGCNTL3, CHGCNTL3_AUTOSTOP_EN, CHGCNTL3_AUTOSTOP_EN },
	{ PMIC_REG_VBUSCNTL, VBUSCNTL_INP_ISEL_MASK, 0 },
	{ PMIC_REG_INTMASK(0), ~0,
		~(u8)(BIT(S0_VBUSPOK) | BIT(S0_VBUSUVLO) | BIT(S0_VBUSOVP)) },
	{ PMIC_REG_INTMASK(1), ~0, ~(u8) BIT(S1_DONE) },
	{ PMIC_REG_INTMASK(2), ~0, ~0 },
	{ PMIC_REG_INTMASK(3), ~0, ~0 },
};

static int sm5708_pmic_extcon_notifier(struct notifier_block *nb,
				  unsigned long event, void *ptr)
{
	struct sm5708_pmic *pmic = to_sm5708_pmic(nb, usb_nb);
	mod_delayed_work(pmic->wq, &pmic->cable_work, msecs_to_jiffies(20));
	return NOTIFY_DONE;
}

static const struct regmap_config sm5708_pmic_regmap_config = {
	.cache_type = REGCACHE_FLAT,
	.num_reg_defaults_raw = 0x40,
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_table = &(const struct regmap_access_table) {
		.yes_ranges = (const struct regmap_range[]) {
			regmap_reg_range(0x00, 0x03), /* INT0-3 */
			regmap_reg_range(0x08, 0x0d), /* STATUS0-3 */
			regmap_reg_range(0x38, 0x3f),
		},
		.n_yes_ranges = 3,
	},
};

static void sm5708_pmic_cleanup(void *ptr)
{
	struct sm5708_pmic *pmic = ptr;
	struct sm5708_fled *led;

	for (led = pmic->leds; led < &pmic->leds[SM5708_NUM_LEDS]; led ++) {
		if (!led->pmic)
			continue;

		disable_delayed_work_sync(&led->timeout_work);

		if (!IS_ERR_OR_NULL(led->v4l2_flash))
			v4l2_flash_release(led->v4l2_flash);
	}

	disable_delayed_work_sync(&pmic->mon_work);
	disable_delayed_work_sync(&pmic->cable_work);
	destroy_workqueue(pmic->wq);
	if (!IS_ERR_OR_NULL(pmic->role_sw))
		usb_role_switch_put(pmic->role_sw);
}

static void sm5708_pmic_put_info(void *ptr)
{
	struct sm5708_pmic *pmic = ptr;

	power_supply_put_battery_info(pmic->psy, pmic->info);
}

/* Offsets from PMIC_LED_BASE */
#define LED_REG_CFG			0x0
#define REG_CFG_MODE_MASK		GENMASK(1, 0)
#define REG_CFG_MODE_VAL_OFF		0x0
#define REG_CFG_MODE_VAL_TORCH		0x1
#define REG_CFG_MODE_VAL_FLASH		0x2
#define REG_CFG_MODE_VAL_GPIO_CTRL	0x3
#define LED_REG_TIMEOUT		0x1
#define REG_TIMEOUT_MASK		GENMASK(3, 0)
#define REG_TIMEOUT_EN			BIT(4)
#define LED_REG_FLASH_ISEL		0x2
#define LED_REG_TORCH_ISEL		0x3

static const struct linear_range
torch_isel_range = LINEAR_RANGE(10000, 0, GENMASK(4, 0), 10000);

static const struct linear_range
flash_timeout_range = LINEAR_RANGE(100000, 0, GENMASK(3, 0), 100000);

#define FLASH_N_RANGES	    (2)
static const struct linear_range
flash_isel_ranges[SM5708_NUM_LEDS][FLASH_N_RANGES] = {
	{ LINEAR_RANGE(300000, 0, 0xe, 25000),
	  LINEAR_RANGE(700000, 0xf, 0x1f, 50000) },
	{ LINEAR_RANGE(25000, 0, 0xe, 25000),
	  LINEAR_RANGE(400000, 0xf, 0x1f, 25000) },
};

static inline struct sm5708_fled *
to_fled(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct sm5708_fled, flash.led_cdev);
}

static u32 sm5708_get_selector(const struct linear_range *r, int n, u32 val)
{
	bool found;
	u32 sel;
	if (linear_range_get_selector_low_array(r, n, val, &sel, &found))
		return r->min_sel;
	return sel;
}

static void sm5708_flash_mark_current_change(struct sm5708_fled *led, u32 curr, u64 time_ns)
{
	u64 duration = min((time_ns - led->last_mark) / 1000, 30 * MICRO);
	led->budget += duration * led->max_avg_current;
	led->budget -= duration * led->last_current;
	led->budget = clamp_t(s64, led->budget,
			(-10 * MICRO) * (s64) led->max_avg_current,
			(2 * MICRO) * (s64) led->max_avg_current);

	LED_DBG("budget: %lld curr %u -> %u\n",
			led->budget, led->last_current, curr);

	led->last_mark = time_ns;
	led->last_current = curr;
}

static int sm5708_flash_set_mode(struct sm5708_fled *led, enum sm5708_mode_req mode, bool on)
{
	u32 val = 0, new_mode = (led->mode & ~mode) | (on ? mode : 0);
	u32 curr = 0;
	u64 time;
	int ret;

	if (led->mode == new_mode)
		return 0;

	if (on)
		sm5708_pmic_update_mode(led->pmic, MODE_REQ_BOOST_MASK, new_mode);

	if (new_mode & MODE_REQ_FLASH) {
		curr = led->flash_current_hw;
		val = REG_CFG_MODE_VAL_FLASH;
	} else if (new_mode & MODE_REQ_TORCH) {
		curr = led->torch_current;
		val = REG_CFG_MODE_VAL_TORCH;
	}

	if (new_mode)
		time = ktime_get_boottime_ns();

	ret = regmap_update_bits(led->rmap, led->base + LED_REG_CFG, REG_CFG_MODE_MASK, val);
	if (!ret) {
		if (!new_mode)
			time = ktime_get_boottime_ns();
		led->mode = new_mode;
		sm5708_flash_mark_current_change(led, curr, time);
	} else {
		regmap_update_bits(led->rmap, led->base + LED_REG_CFG, REG_CFG_MODE_MASK, 0);
		new_mode = 0;
	}

	if (!on)
		sm5708_pmic_update_mode(led->pmic, MODE_REQ_BOOST_MASK, new_mode);

	return ret;
}

static int sm5708_torch_set(struct led_classdev *led_cdev, enum led_brightness val)
{
	struct sm5708_fled *led = to_fled(led_cdev);
	int ret;

	if (val) {
		u32 sel = min_t(u32, led_cdev->max_brightness, val) - 1;
		ret = regmap_write(led->rmap, led->base + LED_REG_TORCH_ISEL, sel);
		if (ret)
			return ret;

		linear_range_get_value(&torch_isel_range, sel, &led->torch_current);
		if (led->mode == MODE_REQ_TORCH)
			sm5708_flash_mark_current_change(led, led->torch_current, ktime_get_boottime_ns());
	}

	return sm5708_flash_set_mode(led, MODE_REQ_TORCH, !!val);
}

static int sm5708_flash_current_write(struct sm5708_fled *led, u32 val)
{
	const struct linear_range *r = flash_isel_ranges[led->index];
	u32 sel = sm5708_get_selector(r, FLASH_N_RANGES, val);

	linear_range_get_value_array(flash_isel_ranges[led->index], FLASH_N_RANGES,
			sel, &led->flash_current_hw);
	return regmap_write(led->rmap, led->base + LED_REG_FLASH_ISEL, sel);
}

static int sm5708_flash_brightness_set(struct led_classdev_flash *flash, u32 val)
{
	struct sm5708_fled *led = to_fled(&flash->led_cdev);
	int ret = sm5708_flash_current_write(led, val);
	if (!ret)
		led->flash_current = val;
	return ret;
}

static int sm5708_flash_strobe_get(struct led_classdev_flash *flash, bool *state)
{
	*state = !!(to_fled(&flash->led_cdev)->mode & MODE_REQ_FLASH);
	return 0;
}

static int sm5708_flash_timeout_write(struct sm5708_fled *led, u32 timeout)
{
	u32 sel = sm5708_get_selector(&flash_timeout_range, 1, timeout);
	return regmap_write(led->rmap, led->base + LED_REG_TIMEOUT,
			REG_TIMEOUT_EN | FIELD_PREP(REG_TIMEOUT_MASK, sel));
}

static int sm5708_flash_timeout_set(struct led_classdev_flash *flash, u32 timeout)
{
	struct sm5708_fled *led = to_fled(&flash->led_cdev);
	int ret = sm5708_flash_timeout_write(led, timeout);
	if (!ret)
		led->flash_timeout = timeout;
	return ret;
}

static void sm5708_flash_timeout_work(struct work_struct *work)
{
	struct sm5708_fled *led = container_of(work, struct sm5708_fled, timeout_work.work);

	mutex_lock(&led->flash.led_cdev.led_access);
	sm5708_flash_set_mode(led, MODE_REQ_FLASH, false);
	mutex_unlock(&led->flash.led_cdev.led_access);
}

static int sm5708_flash_strobe_set(struct led_classdev_flash *flash, bool state)
{
	struct sm5708_fled *led = to_fled(&flash->led_cdev);

	if (state) {
		u64 timeout = led->flash_timeout, curr = led->flash_current;

		sm5708_flash_mark_current_change(led, led->last_current, ktime_get_boottime_ns());

		if (led->budget < 0) {
		    curr = flash_isel_ranges[led->index][0].min;
		} else if (curr * timeout > led->budget &&
		    curr > flash_isel_ranges[led->index][0].min) {
			curr = led->budget;
			do_div(curr, timeout);
			curr = max_t(u64, flash_isel_ranges[led->index][0].min, curr);
		}

		if (led->budget < 0) {
			timeout = flash_timeout_range.min;
		} else if (curr * timeout > led->budget &&
		    timeout > flash_timeout_range.min) {
			timeout = led->budget;
			do_div(timeout, curr);
			timeout = max_t(u64, flash_timeout_range.min, timeout);
		}

		if (curr * timeout > led->budget) {
			LED_DBG("fail, budget: %lld\n", led->budget);
			return -EAGAIN;
		}

		sm5708_flash_current_write(led, curr);
		sm5708_flash_timeout_write(led, timeout);

		LED_DBG("curr: %d->%d timeout: %d->%llu budget: %lld\n",
				led->flash_current, led->flash_current_hw,
				led->flash_timeout, timeout, led->budget);

		queue_delayed_work(led->pmic->wq, &led->timeout_work,
				usecs_to_jiffies(timeout));
	} else {
		cancel_delayed_work(&led->timeout_work);
	}

	return sm5708_flash_set_mode(led, MODE_REQ_FLASH, state);
}

static int sm5708_flash_fault_get(struct led_classdev_flash *flash, u32 *fault)
{
	struct sm5708_fled *led = to_fled(&flash->led_cdev);
	u32 val = 0;
	int ret;

	ret = regmap_read(led->rmap, PMIC_REG_STATUS(3), &val);
	*fault = 0;
	if (led->budget < 0)
		*fault |= LED_FAULT_LED_OVER_TEMPERATURE;
	if (val & BIT(2 * led->index))
		*fault |= LED_FAULT_SHORT_CIRCUIT;
	if (val & BIT(2 * led->index + 1))
		*fault |= LED_FAULT_OVER_VOLTAGE;
	return ret;
}

static const struct led_flash_ops sm5708_flash_ops = {
	.flash_brightness_set = sm5708_flash_brightness_set,
	.strobe_get = sm5708_flash_strobe_get,
	.strobe_set = sm5708_flash_strobe_set,
	.timeout_set = sm5708_flash_timeout_set,
	.fault_get = sm5708_flash_fault_get,
};

static void sm5708_flash_add_range(struct led_flash_setting *s,
		struct fwnode_handle *fwnode, const char *prop_name,
		const struct linear_range *r, int ranges, u32 def_max)
{
	fwnode_property_read_u32(fwnode, prop_name, &def_max);
	def_max = clamp(def_max, r->min, linear_range_get_max_value(r + ranges - 1));
	s->val = s->min = r->min;
	s->step = r->step;
	s->max = def_max;
}

static void sm5708_flash_init(struct sm5708_pmic *pmic, struct fwnode_handle *fwnode)
{
	struct led_init_data init_data = { 0 };
	struct v4l2_flash_config sd_cfg = { 0 };
	struct led_classdev_flash *flash;
	struct device *dev = pmic->dev;
	struct led_classdev *lcdev;
	struct sm5708_fled *led;
	unsigned index;
	int ret;

	ret = fwnode_property_read_u32(fwnode, "reg", &index);
	if (ret || index >= SM5708_NUM_LEDS || pmic->leds[index].pmic)
		return;

	led = &pmic->leds[index];
	led->pmic = pmic;
	led->rmap = pmic->rmap;
	led->base = PMIC_LED_BASE(index);
	led->index = index;

	INIT_DELAYED_WORK(&led->timeout_work, sm5708_flash_timeout_work);

	flash = &led->flash;

	sm5708_flash_add_range(&flash->timeout, fwnode, "flash-max-timeout-us",
			&flash_timeout_range, 1, 200000);
	sm5708_flash_add_range(&flash->brightness, fwnode, "flash-max-microamp",
			flash_isel_ranges[index], FLASH_N_RANGES, 300000);
	sm5708_flash_add_range(&sd_cfg.intensity, fwnode,  "continuous-microamp",
			&torch_isel_range, 1, 100000);

	led->max_avg_current = sd_cfg.intensity.max;

	sm5708_flash_mark_current_change(led, 0, ktime_get_boottime_ns());

	lcdev = &flash->led_cdev;
	lcdev->brightness = LED_OFF;
	lcdev->brightness_set_blocking = sm5708_torch_set;
	lcdev->flags = LED_DEV_CAP_FLASH;
	lcdev->max_brightness = sm5708_get_selector(&torch_isel_range, 1,
						    sd_cfg.intensity.max) + 1;
	init_data.fwnode = fwnode;
#if IS_ENABLED(CONFIG_LEDS_CLASS_FLASH)
	sd_cfg.flash_faults = LED_FAULT_SHORT_CIRCUIT | LED_FAULT_OVER_VOLTAGE;
	sd_cfg.has_external_strobe = false;
	flash->ops = &sm5708_flash_ops;

	if (devm_led_classdev_flash_register_ext(dev, flash, &init_data))
		return;

	snprintf(sd_cfg.dev_name, sizeof(sd_cfg.dev_name),
		 "%s %s", dev_name(lcdev->dev), dev_name(dev));
	led->v4l2_flash = v4l2_flash_init(dev, fwnode, flash, NULL, &sd_cfg);
#elif IS_ENABLED(CONFIG_LEDS_CLASS)
	devm_led_classdev_register_ext(dev, lcdev, &init_data);
#endif
}

#define chk_field(field, min, max) \
	if (info->field < min || info->field > max) \
		return dev_err_probe(dev, -EINVAL, \
				"Invalid battery info: %s = %lld\n", #field, \
				(s64) info->field)
static int sm5708_check_info(struct device *dev,
	struct power_supply_battery_info *info)
{

	chk_field(temp_min, -5, 10);
	chk_field(temp_max, 30, 60);
	chk_field(temp_alert_min, -5, 10);
	chk_field(temp_alert_max, 30, 60);
	chk_field(constant_charge_voltage_max_uv, float_uv_range.min,
			linear_range_get_max_value(&float_uv_range));
	chk_field(constant_charge_current_max_ua, charge_ua_range.min,
			linear_range_get_max_value(&charge_ua_range));
	chk_field(charge_term_current_ua, topoff_ua_range.min,
			linear_range_get_max_value(&topoff_ua_range));
	return 0;
}

static int sm5708_pmic_probe(struct i2c_client *client)
{
	struct power_supply_config psy_config = { 0 };
	struct device *dev = &client->dev;
	struct fwnode_handle *fwnode, *con;
	struct sm5708_pmic *pmic;
	int ret, i;
	u32 val;

	pmic = devm_kzalloc(dev, sizeof(*pmic), GFP_KERNEL);
	if (IS_ERR_OR_NULL(pmic))
		return -ENOMEM;

	mutex_init(&pmic->lock);
	pmic->rmap = devm_regmap_init_i2c(client, &sm5708_pmic_regmap_config);
	if (IS_ERR(pmic->rmap))
		return PTR_ERR(pmic->rmap);

	INIT_DELAYED_WORK(&pmic->mon_work, sm5708_pmic_monitor_work);
	INIT_DELAYED_WORK(&pmic->cable_work, sm5708_pmic_cable_work);

	pmic->max_input_current = mA(900);
	pmic->drvdata.pmic = pmic;
	pmic->dev = dev;
	pmic->usb_nb.notifier_call = sm5708_pmic_extcon_notifier;
	pmic->mode_req = MODE_REQ_INPUT;
	fwnode_property_read_u32(dev_fwnode(dev), "input-current-max-microamp",
			&pmic->max_input_current);
	if (pmic->max_input_current < input_ua_range.min &&
	    pmic->max_input_current > linear_range_get_max_value(&input_ua_range))
		return -EINVAL;
	if (init_charge_on)
		pmic->mode_req |= init_charge_on;
	if (init_discharge)
		pmic->mode_req = 0;
	pmic->wq = alloc_ordered_workqueue("sm5708-pmic", WQ_MEM_RECLAIM);
	dev_set_drvdata(dev, &pmic->drvdata);
	psy_config.drv_data = pmic;

	if (!pmic->wq)
		return -ENOMEM;

	ret = devm_add_action_or_reset(dev, sm5708_pmic_cleanup, pmic);
	if (ret)
		return ret;

	ret = regmap_read(pmic->rmap, PMIC_REG_DEVICEID, &val);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read device id\n");

	if (val != 0xf9)
		return dev_err_probe(dev, -ENODEV,
				"Uknown device ID: %#02x\n", val);

	fwnode = fwnode_find_reference(dev_fwnode(dev), "extcon", 0);
	con = NULL;
	if (!IS_ERR_OR_NULL(fwnode)) {
		con = fwnode_get_named_child_node(fwnode, "connector");
		fwnode_handle_put(fwnode);
	}

	if (!IS_ERR_OR_NULL(con)) {
		pmic->role_sw = fwnode_usb_role_switch_get(con);
		if (ERR_PTR(-EPROBE_DEFER) == pmic->role_sw)
			return -EPROBE_DEFER;

		if (IS_ERR(pmic->role_sw))
			pmic->role_sw = NULL;

		fwnode_handle_put(con);
	}

	if (!pmic->role_sw)
		dev_info(dev, "USB role switch is not found\n");

	pmic->extcon = extcon_get_edev_by_phandle(pmic->dev, 0);
	if (IS_ERR(pmic->extcon))
		return PTR_ERR(pmic->extcon);

	pmic->chg_en_gpio = devm_gpiod_get(dev, "charge-enable", GPIOD_OUT_LOW);
	if (IS_ERR(pmic->chg_en_gpio))
		return dev_err_probe(dev, PTR_ERR(pmic->chg_en_gpio),
				"failed to get charge-enable gpio\n");

	device_for_each_child_node(pmic->dev, fwnode)
		sm5708_flash_init(pmic, fwnode);

	pmic->psy = devm_power_supply_register(dev,
			&sm5708_charger_desc, &psy_config);
	if (IS_ERR(pmic->psy))
		return dev_err_probe(dev, PTR_ERR(pmic->psy),
				"failed to register power supply\n");;

	ret = power_supply_get_battery_info(pmic->psy, &pmic->info);
	ret = ret ?: devm_add_action_or_reset(dev, sm5708_pmic_put_info, pmic);
	ret = ret ?: sm5708_check_info(dev, pmic->info);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(sm5708_pmic_reg_init); i ++) {
		ret = regmap_update_bits(pmic->rmap, sm5708_pmic_reg_init[i].reg,
					 sm5708_pmic_reg_init[i].mask,
					 sm5708_pmic_reg_init[i].val);
		if (ret)
			return ret;
	}

	ret = devm_extcon_register_notifier_all(pmic->dev, pmic->extcon, &pmic->usb_nb);
	if (ret < 0)
		return ret;

	ret = devm_request_threaded_irq(dev, client->irq, NULL,
			sm5708_pmic_irq_handler, IRQF_ONESHOT | IRQF_TRIGGER_LOW,
			"sm5708-pmic", pmic);
	if (ret)
		return dev_err_probe(dev, ret, "failed to request irq\n");

	queue_delayed_work(pmic->wq, &pmic->cable_work, msecs_to_jiffies(0));

	return 0;
}

#define FG_REG_DEVID			0x00
#define REG_DEVID_VAL_SM5708		0x000b

#define FG_REG_CNTL			0x01
#define FG_REG_INT			0x02
#define FG_REG_INT_MASK			0x03
#define FG_REG_INT_STATUS		0x04
#define FG_REG_PARAM_RUN_UPDATE		0x14
#define FG_REG_RS_MAN			0x29
#define FG_REG_VOLT_CAL			0x50
#define FG_REG_CURR_OFF			0x51
#define FG_REG_CURR_P_SLOPE		0x52
#define FG_REG_CURR_N_SLOPE		0x53
/* Common flags for CNTL/INT/MASK/STATUS */
#define INT_VOLT_LOW_MASK		BIT(0)
#define INT_TEMP_LOW_MASK		BIT(1)
#define INT_TEMP_HIGH_MASK		BIT(2)
#define INT_SOC_LOW_MASK		BIT(3)

#define CNTL_RS_MAN_MODE		0x0800

#define FG_REG_SOC			0x05
#define REG_SOC_FRACBITS		8
#define FG_REG_OCV			0x06
#define REG_OCV_FRACBITS		11
#define FG_REG_VOLT			0x07
#define REG_VOLT_FRACBITS		11
#define FG_REG_CURR			0x08
#define REG_CURR_FRACBITS		11
#define FG_REG_TEMP			0x09
#define REG_TEMP_FRACBITS		8

#define FG_REG_SOC_CYCLE		0x0a
#define REG_SOC_CYCLE_MASK		GENMASK(9, 0)

#define FG_REG_THRESH_VOLTAGE_LOW	0x0c
#define FG_REG_THRESH_TEMP		0x0d
#define REG_THRESH_TEMP_LOW_MASK	GENMASK(7, 0)
#define REG_THRESH_TEMP_HIGH_MASK	GENMASK(15, 8)
#define FG_REG_THRESH_SOC_LOW	0x0e

#define FG_REG_OP_STATUS		0x10
#define REG_OP_STATUS_INITIALIZED	BIT(4)
#define REG_RESET_INIT_MARK		0xa000

#define FG_REG_RS_ACTIVE		0x28
#define REG_RS_ACTIVE_FRACBITS		3

#define FG_REG_PARAM_CNTL		0x13
#define REG_PARAM_CNTL_LOCK_MASK	GENMASK(15, 8)
#define REG_PARAM_CNTL_UNLOCKED_VAL	0x3700
#define REG_PARAM_CNTL_TABLE_LEN_MASK	GENMASK(3, 0) /* offset by 1 */

#define FG_REG_RESET		0x91

struct sm5708_bat {
	struct sm5708_drvdata drvdata;
	struct power_supply *psy;
	struct power_supply_battery_info *info;
	struct regmap *rmap;
	struct device *dev;
	int last_health;
	int soc_last;
	struct thermal_zone_device *bat_tz;
};

#define FG_REG_GET_FRAC(bat, reg, mult, _out) ({			\
	int _val, ret;							\
	u32 _rval;							\
	ret = regmap_read(bat->rmap, FG_##reg, &_rval);		\
	if (!ret) {							\
		_val = (_rval & BIT(15) ? -1 : 1) * (_rval & ~BIT(15));	\
		_val = mult_frac(_val, mult, 1 << (reg##_FRACBITS));	\
		*_out = _val;						\
	}								\
	ret; })

static void sm5708_bat_put_info(void *ptr)
{
	struct sm5708_bat *bat = ptr;

	power_supply_put_battery_info(bat->psy, bat->info);
}

static int sm5708_battery_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *prop)
{
	struct sm5708_bat *bat = power_supply_get_drvdata(psy);
	int tmp, ret, *ival = &prop->intval;

	switch (psp) {
	case POWER_SUPPLY_PROP_CAPACITY:
		ret = FG_REG_GET_FRAC(bat, REG_SOC, 1, ival);
		bat->soc_last = *ival;
		return ret;
	case POWER_SUPPLY_PROP_CHARGE_FULL:
		psp = POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN;
		return power_supply_get_property(psy, psp, prop);
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		psp = POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN;
		ret = power_supply_get_property(psy, psp, prop);
		if (ret < 0)
			return ret;

		ret = FG_REG_GET_FRAC(bat, REG_SOC, *ival, &tmp);
		if (ret < 0)
			return ret;

		*ival = clamp(tmp / 100, 0, *ival);
		return ret;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		return FG_REG_GET_FRAC(bat, REG_CURR, MICRO, ival);
	case POWER_SUPPLY_PROP_CYCLE_COUNT:
		return regmap_read(bat->rmap, FG_REG_SOC_CYCLE, &prop->intval);
	case POWER_SUPPLY_PROP_TEMP_AMBIENT:
		return FG_REG_GET_FRAC(bat, REG_TEMP, DECI, ival);
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		return FG_REG_GET_FRAC(bat, REG_VOLT, MICRO, ival);
	case POWER_SUPPLY_PROP_VOLTAGE_OCV:
		return FG_REG_GET_FRAC(bat, REG_OCV, MICRO, ival);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,18,0)
	case POWER_SUPPLY_PROP_INTERNAL_RESISTANCE:
		return FG_REG_GET_FRAC(bat, REG_RS_ACTIVE, 1000, ival);
#endif
	case POWER_SUPPLY_PROP_TEMP:
		ret = thermal_zone_get_temp(bat->bat_tz, &tmp);
		if (!ret)
			*ival = tmp / 100;

		return ret;
	case POWER_SUPPLY_PROP_HEALTH:
		ret = thermal_zone_get_temp(bat->bat_tz, &tmp);
		if (ret < 0 || !bat->info) {
			*ival = POWER_SUPPLY_HEALTH_UNKNOWN;
			return 0;
		}

		tmp /= 1000;

#define TEMP_HYSTERESIS(health, hyst) \
		(bat->last_health == POWER_SUPPLY_HEALTH_##health ? hyst : 0)

		if (tmp <= bat->info->temp_min + TEMP_HYSTERESIS(COLD, 3))
			*ival = POWER_SUPPLY_HEALTH_COLD;
		else if (tmp >= bat->info->temp_max - TEMP_HYSTERESIS(HOT, 5))
			*ival = POWER_SUPPLY_HEALTH_HOT;
		else if (tmp <= bat->info->temp_alert_min + TEMP_HYSTERESIS(COOL, 3))
			*ival = POWER_SUPPLY_HEALTH_COOL;
		else if (tmp >= bat->info->temp_alert_max - TEMP_HYSTERESIS(WARM, 5))
			*ival = POWER_SUPPLY_HEALTH_WARM;
		else
			*ival = POWER_SUPPLY_HEALTH_GOOD;

		bat->last_health = *ival;
		return 0;
	default:
		return -EINVAL;
	}

	return 0;
}

static const struct regmap_config sm5708_bat_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 16,
	.val_format_endian = REGMAP_ENDIAN_LITTLE,
	.max_register	= 0xff,
};

static const enum power_supply_property sm5708_battery_properties[] = {
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CHARGE_FULL,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CYCLE_COUNT,
	POWER_SUPPLY_PROP_HEALTH,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,18,0)
	POWER_SUPPLY_PROP_INTERNAL_RESISTANCE,
#endif
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_TEMP_AMBIENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_VOLTAGE_OCV,
};

static const struct power_supply_desc sm5708_bat_desc = {
	.name		= PSY_BATTERY_NAME,
	.type		= POWER_SUPPLY_TYPE_BATTERY,
	.get_property	= sm5708_battery_get_property,
	.properties	= sm5708_battery_properties,
	.num_properties	= ARRAY_SIZE(sm5708_battery_properties),
};

static int sm5708_battery_configure(struct sm5708_bat *bat)
{
	struct regmap *m = bat->rmap;
	u32 op_sts, reset, pcntl;
	int ret = 0;

	ret = regmap_read(m, FG_REG_OP_STATUS, &op_sts);
	ret = ret ?: regmap_read(m, FG_REG_PARAM_CNTL, &pcntl);
	ret = ret ?: regmap_read(m, FG_REG_RESET, &reset);
	if (ret)
		return ret;

	if (!(pcntl == 15 && (op_sts & REG_OP_STATUS_INITIALIZED) &&
	    reset == REG_RESET_INIT_MARK)) {
		/* Boot-loader does this anyways */
		dev_err(bat->dev, "Battery gauge chip initialization is not supported\n");
		return -EINVAL;
	}

	/* Undo any adjustments made by stock android driver */
	regmap_write(m, FG_REG_VOLT_CAL, 0x8000);
	regmap_write(m, FG_REG_CURR_OFF, 0);
	regmap_write(m, FG_REG_CURR_P_SLOPE, 138);
	regmap_write(m, FG_REG_CURR_N_SLOPE, 138);

	regmap_write(m, FG_REG_PARAM_RUN_UPDATE, 1);
	regmap_write(m, FG_REG_RS_MAN, 64);
	regmap_write(m, FG_REG_PARAM_RUN_UPDATE, 0);

	regmap_update_bits(m, FG_REG_CNTL, CNTL_RS_MAN_MODE, CNTL_RS_MAN_MODE);

	return 0;
}

static irqreturn_t sm5708_bat_irq_thread(int irq, void *data)
{
	struct sm5708_bat *bat = data;
	u32 intval;
	int ret;

	ret = regmap_read(bat->rmap, FG_REG_INT, &intval);
	if (ret)
		return IRQ_HANDLED;

	power_supply_changed(bat->psy);

	if (enable_debug)
		dev_dbg_ratelimited(bat->dev, "IRQ: %x\n", intval);

	return IRQ_HANDLED;
}

static int sm5708_battery_probe(struct i2c_client *client)
{
	struct power_supply_config cfg = { 0 };
	struct device *dev = &client->dev;
	struct sm5708_bat *bat;
	unsigned int devid;
	int ret;

	if (of_device_is_compatible(dev->of_node, "siliconmitus,sm5708-pmic"))
		return sm5708_pmic_probe(client);

	bat = devm_kzalloc(&client->dev, sizeof(*bat), GFP_KERNEL);
	if (!bat)
		return -ENOMEM;

	bat->bat_tz = thermal_zone_get_zone_by_name("battery-thermal");
	if (IS_ERR_OR_NULL(bat->bat_tz)) {
		ret = PTR_ERR(bat->bat_tz) ?: -ENODATA;
		return dev_err_probe(dev, ret == -ENODEV ? -EPROBE_DEFER : ret,
				     "failed to get battery thermal zone\n");
	}

	i2c_set_clientdata(client, bat);

	bat->drvdata.bat = bat;
	bat->dev = dev;
	bat->rmap = devm_regmap_init_i2c(client, &sm5708_bat_regmap_config);
	dev_set_drvdata(dev, &bat->drvdata);
	if (IS_ERR(bat->rmap))
		return PTR_ERR(bat->rmap);

	ret = regmap_read(bat->rmap, FG_REG_DEVID, &devid);
	if (ret)
		return dev_err_probe(bat->dev, -ENODEV,
				"Failed to read DEV_ID\n");

	if (devid != REG_DEVID_VAL_SM5708)
		return dev_err_probe(bat->dev, -ENODEV,
				"Unknown DEV_ID: %x\n", devid);

	ret = sm5708_battery_configure(bat);
	if (ret)
		return dev_err_probe(bat->dev, ret, "Failed to configure chip");

	cfg.drv_data = bat;

	bat->psy = devm_power_supply_register(bat->dev, &sm5708_bat_desc, &cfg);
	if (IS_ERR(bat->psy))
		return PTR_ERR(bat->psy);

	ret = power_supply_get_battery_info(bat->psy, &bat->info);
	ret = ret ?: devm_add_action_or_reset(dev, sm5708_bat_put_info, bat);
	if (ret)
		return ret;

	ret = devm_request_threaded_irq(bat->dev, client->irq,
					NULL, sm5708_bat_irq_thread,
					IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					PSY_BATTERY_NAME, bat);
	if (ret)
		return dev_err_probe(bat->dev, ret, "failed to request irq\n");

	return 0;
}

static int sm5708_system_prepare(struct device *dev)
{
	struct sm5708_drvdata *data = dev_get_drvdata(dev);
	struct sm5708_pmic *pmic = data->pmic;
	union power_supply_propval status;
	int ret;

	if (!pmic)
		return 0;

	ret = power_supply_get_property(pmic->psy, POWER_SUPPLY_PROP_STATUS, &status);
	if (ret || (status.intval != POWER_SUPPLY_STATUS_CHARGING))
		return 0;

	return -EBUSY;
}

static int sm5708_system_suspend(struct device *dev)
{
	struct sm5708_drvdata *data = dev_get_drvdata(dev);
	struct sm5708_pmic *pmic = data->pmic;
	struct sm5708_fled *led;

	if (pmic) {
		pmic->suspended = true;

		for (led = pmic->leds; led < &pmic->leds[SM5708_NUM_LEDS]; led ++)
			if (led->pmic)
				flush_delayed_work(&led->timeout_work);

		flush_delayed_work(&pmic->mon_work);
		flush_delayed_work(&pmic->cable_work);
		sm5708_pmic_update_mode(pmic, 0, 0);
	}

	return 0;
}

static int sm5708_system_resume(struct device *dev)
{
	struct sm5708_drvdata *data = dev_get_drvdata(dev);
	struct sm5708_pmic *pmic = data->pmic;

	if (pmic) {
		pmic->suspended = false;

		queue_delayed_work(pmic->wq, &pmic->cable_work, msecs_to_jiffies(0));
		if (pmic->online)
			queue_delayed_work(pmic->wq, &pmic->mon_work, msecs_to_jiffies(0));
	}

	return 0;
}

static const struct dev_pm_ops sm5708_pm_ops = {
	.prepare = sm5708_system_prepare,
	SET_SYSTEM_SLEEP_PM_OPS(sm5708_system_suspend, sm5708_system_resume)
};

static const struct of_device_id sm5708_of_match[] = {
	{ .compatible = "siliconmitus,sm5708-battery" },
	{ .compatible = "siliconmitus,sm5708-pmic" },
	{ },
};

static struct i2c_driver sm5708_driver = {
	.driver = {
		   .name = "sm5708",
		   .of_match_table = sm5708_of_match,
		   .pm = &sm5708_pm_ops,
	},
	.probe = sm5708_battery_probe,
};

module_param(init_charge_on, bool, 0644);
module_param(init_discharge, bool, 0644);
module_param(enable_debug, bool, 0644);
module_i2c_driver(sm5708_driver);

MODULE_AUTHOR("Vladimir Lypak <vladimir.lypak@gmail.com>");
MODULE_DESCRIPTION("Siliconmitus SM5708 PMIC and battery gauge driver");
MODULE_DEVICE_TABLE(of, sm5708_of_match);
MODULE_LICENSE("GPL v2");
