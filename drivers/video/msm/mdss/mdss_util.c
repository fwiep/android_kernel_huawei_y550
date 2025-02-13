
/* Copyright (c) 2007-2014, The Linux Foundation. All rights reserved.
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

#define pr_fmt(fmt)	"%s: " fmt, __func__

#include <linux/interrupt.h>
#include "mdss_mdp.h"

struct mdss_hw *mdss_irq_handlers[MDSS_MAX_HW_BLK];
static DEFINE_SPINLOCK(mdss_lock);

int mdss_register_irq(struct mdss_hw *hw)
{
	unsigned long irq_flags;
	u32 ndx_bit;

	if (!hw || hw->hw_ndx >= MDSS_MAX_HW_BLK)
		return -EINVAL;

	ndx_bit = BIT(hw->hw_ndx);

	spin_lock_irqsave(&mdss_lock, irq_flags);
	if (!mdss_irq_handlers[hw->hw_ndx])
		mdss_irq_handlers[hw->hw_ndx] = hw;
	else
		pr_err("panel %d's irq at %p is already registered\n",
			hw->hw_ndx, hw->irq_handler);
	spin_unlock_irqrestore(&mdss_lock, irq_flags);

	return 0;
}

void mdss_enable_irq(struct mdss_hw *hw)
{
	unsigned long irq_flags;
	u32 ndx_bit;

	if (hw->hw_ndx >= MDSS_MAX_HW_BLK)
		return;

	if (!mdss_irq_handlers[hw->hw_ndx]) {
		pr_err("failed. First register the irq then enable it.\n");
		return;
	}

	ndx_bit = BIT(hw->hw_ndx);

	pr_debug("Enable HW=%d irq ena=%d mask=%x\n", hw->hw_ndx,
			mdss_res->irq_ena, mdss_res->irq_mask);

	spin_lock_irqsave(&mdss_lock, irq_flags);
	if (mdss_res->irq_mask & ndx_bit) {
		pr_debug("MDSS HW ndx=%d is already set, mask=%x\n",
				hw->hw_ndx, mdss_res->irq_mask);
	} else {
		mdss_res->irq_mask |= ndx_bit;
		if (!mdss_res->irq_ena) {
			mdss_res->irq_ena = true;
			enable_irq(mdss_res->irq);
		}
	}
	spin_unlock_irqrestore(&mdss_lock, irq_flags);
}

void mdss_disable_irq(struct mdss_hw *hw)
{
	unsigned long irq_flags;
	u32 ndx_bit;

	if (hw->hw_ndx >= MDSS_MAX_HW_BLK)
		return;

	ndx_bit = BIT(hw->hw_ndx);

	pr_debug("Disable HW=%d irq ena=%d mask=%x\n", hw->hw_ndx,
			mdss_res->irq_ena, mdss_res->irq_mask);

	spin_lock_irqsave(&mdss_lock, irq_flags);
	if (!(mdss_res->irq_mask & ndx_bit)) {
		pr_warn("MDSS HW ndx=%d is NOT set, mask=%x, hist mask=%x\n",
			hw->hw_ndx, mdss_res->mdp_irq_mask,
			mdss_res->mdp_hist_irq_mask);
	} else {
		mdss_res->irq_mask &= ~ndx_bit;
		if (mdss_res->irq_mask == 0) {
			mdss_res->irq_ena = false;
			disable_irq_nosync(mdss_res->irq);
		}
	}
	spin_unlock_irqrestore(&mdss_lock, irq_flags);
}

/* called from interrupt context */
void mdss_disable_irq_nosync(struct mdss_hw *hw)
{
	u32 ndx_bit;

	if (hw->hw_ndx >= MDSS_MAX_HW_BLK)
		return;

	ndx_bit = BIT(hw->hw_ndx);

	pr_debug("Disable HW=%d irq ena=%d mask=%x\n", hw->hw_ndx,
			mdss_res->irq_ena, mdss_res->irq_mask);

	spin_lock(&mdss_lock);
	if (!(mdss_res->irq_mask & ndx_bit)) {
		pr_warn("MDSS HW ndx=%d is NOT set, mask=%x, hist mask=%x\n",
			hw->hw_ndx, mdss_res->mdp_irq_mask,
			mdss_res->mdp_hist_irq_mask);
	} else {
		mdss_res->irq_mask &= ~ndx_bit;
		if (mdss_res->irq_mask == 0) {
			mdss_res->irq_ena = false;
			disable_irq_nosync(mdss_res->irq);
		}
	}
	spin_unlock(&mdss_lock);
}

int mdss_irq_dispatch(u32 hw_ndx, int irq, void *ptr)
{
	struct mdss_hw *hw;
	int rc = -ENODEV;

	spin_lock(&mdss_lock);
	hw = mdss_irq_handlers[hw_ndx];
	spin_unlock(&mdss_lock);

	if (hw)
		rc = hw->irq_handler(irq, hw->ptr);

	return rc;
}

struct mdss_util_intf mdss_util = {
	mdss_register_irq,
	mdss_enable_irq,
	mdss_disable_irq,
	mdss_disable_irq_nosync,
	mdss_irq_dispatch
};

struct mdss_util_intf *mdss_get_util_intf()
{
	return &mdss_util;
}
EXPORT_SYMBOL(mdss_get_util_intf);
