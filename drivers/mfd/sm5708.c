/*
 * sm5708.c - mfd core driver for the SM5708
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/mfd/core.h>
#include <linux/regulator/machine.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

#include <linux/mfd/sm5708.h>

#define MFD_DEV_NAME "sm5708-mfd"

#define SM5708_IRQ(name, regnum) \
	REGMAP_IRQ_REG(name##_IRQ, regnum - 1, name##_STATUS##regnum)

static const struct regmap_irq sm5708_irqs[] = {
	SM5708_IRQ(SM5708_VBUSPOK,     1),
	SM5708_IRQ(SM5708_VBUSUVLO,    1),
	SM5708_IRQ(SM5708_VBUSOVP,     1),
	SM5708_IRQ(SM5708_VBUSLIMIT,   1),

	SM5708_IRQ(SM5708_AICL,        2),
	SM5708_IRQ(SM5708_BATOVP,      2),
	SM5708_IRQ(SM5708_NOBAT,       2),
	SM5708_IRQ(SM5708_CHGON,       2),
	SM5708_IRQ(SM5708_Q4FULLON,    2),
	SM5708_IRQ(SM5708_TOPOFF,      2),
	SM5708_IRQ(SM5708_DONE,        2),
	SM5708_IRQ(SM5708_WDTMROFF,    2),

	SM5708_IRQ(SM5708_THEMREG,     3),
	SM5708_IRQ(SM5708_THEMSHDN,    3),
	SM5708_IRQ(SM5708_OTGFAIL,     3),
	SM5708_IRQ(SM5708_DISLIMIT,    3),
	SM5708_IRQ(SM5708_PRETMROFF,   3),
	SM5708_IRQ(SM5708_FASTTMROFF,  3),
	SM5708_IRQ(SM5708_LOWBATT,     3),
	SM5708_IRQ(SM5708_nENQ4,       3),

	SM5708_IRQ(SM5708_FLED1SHORT,  4),
	SM5708_IRQ(SM5708_FLED1OPEN,   4),
	SM5708_IRQ(SM5708_FLED2SHORT,  4),
	SM5708_IRQ(SM5708_FLED2OPEN,   4),
	SM5708_IRQ(SM5708_BOOSTPOK_NG, 4),
	SM5708_IRQ(SM5708_BOOSTPOK,    4),
	SM5708_IRQ(SM5708_ABSTMR1OFF,  4),
	SM5708_IRQ(SM5708_SBPS,        4),
};

static bool sm5708_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case SM5708_REG_INT1 ... SM5708_REG_INT4:
	case SM5708_REG_STATUS1 ... SM5708_REG_STATUS4:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config sm5708_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
	.volatile_reg	= sm5708_volatile_reg,
	.max_register	= SM5708_REG_MAX,
};

void sm5708_request_mode(struct device *dev, u8 mode_mask, bool enable) {
	struct sm5708_chip *chip = (struct sm5708_chip*) dev_get_drvdata(dev);
	u8 new_mode;
	int boost_output = SM5708_BST_OUT_4v5;
	int otg_current = SM5708_OTG_CURRENT_500mA;
	int op_mode = SM5708_MODE_CHG_ON;

	mutex_lock(&chip->mode_mutex);

	if (enable)
		new_mode = chip->requested_mode | mode_mask;
	else
		new_mode = chip->requested_mode & ~mode_mask;

	if (new_mode == chip->requested_mode)
		goto unlock;

	switch(new_mode & ~SM5708_TORCH) {
		case SM5708_FLASH:
		case SM5708_FLASH | SM5708_CHARGE:
			op_mode = SM5708_MODE_FLASH_BOOST;
			break;
		case SM5708_FLASH | SM5708_OTG:
			op_mode = SM5708_MODE_FLASH_BOOST;
			otg_current = SM5708_OTG_CURRENT_900mA;
			break;
		case SM5708_OTG:
			otg_current = SM5708_OTG_CURRENT_900mA;
			boost_output = SM5708_BST_OUT_5v1;
			op_mode = SM5708_MODE_USB_OTG;
			break;
		default:
			if (new_mode == SM5708_TORCH)
				op_mode = SM5708_MODE_FLASH_BOOST;
	}

	if (chip->op_mode == SM5708_MODE_USB_OTG &&
		op_mode == SM5708_MODE_CHG_ON) {
		regmap_update_bits(chip->regmap, SM5708_REG_CNTL, 0x7,
				SM5708_MODE_SUSPEND);
		msleep(80);
	}

	regmap_update_bits(chip->regmap, SM5708_REG_FLEDCNTL6, 0xf, boost_output);
	regmap_update_bits(chip->regmap, SM5708_REG_CHGCNTL5, 0x3 << 2, otg_current << 2);
	regmap_update_bits(chip->regmap, SM5708_REG_CNTL, 0x7, op_mode);

	chip->op_mode = op_mode;
	chip->requested_mode = new_mode;
unlock:
	mutex_unlock(&chip->mode_mutex);
}
EXPORT_SYMBOL(sm5708_request_mode);

static const struct regmap_irq_chip sm5708_irq_chip = {
	.name		= MFD_DEV_NAME,
	.status_base	= SM5708_REG_INT1,
	.mask_base	= SM5708_REG_INTMSK1,
	.num_regs	= 4,
	.irqs		= sm5708_irqs,
	.num_irqs	= ARRAY_SIZE(sm5708_irqs),
};

static struct mfd_cell sm5708_devs[] = {
	{ .name = "sm5708-charger", .of_compatible = "siliconmitus,sm5708-charger" },
	{ .name = "sm5708-rgb-leds", .of_compatible = "siliconmitus,sm5708-leds" },
	{ .name = "sm5708-fled", .of_compatible = "siliconmitus,sm5708-fled" },
};

static int sm5708_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *dev_id)
{
	struct sm5708_chip *chip;
	int ret = 0;

	chip = devm_kzalloc(&i2c->dev, sizeof(struct sm5708_chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	i2c_set_clientdata(i2c, chip);
	chip->dev = &i2c->dev;
	chip->i2c = i2c;
	chip->irq = i2c->irq;
	mutex_init(&chip->mode_mutex);

	chip->regmap = devm_regmap_init_i2c(i2c,
			&sm5708_regmap_config);

	ret = regmap_add_irq_chip(chip->regmap, chip->irq,
			IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND,
			0, &sm5708_irq_chip, &chip->irq_data);
	if (ret) {
		dev_err(&i2c->dev, "Failed to add IRQ chip: %d\n", ret);
		return ret;
	}

	ret = devm_mfd_add_devices(chip->dev, -1, sm5708_devs,
			ARRAY_SIZE(sm5708_devs), NULL, 0, NULL);
	if (ret < 0)
		return ret;

	device_init_wakeup(chip->dev, 0);

	return 0;
}

static int sm5708_suspend(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_chip *chip = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		enable_irq_wake(chip->irq);

	disable_irq(chip->irq);

	return 0;
}

static int sm5708_resume(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_chip *chip = i2c_get_clientdata(i2c);

	if (device_may_wakeup(dev))
		disable_irq_wake(chip->irq);

	enable_irq(chip->irq);

	return 0;
}

static void sm5708_shutdown(struct device *dev)
{
	struct i2c_client *i2c = container_of(dev, struct i2c_client, dev);
	struct sm5708_chip *chip = i2c_get_clientdata(i2c);

	regmap_update_bits(chip->regmap, SM5708_REG_CNTL, 0x7,
			SM5708_MODE_CHG_ON);
}

static SIMPLE_DEV_PM_OPS(sm5708_pm, sm5708_suspend, sm5708_resume);

static const struct of_device_id sm5708_i2c_dt_ids[] = {
	{ .compatible = "siliconmitus,sm5708" },
	{ },
};
MODULE_DEVICE_TABLE(of, sm5708_i2c_dt_ids);

static struct i2c_driver sm5708_i2c_driver = {
	.driver		= {
		.name	= MFD_DEV_NAME,
		.owner	= THIS_MODULE,
		.pm	= &sm5708_pm,
		.of_match_table	= sm5708_i2c_dt_ids,
		.shutdown = sm5708_shutdown,
	},
	.probe		= sm5708_i2c_probe,
};

module_i2c_driver(sm5708_i2c_driver);

MODULE_DESCRIPTION("SM5708 multi-function core driver");
MODULE_LICENSE("GPL");
