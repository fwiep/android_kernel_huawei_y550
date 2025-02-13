/*
 * wm_adsp.c  --  Wolfson ADSP support
 *
 * Copyright 2012 Wolfson Microelectronics plc
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/list.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/jack.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include <linux/mfd/arizona/registers.h>

#include "arizona.h"
#include "wm_adsp.h"

#define adsp_crit(_dsp, fmt, ...) \
	dev_crit(_dsp->dev, "DSP%d: " fmt, _dsp->num, ##__VA_ARGS__)
#define adsp_err(_dsp, fmt, ...) \
	dev_err(_dsp->dev, "DSP%d: " fmt, _dsp->num, ##__VA_ARGS__)
#define adsp_warn(_dsp, fmt, ...) \
	dev_warn(_dsp->dev, "DSP%d: " fmt, _dsp->num, ##__VA_ARGS__)
#define adsp_info(_dsp, fmt, ...) \
	dev_info(_dsp->dev, "DSP%d: " fmt, _dsp->num, ##__VA_ARGS__)
#define adsp_dbg(_dsp, fmt, ...) \
	dev_dbg(_dsp->dev, "DSP%d: " fmt, _dsp->num, ##__VA_ARGS__)

#define ADSP1_CONTROL_1                   0x00
#define ADSP1_CONTROL_2                   0x02
#define ADSP1_CONTROL_3                   0x03
#define ADSP1_CONTROL_4                   0x04
#define ADSP1_CONTROL_5                   0x06
#define ADSP1_CONTROL_6                   0x07
#define ADSP1_CONTROL_7                   0x08
#define ADSP1_CONTROL_8                   0x09
#define ADSP1_CONTROL_9                   0x0A
#define ADSP1_CONTROL_10                  0x0B
#define ADSP1_CONTROL_11                  0x0C
#define ADSP1_CONTROL_12                  0x0D
#define ADSP1_CONTROL_13                  0x0F
#define ADSP1_CONTROL_14                  0x10
#define ADSP1_CONTROL_15                  0x11
#define ADSP1_CONTROL_16                  0x12
#define ADSP1_CONTROL_17                  0x13
#define ADSP1_CONTROL_18                  0x14
#define ADSP1_CONTROL_19                  0x16
#define ADSP1_CONTROL_20                  0x17
#define ADSP1_CONTROL_21                  0x18
#define ADSP1_CONTROL_22                  0x1A
#define ADSP1_CONTROL_23                  0x1B
#define ADSP1_CONTROL_24                  0x1C
#define ADSP1_CONTROL_25                  0x1E
#define ADSP1_CONTROL_26                  0x20
#define ADSP1_CONTROL_27                  0x21
#define ADSP1_CONTROL_28                  0x22
#define ADSP1_CONTROL_29                  0x23
#define ADSP1_CONTROL_30                  0x24
#define ADSP1_CONTROL_31                  0x26

/*
 * ADSP1 Control 19
 */
#define ADSP1_WDMA_BUFFER_LENGTH_MASK     0x00FF  /* DSP1_WDMA_BUFFER_LENGTH - [7:0] */
#define ADSP1_WDMA_BUFFER_LENGTH_SHIFT         0  /* DSP1_WDMA_BUFFER_LENGTH - [7:0] */
#define ADSP1_WDMA_BUFFER_LENGTH_WIDTH         8  /* DSP1_WDMA_BUFFER_LENGTH - [7:0] */


/*
 * ADSP1 Control 30
 */
#define ADSP1_DBG_CLK_ENA                 0x0008  /* DSP1_DBG_CLK_ENA */
#define ADSP1_DBG_CLK_ENA_MASK            0x0008  /* DSP1_DBG_CLK_ENA */
#define ADSP1_DBG_CLK_ENA_SHIFT                3  /* DSP1_DBG_CLK_ENA */
#define ADSP1_DBG_CLK_ENA_WIDTH                1  /* DSP1_DBG_CLK_ENA */
#define ADSP1_SYS_ENA                     0x0004  /* DSP1_SYS_ENA */
#define ADSP1_SYS_ENA_MASK                0x0004  /* DSP1_SYS_ENA */
#define ADSP1_SYS_ENA_SHIFT                    2  /* DSP1_SYS_ENA */
#define ADSP1_SYS_ENA_WIDTH                    1  /* DSP1_SYS_ENA */
#define ADSP1_CORE_ENA                    0x0002  /* DSP1_CORE_ENA */
#define ADSP1_CORE_ENA_MASK               0x0002  /* DSP1_CORE_ENA */
#define ADSP1_CORE_ENA_SHIFT                   1  /* DSP1_CORE_ENA */
#define ADSP1_CORE_ENA_WIDTH                   1  /* DSP1_CORE_ENA */
#define ADSP1_START                       0x0001  /* DSP1_START */
#define ADSP1_START_MASK                  0x0001  /* DSP1_START */
#define ADSP1_START_SHIFT                      0  /* DSP1_START */
#define ADSP1_START_WIDTH                      1  /* DSP1_START */

/*
 * ADSP1 Control 31
 */
#define ADSP1_CLK_SEL_MASK                0x0007  /* CLK_SEL_ENA */
#define ADSP1_CLK_SEL_SHIFT                    0  /* CLK_SEL_ENA */
#define ADSP1_CLK_SEL_WIDTH                    3  /* CLK_SEL_ENA */

#define ADSP2_CONTROL        0x0
#define ADSP2_CLOCKING       0x1
#define ADSP2_STATUS1        0x4
#define ADSP2_WDMA_CONFIG_1 0x30
#define ADSP2_WDMA_CONFIG_2 0x31
#define ADSP2_RDMA_CONFIG_1 0x34

/*
 * ADSP2 Control
 */

#define ADSP2_MEM_ENA                     0x0010  /* DSP1_MEM_ENA */
#define ADSP2_MEM_ENA_MASK                0x0010  /* DSP1_MEM_ENA */
#define ADSP2_MEM_ENA_SHIFT                    4  /* DSP1_MEM_ENA */
#define ADSP2_MEM_ENA_WIDTH                    1  /* DSP1_MEM_ENA */
#define ADSP2_SYS_ENA                     0x0004  /* DSP1_SYS_ENA */
#define ADSP2_SYS_ENA_MASK                0x0004  /* DSP1_SYS_ENA */
#define ADSP2_SYS_ENA_SHIFT                    2  /* DSP1_SYS_ENA */
#define ADSP2_SYS_ENA_WIDTH                    1  /* DSP1_SYS_ENA */
#define ADSP2_CORE_ENA                    0x0002  /* DSP1_CORE_ENA */
#define ADSP2_CORE_ENA_MASK               0x0002  /* DSP1_CORE_ENA */
#define ADSP2_CORE_ENA_SHIFT                   1  /* DSP1_CORE_ENA */
#define ADSP2_CORE_ENA_WIDTH                   1  /* DSP1_CORE_ENA */
#define ADSP2_START                       0x0001  /* DSP1_START */
#define ADSP2_START_MASK                  0x0001  /* DSP1_START */
#define ADSP2_START_SHIFT                      0  /* DSP1_START */
#define ADSP2_START_WIDTH                      1  /* DSP1_START */

/*
 * ADSP2 clocking
 */
#define ADSP2_CLK_SEL_MASK                0x0007  /* CLK_SEL_ENA */
#define ADSP2_CLK_SEL_SHIFT                    0  /* CLK_SEL_ENA */
#define ADSP2_CLK_SEL_WIDTH                    3  /* CLK_SEL_ENA */

/*
 * ADSP2 Status 1
 */
#define ADSP2_RAM_RDY                     0x0001
#define ADSP2_RAM_RDY_MASK                0x0001
#define ADSP2_RAM_RDY_SHIFT                    0
#define ADSP2_RAM_RDY_WIDTH                    1

struct wm_adsp_buf {
	struct list_head list;
	void *buf;
};

static struct wm_adsp_buf *wm_adsp_buf_alloc(const void *src, size_t len,
					     struct list_head *list)
{
	struct wm_adsp_buf *buf = kzalloc(sizeof(*buf), GFP_KERNEL);

	if (buf == NULL)
		return NULL;

	buf->buf = kmemdup(src, len, GFP_KERNEL | GFP_DMA);
	if (!buf->buf) {
		kfree(buf);
		return NULL;
	}

	if (list)
		list_add_tail(&buf->list, list);

	return buf;
}

static void wm_adsp_buf_free(struct list_head *list)
{
	while (!list_empty(list)) {
		struct wm_adsp_buf *buf = list_first_entry(list,
							   struct wm_adsp_buf,
							   list);
		list_del(&buf->list);
		kfree(buf->buf);
		kfree(buf);
	}
}

#define WM_ADSP_NUM_FW 5

#define WM_ADSP_FW_MBC_VSS        0
#define WM_ADSP_FW_TX             1
#define WM_ADSP_FW_TX_SPK         2
#define WM_ADSP_FW_RX_ANC         3
#define WM_ADSP_FW_EZ2CONTROL 	  4

static const char *wm_adsp_fw_text[WM_ADSP_NUM_FW] = {
	[WM_ADSP_FW_MBC_VSS] =    "MBC/VSS",
	[WM_ADSP_FW_TX] =         "Tx",
	[WM_ADSP_FW_TX_SPK] =     "Tx Speaker",
	[WM_ADSP_FW_RX_ANC] =     "Rx ANC",
	[WM_ADSP_FW_EZ2CONTROL] = "Ez2Control",
};

struct wm_adsp_buffer_region_def {
       unsigned int mem_type;
       unsigned int base_offset;
       unsigned int size_offset;
};

struct wm_adsp_fw_caps {
	u32 id;
	struct snd_codec_desc desc;
	int num_host_regions;
	struct wm_adsp_buffer_region_def *host_region_defs;
};

struct wm_adsp_system_config_xm_hdr {
	__be32 sys_enable;
	__be32 fw_id;
	__be32 fw_rev;
	__be32 boot_status;
	__be32 watchdog;
	__be32 dma_buffer_size;
	__be32 rdma[6];
	__be32 wdma[8];
	__be32 build_job_name[3];
	__be32 build_job_number;
};

struct wm_adsp_alg_xm_struct {
	__be32 magic;
	__be32 smoothing;
	__be32 threshold;
	__be32 host_buf_ptr;
	__be32 start_seq;
	__be32 high_water_mark;
	__be32 low_water_mark;
	__be64 smoothed_power;
};

struct wm_adsp_host_buffer {
	__be32 X_buf_base;
	__be32 X_buf_size;
	__be32 X_buf_base2;
	__be32 X_buf_size2;
	__be32 Y_buf_base;
	__be32 Y_buf_size;
	__be32 high_water_mark;
	__be32 low_water_mark;
	__be32 next_write_index;
	__be32 next_read_index;
	__be32 overflow;
	__be32 state;
	__be32 wrapped;
	__be32 requested_rewind;
	__be32 applied_rewind;
};

#define WM_ADSP_DATA_WORD_SIZE         3
#define WM_ADSP_ALG_XM_STRUCT_MAGIC    0x58b90c

#define ADSP2_SYSTEM_CONFIG_XM_PTR \
	(offsetof(struct wmfw_adsp2_id_hdr, xm) / sizeof(__be32))

#define WM_ADSP_ALG_XM_PTR \
	(sizeof(struct wm_adsp_system_config_xm_hdr) / sizeof(__be32))

#define HOST_BUFFER_FIELD(field) \
	(offsetof(struct wm_adsp_host_buffer, field) / sizeof(__be32))

#define ALG_XM_FIELD(field) \
	(offsetof(struct wm_adsp_alg_xm_struct, field) / sizeof(__be32))

struct wm_adsp_buffer_region_def ez2control_regions[] = {
	{
		.mem_type = WMFW_ADSP2_XM,
		.base_offset = HOST_BUFFER_FIELD(X_buf_base),
		.size_offset = HOST_BUFFER_FIELD(X_buf_size),
	},
	{
		.mem_type = WMFW_ADSP2_XM,
		.base_offset = HOST_BUFFER_FIELD(X_buf_base2),
		.size_offset = HOST_BUFFER_FIELD(X_buf_size2),
	},
	{
		.mem_type = WMFW_ADSP2_YM,
		.base_offset = HOST_BUFFER_FIELD(Y_buf_base),
		.size_offset = HOST_BUFFER_FIELD(Y_buf_size),
	},
};

static const struct wm_adsp_fw_caps ez2control_caps[] = {
	{
		.id = SND_AUDIOCODEC_PCM,
		.desc = {
			.max_ch = 1,
			.sample_rates = SNDRV_PCM_RATE_16000,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.num_host_regions = ARRAY_SIZE(ez2control_regions),
		.host_region_defs = ez2control_regions,
	},
};

static const struct {
	const char *file;
	int compr_direction;
	int num_caps;
	const struct wm_adsp_fw_caps *caps;
} wm_adsp_fw[WM_ADSP_NUM_FW] = {
	[WM_ADSP_FW_MBC_VSS] =    { .file = "mbc-vss" },
	[WM_ADSP_FW_TX] =         { .file = "tx" },
	[WM_ADSP_FW_TX_SPK] =     { .file = "tx-spk" },
	[WM_ADSP_FW_RX_ANC] =     { .file = "rx-anc" },
	[WM_ADSP_FW_EZ2CONTROL] = {
		.file = "ez2-control",
		.compr_direction = SND_COMPRESS_CAPTURE,
		.num_caps = ARRAY_SIZE(ez2control_caps),
		.caps = ez2control_caps,
	},
};

struct wm_coeff_ctl_ops {
	int (*xget)(struct snd_kcontrol *kcontrol,
		    struct snd_ctl_elem_value *ucontrol);
	int (*xput)(struct snd_kcontrol *kcontrol,
		    struct snd_ctl_elem_value *ucontrol);
	int (*xinfo)(struct snd_kcontrol *kcontrol,
		     struct snd_ctl_elem_info *uinfo);
};

struct wm_coeff {
	struct device *dev;
	struct list_head ctl_list;
	struct regmap *regmap;
};

struct wm_coeff_ctl {
	const char *name;
	struct snd_card *card;
	struct wm_adsp_alg_region region;
	struct wm_coeff_ctl_ops ops;
	struct wm_adsp *adsp;
	void *private;
	unsigned int enabled:1;
	struct list_head list;
	void *cache;
	size_t len;
	unsigned int set:1;
	struct snd_kcontrol *kcontrol;
};

static int wm_adsp_fw_get(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct wm_adsp *adsp = snd_soc_codec_get_drvdata(codec);

	ucontrol->value.integer.value[0] = adsp[e->shift_l].fw;

	return 0;
}

static int wm_adsp_fw_put(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	struct wm_adsp *adsp = snd_soc_codec_get_drvdata(codec);

	if (ucontrol->value.integer.value[0] == adsp[e->shift_l].fw)
		return 0;

	if (ucontrol->value.integer.value[0] >= WM_ADSP_NUM_FW)
		return -EINVAL;

	if (adsp[e->shift_l].running)
		return -EBUSY;

	adsp[e->shift_l].fw = ucontrol->value.integer.value[0];

	return 0;
}

static const struct soc_enum wm_adsp_fw_enum[] = {
	SOC_ENUM_SINGLE(0, 0, ARRAY_SIZE(wm_adsp_fw_text), wm_adsp_fw_text),
	SOC_ENUM_SINGLE(0, 1, ARRAY_SIZE(wm_adsp_fw_text), wm_adsp_fw_text),
	SOC_ENUM_SINGLE(0, 2, ARRAY_SIZE(wm_adsp_fw_text), wm_adsp_fw_text),
	SOC_ENUM_SINGLE(0, 3, ARRAY_SIZE(wm_adsp_fw_text), wm_adsp_fw_text),
};

const struct snd_kcontrol_new wm_adsp1_fw_controls[] = {
	SOC_ENUM_EXT("DSP1 Firmware", wm_adsp_fw_enum[0],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM_EXT("DSP2 Firmware", wm_adsp_fw_enum[1],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM_EXT("DSP3 Firmware", wm_adsp_fw_enum[2],
		     wm_adsp_fw_get, wm_adsp_fw_put),
};
EXPORT_SYMBOL_GPL(wm_adsp1_fw_controls);

#if IS_ENABLED(CONFIG_SND_SOC_ARIZONA)
static const struct soc_enum wm_adsp2_rate_enum[] = {
	SOC_VALUE_ENUM_SINGLE(ARIZONA_DSP1_CONTROL_1,
			      ARIZONA_DSP1_RATE_SHIFT, 0xf,
			      ARIZONA_RATE_ENUM_SIZE,
			      arizona_rate_text, arizona_rate_val),
	SOC_VALUE_ENUM_SINGLE(ARIZONA_DSP2_CONTROL_1,
			      ARIZONA_DSP1_RATE_SHIFT, 0xf,
			      ARIZONA_RATE_ENUM_SIZE,
			      arizona_rate_text, arizona_rate_val),
	SOC_VALUE_ENUM_SINGLE(ARIZONA_DSP3_CONTROL_1,
			      ARIZONA_DSP1_RATE_SHIFT, 0xf,
			      ARIZONA_RATE_ENUM_SIZE,
			      arizona_rate_text, arizona_rate_val),
	SOC_VALUE_ENUM_SINGLE(ARIZONA_DSP4_CONTROL_1,
			      ARIZONA_DSP1_RATE_SHIFT, 0xf,
			      ARIZONA_RATE_ENUM_SIZE,
			      arizona_rate_text, arizona_rate_val),
};

const struct snd_kcontrol_new wm_adsp2_fw_controls[] = {
	SOC_ENUM_EXT("DSP1 Firmware", wm_adsp_fw_enum[0],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM("DSP1 Rate", wm_adsp2_rate_enum[0]),
	SOC_ENUM_EXT("DSP2 Firmware", wm_adsp_fw_enum[1],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM("DSP2 Rate", wm_adsp2_rate_enum[1]),
	SOC_ENUM_EXT("DSP3 Firmware", wm_adsp_fw_enum[2],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM("DSP3 Rate", wm_adsp2_rate_enum[2]),
	SOC_ENUM_EXT("DSP4 Firmware", wm_adsp_fw_enum[3],
		     wm_adsp_fw_get, wm_adsp_fw_put),
	SOC_ENUM("DSP4 Rate", wm_adsp2_rate_enum[3]),
};
EXPORT_SYMBOL_GPL(wm_adsp2_fw_controls);
#endif

static struct wm_adsp_region const *wm_adsp_find_region(struct wm_adsp *dsp,
							int type)
{
	int i;

	for (i = 0; i < dsp->num_mems; i++)
		if (dsp->mem[i].type == type)
			return &dsp->mem[i];

	return NULL;
}

static unsigned int wm_adsp_region_to_reg(struct wm_adsp_region const *region,
					  unsigned int offset)
{
	switch (region->type) {
	case WMFW_ADSP1_PM:
		return region->base + (offset * 3);
	case WMFW_ADSP1_DM:
		return region->base + (offset * 2);
	case WMFW_ADSP2_XM:
		return region->base + (offset * 2);
	case WMFW_ADSP2_YM:
		return region->base + (offset * 2);
	case WMFW_ADSP1_ZM:
		return region->base + (offset * 2);
	default:
		WARN_ON(NULL != "Unknown memory region type");
		return offset;
	}
}

static int wm_coeff_info(struct snd_kcontrol *kcontrol,
			 struct snd_ctl_elem_info *uinfo)
{
	struct wm_coeff_ctl *ctl = (struct wm_coeff_ctl *)kcontrol->private_value;

	uinfo->type = SNDRV_CTL_ELEM_TYPE_BYTES;
	uinfo->count = ctl->len;
	return 0;
}

static int wm_coeff_write_control(struct snd_kcontrol *kcontrol,
				  const void *buf, size_t len)
{
	struct wm_coeff *wm_coeff= snd_kcontrol_chip(kcontrol);
	struct wm_coeff_ctl *ctl = (struct wm_coeff_ctl *)kcontrol->private_value;
	struct wm_adsp_alg_region *region = &ctl->region;
	const struct wm_adsp_region *mem;
	struct wm_adsp *adsp = ctl->adsp;
	void *scratch;
	int ret;
	unsigned int reg;

	mem = wm_adsp_find_region(adsp, region->type);
	if (!mem) {
		adsp_err(adsp, "No base for region %x\n",
			 region->type);
		return -EINVAL;
	}

	reg = ctl->region.base;
	reg = wm_adsp_region_to_reg(mem, reg);

	scratch = kmemdup(buf, ctl->len, GFP_KERNEL | GFP_DMA);
	if (!scratch)
		return -ENOMEM;

	ret = regmap_raw_write(wm_coeff->regmap, reg, scratch,
			       ctl->len);
	if (ret) {
		adsp_err(adsp, "Failed to write %zu bytes to %x\n",
			 ctl->len, reg);
		kfree(scratch);
		return ret;
	}

	kfree(scratch);

	return 0;
}

static int wm_coeff_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct wm_coeff_ctl *ctl = (struct wm_coeff_ctl *)kcontrol->private_value;
	char *p = ucontrol->value.bytes.data;

	memcpy(ctl->cache, p, ctl->len);

	if (!ctl->enabled) {
		ctl->set = 1;
		return 0;
	}

	return wm_coeff_write_control(kcontrol, p, ctl->len);
}

static int wm_coeff_read_control(struct snd_kcontrol *kcontrol,
				 void *buf, size_t len)
{
	struct wm_coeff *wm_coeff= snd_kcontrol_chip(kcontrol);
	struct wm_coeff_ctl *ctl = (struct wm_coeff_ctl *)kcontrol->private_value;
	struct wm_adsp_alg_region *region = &ctl->region;
	const struct wm_adsp_region *mem;
	struct wm_adsp *adsp = ctl->adsp;
	void *scratch;
	int ret;
	unsigned int reg;

	mem = wm_adsp_find_region(adsp, region->type);
	if (!mem) {
		adsp_err(adsp, "No base for region %x\n",
			 region->type);
		return -EINVAL;
	}

	reg = ctl->region.base;
	reg = wm_adsp_region_to_reg(mem, reg);

	scratch = kmalloc(ctl->len, GFP_KERNEL | GFP_DMA);
	if (!scratch)
		return -ENOMEM;

	ret = regmap_raw_read(wm_coeff->regmap, reg, scratch, ctl->len);
	if (ret) {
		adsp_err(adsp, "Failed to read %zu bytes from %x\n",
			 ctl->len, reg);
		kfree(scratch);
		return ret;
	}

	memcpy(buf, scratch, ctl->len);
	kfree(scratch);

	return 0;
}

static int wm_coeff_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct wm_coeff_ctl *ctl = (struct wm_coeff_ctl *)kcontrol->private_value;
	char *p = ucontrol->value.bytes.data;

	memcpy(p, ctl->cache, ctl->len);
	return 0;
}

static int wm_coeff_add_kcontrol(struct wm_coeff *wm_coeff,
				 struct wm_coeff_ctl *ctl,
				 const struct snd_kcontrol_new *kctl)
{
	int ret;
	struct snd_kcontrol *kcontrol;

	kcontrol = snd_ctl_new1(kctl, wm_coeff);
	ret = snd_ctl_add(ctl->card, kcontrol);
	if (ret < 0) {
		dev_err(wm_coeff->dev, "Failed to add %s: %d\n",
			kctl->name, ret);
		return ret;
	}
	ctl->kcontrol = kcontrol;
	return 0;
}

struct wmfw_ctl_work {
	struct wm_coeff *wm_coeff;
	struct wm_coeff_ctl *ctl;
	struct work_struct work;
};

static int wmfw_add_ctl(struct wm_coeff *wm_coeff,
			struct wm_coeff_ctl *ctl)
{
	struct snd_kcontrol_new *kcontrol;
	int ret;

	if (!wm_coeff || !ctl || !ctl->name || !ctl->card)
		return -EINVAL;

	kcontrol = kzalloc(sizeof(*kcontrol), GFP_KERNEL);
	if (!kcontrol)
		return -ENOMEM;
	kcontrol->iface = SNDRV_CTL_ELEM_IFACE_MIXER;

	kcontrol->name = ctl->name;
	kcontrol->info = wm_coeff_info;
	kcontrol->get = wm_coeff_get;
	kcontrol->put = wm_coeff_put;
	kcontrol->private_value = (unsigned long)ctl;

	ret = wm_coeff_add_kcontrol(wm_coeff,
				    ctl, kcontrol);
	if (ret < 0)
		goto err_kcontrol;

	kfree(kcontrol);

	list_add(&ctl->list, &wm_coeff->ctl_list);
	return 0;

err_kcontrol:
	kfree(kcontrol);
	return ret;
}

static int wm_adsp_load(struct wm_adsp *dsp)
{
	LIST_HEAD(buf_list);
	const struct firmware *firmware;
	struct regmap *regmap = dsp->regmap;
	unsigned int pos = 0;
	const struct wmfw_header *header;
	const struct wmfw_adsp1_sizes *adsp1_sizes;
	const struct wmfw_adsp2_sizes *adsp2_sizes;
	const struct wmfw_footer *footer;
	const struct wmfw_region *region;
	const struct wm_adsp_region *mem;
	const char *region_name;
	char *file, *text;
	struct wm_adsp_buf *buf;
	unsigned int reg;
	int regions = 0;
	int ret, offset, type, sizes;

	file = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (file == NULL)
		return -ENOMEM;

	snprintf(file, PAGE_SIZE, "%s-dsp%d-%s.wmfw", dsp->part, dsp->num,
		 wm_adsp_fw[dsp->fw].file);
	file[PAGE_SIZE - 1] = '\0';

	ret = request_firmware(&firmware, file, dsp->dev);
	if (ret != 0) {
		adsp_err(dsp, "Failed to request '%s'\n", file);
		goto out;
	}
	ret = -EINVAL;

	pos = sizeof(*header) + sizeof(*adsp1_sizes) + sizeof(*footer);
	if (pos >= firmware->size) {
		adsp_err(dsp, "%s: file too short, %zu bytes\n",
			 file, firmware->size);
		goto out_fw;
	}

	header = (void*)&firmware->data[0];

	if (memcmp(&header->magic[0], "WMFW", 4) != 0) {
		adsp_err(dsp, "%s: invalid magic\n", file);
		goto out_fw;
	}

	if (header->ver != 0) {
		adsp_err(dsp, "%s: unknown file format %d\n",
			 file, header->ver);
		goto out_fw;
	}

	if (header->core != dsp->type) {
		adsp_err(dsp, "%s: invalid core %d != %d\n",
			 file, header->core, dsp->type);
		goto out_fw;
	}

	switch (dsp->type) {
	case WMFW_ADSP1:
		pos = sizeof(*header) + sizeof(*adsp1_sizes) + sizeof(*footer);
		adsp1_sizes = (void *)&(header[1]);
		footer = (void *)&(adsp1_sizes[1]);
		sizes = sizeof(*adsp1_sizes);

		adsp_dbg(dsp, "%s: %d DM, %d PM, %d ZM\n",
			 file, le32_to_cpu(adsp1_sizes->dm),
			 le32_to_cpu(adsp1_sizes->pm),
			 le32_to_cpu(adsp1_sizes->zm));
		break;

	case WMFW_ADSP2:
		pos = sizeof(*header) + sizeof(*adsp2_sizes) + sizeof(*footer);
		adsp2_sizes = (void *)&(header[1]);
		footer = (void *)&(adsp2_sizes[1]);
		sizes = sizeof(*adsp2_sizes);

		adsp_dbg(dsp, "%s: %d XM, %d YM %d PM, %d ZM\n",
			 file, le32_to_cpu(adsp2_sizes->xm),
			 le32_to_cpu(adsp2_sizes->ym),
			 le32_to_cpu(adsp2_sizes->pm),
			 le32_to_cpu(adsp2_sizes->zm));
		break;

	default:
		BUG_ON(NULL == "Unknown DSP type");
		goto out_fw;
	}

	if (le32_to_cpu(header->len) != sizeof(*header) +
	    sizes + sizeof(*footer)) {
		adsp_err(dsp, "%s: unexpected header length %d\n",
			 file, le32_to_cpu(header->len));
		goto out_fw;
	}

	adsp_dbg(dsp, "%s: timestamp %llu\n", file,
		 le64_to_cpu(footer->timestamp));

	while (pos < firmware->size &&
	       pos - firmware->size > sizeof(*region)) {
		region = (void *)&(firmware->data[pos]);
		region_name = "Unknown";
		reg = 0;
		text = NULL;
		offset = le32_to_cpu(region->offset) & 0xffffff;
		type = be32_to_cpu(region->type) & 0xff;
		mem = wm_adsp_find_region(dsp, type);
		
		switch (type) {
		case WMFW_NAME_TEXT:
			region_name = "Firmware name";
			text = kzalloc(le32_to_cpu(region->len) + 1,
				       GFP_KERNEL);
			break;
		case WMFW_INFO_TEXT:
			region_name = "Information";
			text = kzalloc(le32_to_cpu(region->len) + 1,
				       GFP_KERNEL);
			break;
		case WMFW_ABSOLUTE:
			region_name = "Absolute";
			reg = offset;
			break;
		case WMFW_ADSP1_PM:
			BUG_ON(!mem);
			region_name = "PM";
			reg = wm_adsp_region_to_reg(mem, offset);
			break;
		case WMFW_ADSP1_DM:
			BUG_ON(!mem);
			region_name = "DM";
			reg = wm_adsp_region_to_reg(mem, offset);
			break;
		case WMFW_ADSP2_XM:
			BUG_ON(!mem);
			region_name = "XM";
			reg = wm_adsp_region_to_reg(mem, offset);
			break;
		case WMFW_ADSP2_YM:
			BUG_ON(!mem);
			region_name = "YM";
			reg = wm_adsp_region_to_reg(mem, offset);
			break;
		case WMFW_ADSP1_ZM:
			BUG_ON(!mem);
			region_name = "ZM";
			reg = wm_adsp_region_to_reg(mem, offset);
			break;
		default:
			adsp_warn(dsp,
				  "%s.%d: Unknown region type %x at %d(%x)\n",
				  file, regions, type, pos, pos);
			break;
		}

		adsp_dbg(dsp, "%s.%d: %d bytes at %d in %s\n", file,
			 regions, le32_to_cpu(region->len), offset,
			 region_name);

		if (text) {
			memcpy(text, region->data, le32_to_cpu(region->len));
			adsp_info(dsp, "%s: %s\n", file, text);
			kfree(text);
		}

		if (reg) {
			buf = wm_adsp_buf_alloc(region->data,
						le32_to_cpu(region->len),
						&buf_list);
			if (!buf) {
				adsp_err(dsp, "Out of memory\n");
				return -ENOMEM;
			}

			ret = regmap_raw_write_async(regmap, reg, buf->buf,
						     le32_to_cpu(region->len));
			if (ret != 0) {
				adsp_err(dsp,
					"%s.%d: Failed to write %d bytes at %d in %s: %d\n",
					file, regions,
					le32_to_cpu(region->len), offset,
					region_name, ret);
				goto out_fw;
			}
		}

		pos += le32_to_cpu(region->len) + sizeof(*region);
		regions++;
	}

	ret = regmap_async_complete(regmap);
	if (ret != 0) {
		adsp_err(dsp, "Failed to complete async write: %d\n", ret);
		goto out_fw;
	}

	if (pos > firmware->size)
		adsp_warn(dsp, "%s.%d: %zu bytes at end of file\n",
			  file, regions, pos - firmware->size);

out_fw:
	regmap_async_complete(regmap);
	wm_adsp_buf_free(&buf_list);
	release_firmware(firmware);
out:
	kfree(file);

	return ret;
}

static int wm_coeff_init_control_caches(struct wm_coeff *wm_coeff)
{
	struct wm_coeff_ctl *ctl;
	int ret;

	list_for_each_entry(ctl, &wm_coeff->ctl_list,
			    list) {
		if (!ctl->enabled || ctl->set)
			continue;
		ret = wm_coeff_read_control(ctl->kcontrol,
					    ctl->cache,
					    ctl->len);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int wm_coeff_sync_controls(struct wm_coeff *wm_coeff)
{
	struct wm_coeff_ctl *ctl;
	int ret;

	list_for_each_entry(ctl, &wm_coeff->ctl_list,
			    list) {
		if (!ctl->enabled)
			continue;
		if (ctl->set) {
			ret = wm_coeff_write_control(ctl->kcontrol,
						     ctl->cache,
						     ctl->len);
			if (ret < 0)
				return ret;
		}
	}

	return 0;
}

static void wm_adsp_ctl_work(struct work_struct *work)
{
	struct wmfw_ctl_work *ctl_work = container_of(work,
						      struct wmfw_ctl_work,
						      work);

	wmfw_add_ctl(ctl_work->wm_coeff, ctl_work->ctl);
	kfree(ctl_work);
}

static int wm_adsp_create_control(struct snd_soc_codec *codec,
				  const struct wm_adsp_alg_region *region)

{
	struct wm_adsp *dsp = snd_soc_codec_get_drvdata(codec);
	struct wm_coeff_ctl *ctl;
	struct wmfw_ctl_work *ctl_work;
	char *name;
	char *region_name;
	int ret;

	name = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	switch (region->type) {
	case WMFW_ADSP1_PM:
		region_name = "PM";
		break;
	case WMFW_ADSP1_DM:
		region_name = "DM";
		break;
	case WMFW_ADSP2_XM:
		region_name = "XM";
		break;
	case WMFW_ADSP2_YM:
		region_name = "YM";
		break;
	case WMFW_ADSP1_ZM:
		region_name = "ZM";
		break;
	default:
		return -EINVAL;
	}

	snprintf(name, PAGE_SIZE, "DSP%d %s %x",
		 dsp->num, region_name, region->alg);

	list_for_each_entry(ctl, &dsp->wm_coeff->ctl_list,
			    list) {
		if (!strcmp(ctl->name, name)) {
			if (!ctl->enabled)
				ctl->enabled = 1;
			return 0;
		}
	}

	ctl = kzalloc(sizeof(*ctl), GFP_KERNEL);
	if (!ctl) {
		ret = -ENOMEM;
		goto err_name;
	}
	ctl->region = *region;
	ctl->name = kmemdup(name, strlen(name) + 1, GFP_KERNEL);
	if (!ctl->name) {
		ret = -ENOMEM;
		goto err_ctl;
	}
	ctl->enabled = 1;
	ctl->set = 0;
	ctl->ops.xget = wm_coeff_get;
	ctl->ops.xput = wm_coeff_put;
	ctl->card = codec->card->snd_card;
	ctl->adsp = dsp;

	ctl->len = region->len;
	ctl->cache = kzalloc(ctl->len, GFP_KERNEL);
	if (!ctl->cache) {
		ret = -ENOMEM;
		goto err_ctl_name;
	}

	ctl_work = kzalloc(sizeof(*ctl_work), GFP_KERNEL);
	if (!ctl_work) {
		ret = -ENOMEM;
		goto err_ctl_cache;
	}

	ctl_work->wm_coeff = dsp->wm_coeff;
	ctl_work->ctl = ctl;
	INIT_WORK(&ctl_work->work, wm_adsp_ctl_work);
	schedule_work(&ctl_work->work);

	kfree(name);

	return 0;

err_ctl_cache:
	kfree(ctl->cache);
err_ctl_name:
	kfree(ctl->name);
err_ctl:
	kfree(ctl);
err_name:
	kfree(name);
	return ret;
}

static int wm_adsp_setup_algs(struct wm_adsp *dsp, struct snd_soc_codec *codec)
{
	struct regmap *regmap = dsp->regmap;
	struct wmfw_adsp1_id_hdr adsp1_id;
	struct wmfw_adsp2_id_hdr adsp2_id;
	struct wmfw_adsp1_alg_hdr *adsp1_alg;
	struct wmfw_adsp2_alg_hdr *adsp2_alg;
	void *alg, *buf;
	struct wm_adsp_alg_region *region;
	const struct wm_adsp_region *mem;
	unsigned int pos, term;
	size_t algs, buf_size;
	__be32 val;
	int i, ret;

	switch (dsp->type) {
	case WMFW_ADSP1:
		mem = wm_adsp_find_region(dsp, WMFW_ADSP1_DM);
		break;
	case WMFW_ADSP2:
		mem = wm_adsp_find_region(dsp, WMFW_ADSP2_XM);
		break;
	default:
		mem = NULL;
		break;
	}

	if (mem == NULL) {
		BUG_ON(mem != NULL);
		return -EINVAL;
	}

	switch (dsp->type) {
	case WMFW_ADSP1:
		ret = regmap_raw_read(regmap, mem->base, &adsp1_id,
				      sizeof(adsp1_id));
		if (ret != 0) {
			adsp_err(dsp, "Failed to read algorithm info: %d\n",
				 ret);
			return ret;
		}

		buf = &adsp1_id;
		buf_size = sizeof(adsp1_id);

		algs = be32_to_cpu(adsp1_id.algs);
		dsp->fw_id = be32_to_cpu(adsp1_id.fw.id);
		adsp_info(dsp, "Firmware: %x v%d.%d.%d, %zu algorithms\n",
			  dsp->fw_id,
			  (be32_to_cpu(adsp1_id.fw.ver) & 0xff0000) >> 16,
			  (be32_to_cpu(adsp1_id.fw.ver) & 0xff00) >> 8,
			  be32_to_cpu(adsp1_id.fw.ver) & 0xff,
			  algs);

		region = kzalloc(sizeof(*region), GFP_KERNEL);
		if (!region)
			return -ENOMEM;
		region->type = WMFW_ADSP1_ZM;
		region->alg = be32_to_cpu(adsp1_id.fw.id);
		region->base = be32_to_cpu(adsp1_id.zm);
		list_add_tail(&region->list, &dsp->alg_regions);

		region = kzalloc(sizeof(*region), GFP_KERNEL);
		if (!region)
			return -ENOMEM;
		region->type = WMFW_ADSP1_DM;
		region->alg = be32_to_cpu(adsp1_id.fw.id);
		region->base = be32_to_cpu(adsp1_id.dm);
		list_add_tail(&region->list, &dsp->alg_regions);

		pos = sizeof(adsp1_id) / 2;
		term = pos + ((sizeof(*adsp1_alg) * algs) / 2);
		break;

	case WMFW_ADSP2:
		ret = regmap_raw_read(regmap, mem->base, &adsp2_id,
				      sizeof(adsp2_id));
		if (ret != 0) {
			adsp_err(dsp, "Failed to read algorithm info: %d\n",
				 ret);
			return ret;
		}

		buf = &adsp2_id;
		buf_size = sizeof(adsp2_id);

		algs = be32_to_cpu(adsp2_id.algs);
		dsp->fw_id = be32_to_cpu(adsp2_id.fw.id);
		adsp_info(dsp, "Firmware: %x v%d.%d.%d, %zu algorithms\n",
			  dsp->fw_id,
			  (be32_to_cpu(adsp2_id.fw.ver) & 0xff0000) >> 16,
			  (be32_to_cpu(adsp2_id.fw.ver) & 0xff00) >> 8,
			  be32_to_cpu(adsp2_id.fw.ver) & 0xff,
			  algs);

		region = kzalloc(sizeof(*region), GFP_KERNEL);
		if (!region)
			return -ENOMEM;
		region->type = WMFW_ADSP2_XM;
		region->alg = be32_to_cpu(adsp2_id.fw.id);
		region->base = be32_to_cpu(adsp2_id.xm);
		list_add_tail(&region->list, &dsp->alg_regions);

		region = kzalloc(sizeof(*region), GFP_KERNEL);
		if (!region)
			return -ENOMEM;
		region->type = WMFW_ADSP2_YM;
		region->alg = be32_to_cpu(adsp2_id.fw.id);
		region->base = be32_to_cpu(adsp2_id.ym);
		list_add_tail(&region->list, &dsp->alg_regions);

		region = kzalloc(sizeof(*region), GFP_KERNEL);
		if (!region)
			return -ENOMEM;
		region->type = WMFW_ADSP2_ZM;
		region->alg = be32_to_cpu(adsp2_id.fw.id);
		region->base = be32_to_cpu(adsp2_id.zm);
		list_add_tail(&region->list, &dsp->alg_regions);

		pos = sizeof(adsp2_id) / 2;
		term = pos + ((sizeof(*adsp2_alg) * algs) / 2);
		break;

	default:
		BUG_ON(NULL == "Unknown DSP type");
		return -EINVAL;
	}

	if (algs == 0) {
		adsp_err(dsp, "No algorithms\n");
		return -EINVAL;
	}

	if (algs > 1024) {
		adsp_err(dsp, "Algorithm count %zx excessive\n", algs);
		print_hex_dump_bytes(dev_name(dsp->dev), DUMP_PREFIX_OFFSET,
				     buf, buf_size);
		return -EINVAL;
	}

	/* Read the terminator first to validate the length */
	ret = regmap_raw_read(regmap, mem->base + term, &val, sizeof(val));
	if (ret != 0) {
		adsp_err(dsp, "Failed to read algorithm list end: %d\n",
			ret);
		return ret;
	}

	if (be32_to_cpu(val) != 0xbedead)
		adsp_warn(dsp, "Algorithm list end %x 0x%x != 0xbeadead\n",
			  term, be32_to_cpu(val));

	alg = kzalloc((term - pos) * 2, GFP_KERNEL | GFP_DMA);
	if (!alg)
		return -ENOMEM;

	ret = regmap_raw_read(regmap, mem->base + pos, alg, (term - pos) * 2);
	if (ret != 0) {
		adsp_err(dsp, "Failed to read algorithm list: %d\n",
			ret);
		goto out;
	}

	adsp1_alg = alg;
	adsp2_alg = alg;

	for (i = 0; i < algs; i++) {
		switch (dsp->type) {
		case WMFW_ADSP1:
			adsp_info(dsp, "%d: ID %x v%d.%d.%d DM@%x ZM@%x\n",
				  i, be32_to_cpu(adsp1_alg[i].alg.id),
				  (be32_to_cpu(adsp1_alg[i].alg.ver) & 0xff0000) >> 16,
				  (be32_to_cpu(adsp1_alg[i].alg.ver) & 0xff00) >> 8,
				  be32_to_cpu(adsp1_alg[i].alg.ver) & 0xff,
				  be32_to_cpu(adsp1_alg[i].dm),
				  be32_to_cpu(adsp1_alg[i].zm));

			region = kzalloc(sizeof(*region), GFP_KERNEL);
			if (!region)
				return -ENOMEM;
			region->type = WMFW_ADSP1_DM;
			region->alg = be32_to_cpu(adsp1_alg[i].alg.id);
			region->base = be32_to_cpu(adsp1_alg[i].dm);
			region->len = 0;
			list_add_tail(&region->list, &dsp->alg_regions);
			if (i + 1 < algs) {
				region->len = be32_to_cpu(adsp1_alg[i + 1].dm);
				region->len -= be32_to_cpu(adsp1_alg[i].dm);
				wm_adsp_create_control(codec, region);
			} else {
				adsp_warn(dsp, "Missing length info for region DM with ID %x\n",
					  be32_to_cpu(adsp1_alg[i].alg.id));
			}

			region = kzalloc(sizeof(*region), GFP_KERNEL);
			if (!region)
				return -ENOMEM;
			region->type = WMFW_ADSP1_ZM;
			region->alg = be32_to_cpu(adsp1_alg[i].alg.id);
			region->base = be32_to_cpu(adsp1_alg[i].zm);
			region->len = 0;
			list_add_tail(&region->list, &dsp->alg_regions);
			if (i + 1 < algs) {
				region->len = be32_to_cpu(adsp1_alg[i + 1].zm);
				region->len -= be32_to_cpu(adsp1_alg[i].zm);
				wm_adsp_create_control(codec, region);
			} else {
				adsp_warn(dsp, "Missing length info for region ZM with ID %x\n",
					  be32_to_cpu(adsp1_alg[i].alg.id));
			}
			break;

		case WMFW_ADSP2:
			adsp_info(dsp,
				  "%d: ID %x v%d.%d.%d XM@%x YM@%x ZM@%x\n",
				  i, be32_to_cpu(adsp2_alg[i].alg.id),
				  (be32_to_cpu(adsp2_alg[i].alg.ver) & 0xff0000) >> 16,
				  (be32_to_cpu(adsp2_alg[i].alg.ver) & 0xff00) >> 8,
				  be32_to_cpu(adsp2_alg[i].alg.ver) & 0xff,
				  be32_to_cpu(adsp2_alg[i].xm),
				  be32_to_cpu(adsp2_alg[i].ym),
				  be32_to_cpu(adsp2_alg[i].zm));

			region = kzalloc(sizeof(*region), GFP_KERNEL);
			if (!region)
				return -ENOMEM;
			region->type = WMFW_ADSP2_XM;
			region->alg = be32_to_cpu(adsp2_alg[i].alg.id);
			region->base = be32_to_cpu(adsp2_alg[i].xm);
			region->len = 0;
			list_add_tail(&region->list, &dsp->alg_regions);
			if (i + 1 < algs) {
				region->len = be32_to_cpu(adsp2_alg[i + 1].xm);
				region->len -= be32_to_cpu(adsp2_alg[i].xm);
				wm_adsp_create_control(codec, region);
			} else {
				adsp_warn(dsp, "Missing length info for region XM with ID %x\n",
					  be32_to_cpu(adsp2_alg[i].alg.id));
			}

			region = kzalloc(sizeof(*region), GFP_KERNEL);
			if (!region)
				return -ENOMEM;
			region->type = WMFW_ADSP2_YM;
			region->alg = be32_to_cpu(adsp2_alg[i].alg.id);
			region->base = be32_to_cpu(adsp2_alg[i].ym);
			region->len = 0;
			list_add_tail(&region->list, &dsp->alg_regions);
			if (i + 1 < algs) {
				region->len = be32_to_cpu(adsp2_alg[i + 1].ym);
				region->len -= be32_to_cpu(adsp2_alg[i].ym);
				wm_adsp_create_control(codec, region);
			} else {
				adsp_warn(dsp, "Missing length info for region YM with ID %x\n",
					  be32_to_cpu(adsp2_alg[i].alg.id));
			}

			region = kzalloc(sizeof(*region), GFP_KERNEL);
			if (!region)
				return -ENOMEM;
			region->type = WMFW_ADSP2_ZM;
			region->alg = be32_to_cpu(adsp2_alg[i].alg.id);
			region->base = be32_to_cpu(adsp2_alg[i].zm);
			region->len = 0;
			list_add_tail(&region->list, &dsp->alg_regions);
			if (i + 1 < algs) {
				region->len = be32_to_cpu(adsp2_alg[i + 1].zm);
				region->len -= be32_to_cpu(adsp2_alg[i].zm);
				wm_adsp_create_control(codec, region);
			} else {
				adsp_warn(dsp, "Missing length info for region ZM with ID %x\n",
					  be32_to_cpu(adsp2_alg[i].alg.id));
			}
			break;
		}
	}

out:
	kfree(alg);
	return ret;
}

static int wm_adsp_load_coeff(struct wm_adsp *dsp)
{
	LIST_HEAD(buf_list);
	struct regmap *regmap = dsp->regmap;
	struct wmfw_coeff_hdr *hdr;
	struct wmfw_coeff_item *blk;
	const struct firmware *firmware;
	const struct wm_adsp_region *mem;
	struct wm_adsp_alg_region *alg_region;
	const char *region_name;
	int ret, pos, blocks, type, offset, reg;
	char *file;
	struct wm_adsp_buf *buf;
	int tmp;

	file = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (file == NULL)
		return -ENOMEM;

	snprintf(file, PAGE_SIZE, "%s-dsp%d-%s.bin", dsp->part, dsp->num,
		 wm_adsp_fw[dsp->fw].file);
	file[PAGE_SIZE - 1] = '\0';

	ret = request_firmware(&firmware, file, dsp->dev);
	if (ret != 0) {
		adsp_warn(dsp, "Failed to request '%s'\n", file);
		ret = 0;
		goto out;
	}
	ret = -EINVAL;

	if (sizeof(*hdr) >= firmware->size) {
		adsp_err(dsp, "%s: file too short, %zu bytes\n",
			file, firmware->size);
		goto out_fw;
	}

	hdr = (void*)&firmware->data[0];
	if (memcmp(hdr->magic, "WMDR", 4) != 0) {
		adsp_err(dsp, "%s: invalid magic\n", file);
		goto out_fw;
	}

	switch (be32_to_cpu(hdr->rev) & 0xff) {
	case 1:
		break;
	default:
		adsp_err(dsp, "%s: Unsupported coefficient file format %d\n",
			 file, be32_to_cpu(hdr->rev) & 0xff);
		ret = -EINVAL;
		goto out_fw;
	}

	adsp_dbg(dsp, "%s: v%d.%d.%d\n", file,
		(le32_to_cpu(hdr->ver) >> 16) & 0xff,
		(le32_to_cpu(hdr->ver) >>  8) & 0xff,
		le32_to_cpu(hdr->ver) & 0xff);

	pos = le32_to_cpu(hdr->len);

	blocks = 0;
	while (pos < firmware->size &&
	       pos - firmware->size > sizeof(*blk)) {
		blk = (void*)(&firmware->data[pos]);

		type = le16_to_cpu(blk->type);
		offset = le16_to_cpu(blk->offset);

		adsp_dbg(dsp, "%s.%d: %x v%d.%d.%d\n",
			 file, blocks, le32_to_cpu(blk->id),
			 (le32_to_cpu(blk->ver) >> 16) & 0xff,
			 (le32_to_cpu(blk->ver) >>  8) & 0xff,
			 le32_to_cpu(blk->ver) & 0xff);
		adsp_dbg(dsp, "%s.%d: %d bytes at 0x%x in %x\n",
			 file, blocks, le32_to_cpu(blk->len), offset, type);

		reg = 0;
		region_name = "Unknown";
		switch (type) {
		case (WMFW_NAME_TEXT << 8):
		case (WMFW_INFO_TEXT << 8):
			break;
		case (WMFW_ABSOLUTE << 8):
			/*
			 * Old files may use this for global
			 * coefficients.
			 */
			if (le32_to_cpu(blk->id) == dsp->fw_id &&
			    offset == 0) {
				region_name = "global coefficients";
				mem = wm_adsp_find_region(dsp, type);
				if (!mem) {
					adsp_err(dsp, "No ZM\n");
					break;
				}
				reg = wm_adsp_region_to_reg(mem, 0);

			} else {
				region_name = "register";
				reg = offset;
			}
			break;

		case WMFW_ADSP1_DM:
		case WMFW_ADSP1_ZM:
		case WMFW_ADSP2_XM:
		case WMFW_ADSP2_YM:
			adsp_dbg(dsp, "%s.%d: %d bytes in %x for %x\n",
				 file, blocks, le32_to_cpu(blk->len),
				 type, le32_to_cpu(blk->id));

			mem = wm_adsp_find_region(dsp, type);
			if (!mem) {
				adsp_err(dsp, "No base for region %x\n", type);
				break;
			}

			reg = 0;
			list_for_each_entry(alg_region,
					    &dsp->alg_regions, list) {
				if (le32_to_cpu(blk->id) == alg_region->alg &&
				    type == alg_region->type) {
					reg = alg_region->base;
					reg = wm_adsp_region_to_reg(mem,
								    reg);
					reg += offset;
				}
			}

			if (reg == 0)
				adsp_err(dsp, "No %x for algorithm %x\n",
					 type, le32_to_cpu(blk->id));
			break;

		default:
			adsp_err(dsp, "%s.%d: Unknown region type %x at %d\n",
				 file, blocks, type, pos);
			break;
		}

		if (reg) {
			buf = wm_adsp_buf_alloc(blk->data,
						le32_to_cpu(blk->len),
						&buf_list);
			if (!buf) {
				adsp_err(dsp, "Out of memory\n");
				ret = -ENOMEM;
				goto out_fw;
			}

			adsp_dbg(dsp, "%s.%d: Writing %d bytes at %x\n",
				 file, blocks, le32_to_cpu(blk->len),
				 reg);
			ret = regmap_raw_write_async(regmap, reg, buf->buf,
						     le32_to_cpu(blk->len));
			if (ret != 0) {
				adsp_err(dsp,
					"%s.%d: Failed to write to %x in %s\n",
					file, blocks, reg, region_name);
			}
		}

		tmp = le32_to_cpu(blk->len) % 4;
		if (tmp)
			pos += le32_to_cpu(blk->len) + (4 - tmp) + sizeof(*blk);
		else
			pos += le32_to_cpu(blk->len) + sizeof(*blk);

		blocks++;
	}

	ret = regmap_async_complete(regmap);
	if (ret != 0)
		adsp_err(dsp, "Failed to complete async write: %d\n", ret);

	if (pos > firmware->size)
		adsp_warn(dsp, "%s.%d: %zu bytes at end of file\n",
			  file, blocks, pos - firmware->size);

out_fw:
	regmap_async_complete(regmap);
	release_firmware(firmware);
	wm_adsp_buf_free(&buf_list);
out:
	kfree(file);
	return ret;
}

int wm_adsp1_init(struct wm_adsp *adsp)
{
	INIT_LIST_HEAD(&adsp->alg_regions);

	return 0;
}
EXPORT_SYMBOL_GPL(wm_adsp1_init);

int wm_adsp1_event(struct snd_soc_dapm_widget *w,
		   struct snd_kcontrol *kcontrol,
		   int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct wm_adsp *dsps = snd_soc_codec_get_drvdata(codec);
	struct wm_adsp *dsp = &dsps[w->shift];
	struct wm_coeff_ctl *ctl;
	int ret;
	int val;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_30,
				   ADSP1_SYS_ENA, ADSP1_SYS_ENA);

		/*
		 * For simplicity set the DSP clock rate to be the
		 * SYSCLK rate rather than making it configurable.
		 */
		if(dsp->sysclk_reg) {
			ret = regmap_read(dsp->regmap, dsp->sysclk_reg, &val);
			if (ret != 0) {
				adsp_err(dsp, "Failed to read SYSCLK state: %d\n",
				ret);
				return ret;
			}

			val = (val & dsp->sysclk_mask)
				>> dsp->sysclk_shift;

			ret = regmap_update_bits(dsp->regmap,
						 dsp->base + ADSP1_CONTROL_31,
						 ADSP1_CLK_SEL_MASK, val);
			if (ret != 0) {
				adsp_err(dsp, "Failed to set clock rate: %d\n",
					 ret);
				return ret;
			}
		}

		ret = wm_adsp_load(dsp);
		if (ret != 0)
			goto err;

		ret = wm_adsp_setup_algs(dsp, codec);
		if (ret != 0)
			goto err;

		ret = wm_adsp_load_coeff(dsp);
		if (ret != 0)
			goto err;

		/* Initialize caches for enabled and unset controls */
		ret = wm_coeff_init_control_caches(dsp->wm_coeff);
		if (ret != 0)
			goto err;

		/* Sync set controls */
		ret = wm_coeff_sync_controls(dsp->wm_coeff);
		if (ret != 0)
			goto err;

		/* Start the core running */
		regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_30,
				   ADSP1_CORE_ENA | ADSP1_START,
				   ADSP1_CORE_ENA | ADSP1_START);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		/* Halt the core */
		regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_30,
				   ADSP1_CORE_ENA | ADSP1_START, 0);

		regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_19,
				   ADSP1_WDMA_BUFFER_LENGTH_MASK, 0);

		regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_30,
				   ADSP1_SYS_ENA, 0);

		list_for_each_entry(ctl, &dsp->wm_coeff->ctl_list,
				    list) {
			ctl->enabled = 0;
		}
		break;

	default:
		break;
	}

	return 0;

err:
	regmap_update_bits(dsp->regmap, dsp->base + ADSP1_CONTROL_30,
			   ADSP1_SYS_ENA, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(wm_adsp1_event);

static int wm_adsp2_ena(struct wm_adsp *dsp)
{
	unsigned int val;
	int ret, count;

	ret = regmap_update_bits(dsp->regmap, dsp->base + ADSP2_CONTROL,
				 ADSP2_SYS_ENA, ADSP2_SYS_ENA);
	if (ret != 0)
		return ret;

	/* Wait for the RAM to start, should be near instantaneous */
	for (count = 0; count < 10; ++count) {
		ret = regmap_read(dsp->regmap, dsp->base + ADSP2_STATUS1,
				  &val);
		if (ret != 0)
			return ret;

		if (val & ADSP2_RAM_RDY)
			break;

		msleep(1);
	}

	if (!(val & ADSP2_RAM_RDY)) {
		adsp_err(dsp, "Failed to start DSP RAM\n");
		return -EBUSY;
	}

	adsp_dbg(dsp, "RAM ready after %d polls\n", count);
	adsp_info(dsp, "RAM ready after %d polls\n", count);

	return 0;
}

int wm_adsp2_event(struct snd_soc_dapm_widget *w,
		   struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	struct wm_adsp *dsps = snd_soc_codec_get_drvdata(codec);
	struct wm_adsp *dsp = &dsps[w->shift];
	struct wm_adsp_alg_region *alg_region;
	struct wm_coeff_ctl *ctl;
	unsigned int val;
	int ret;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		/*
		 * For simplicity set the DSP clock rate to be the
		 * SYSCLK rate rather than making it configurable.
		 */
		ret = regmap_read(dsp->regmap, ARIZONA_SYSTEM_CLOCK_1, &val);
		if (ret != 0) {
			adsp_err(dsp, "Failed to read SYSCLK state: %d\n",
				 ret);
			return ret;
		}
		val = (val & ARIZONA_SYSCLK_FREQ_MASK)
			>> ARIZONA_SYSCLK_FREQ_SHIFT;

		ret = regmap_update_bits(dsp->regmap,
					 dsp->base + ADSP2_CLOCKING,
					 ADSP2_CLK_SEL_MASK, val);
		if (ret != 0) {
			adsp_err(dsp, "Failed to set clock rate: %d\n",
				 ret);
			return ret;
		}

		if (dsp->dvfs) {
			ret = regmap_read(dsp->regmap,
					  dsp->base + ADSP2_CLOCKING, &val);
			if (ret != 0) {
				dev_err(dsp->dev,
					"Failed to read clocking: %d\n", ret);
				return ret;
			}

			if ((val & ADSP2_CLK_SEL_MASK) >= 3) {
				ret = regulator_enable(dsp->dvfs);
				if (ret != 0) {
					dev_err(dsp->dev,
						"Failed to enable supply: %d\n",
						ret);
					return ret;
				}

				ret = regulator_set_voltage(dsp->dvfs,
							    1800000,
							    1800000);
				if (ret != 0) {
					dev_err(dsp->dev,
						"Failed to raise supply: %d\n",
						ret);
					return ret;
				}
			}
		}

		ret = wm_adsp2_ena(dsp);
		if (ret != 0)
			return ret;

		ret = wm_adsp_load(dsp);
		if (ret != 0)
			goto err;

		ret = wm_adsp_setup_algs(dsp, codec);
		if (ret != 0)
			goto err;

		ret = wm_adsp_load_coeff(dsp);
		if (ret != 0)
			goto err;

		/* Initialize caches for enabled and unset controls */
		ret = wm_coeff_init_control_caches(dsp->wm_coeff);
		if (ret != 0)
			goto err;

		/* Sync set controls */
		ret = wm_coeff_sync_controls(dsp->wm_coeff);
		if (ret != 0)
			goto err;

		ret = regmap_update_bits(dsp->regmap,
					 dsp->base + ADSP2_CONTROL,
					 ADSP2_CORE_ENA | ADSP2_START,
					 ADSP2_CORE_ENA | ADSP2_START);
		if (ret != 0)
			goto err;

		dsp->running = true;
		break;

	case SND_SOC_DAPM_PRE_PMD:
		dsp->running = false;

		regmap_update_bits(dsp->regmap, dsp->base + ADSP2_CONTROL,
				   ADSP2_SYS_ENA | ADSP2_CORE_ENA |
				   ADSP2_START, 0);

		/* Make sure DMAs are quiesced */
		regmap_write(dsp->regmap, dsp->base + ADSP2_WDMA_CONFIG_1, 0);
		regmap_write(dsp->regmap, dsp->base + ADSP2_WDMA_CONFIG_2, 0);
		regmap_write(dsp->regmap, dsp->base + ADSP2_RDMA_CONFIG_1, 0);

		if (dsp->dvfs) {
			ret = regulator_set_voltage(dsp->dvfs, 1200000,
						    1800000);
			if (ret != 0)
				dev_warn(dsp->dev,
					 "Failed to lower supply: %d\n",
					 ret);

			ret = regulator_disable(dsp->dvfs);
			if (ret != 0)
				dev_err(dsp->dev,
					"Failed to enable supply: %d\n",
					ret);
		}

		list_for_each_entry(ctl, &dsp->wm_coeff->ctl_list,
				    list) {
			ctl->enabled = 0;
		}

		while (!list_empty(&dsp->alg_regions)) {
			alg_region = list_first_entry(&dsp->alg_regions,
						      struct wm_adsp_alg_region,
						      list);
			list_del(&alg_region->list);
			kfree(alg_region);
		}
		break;

	default:
		break;
	}

	return 0;
err:
	regmap_update_bits(dsp->regmap, dsp->base + ADSP2_CONTROL,
			   ADSP2_SYS_ENA | ADSP2_CORE_ENA | ADSP2_START, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(wm_adsp2_event);

int wm_adsp2_init(struct wm_adsp *adsp, bool dvfs)
{
	int ret;

	/*
	 * Disable the DSP memory by default when in reset for a small
	 * power saving.
	 */
	ret = regmap_update_bits(adsp->regmap, adsp->base + ADSP2_CONTROL,
				 ADSP2_MEM_ENA, 0);
	if (ret != 0) {
		adsp_err(adsp, "Failed to clear memory retention: %d\n", ret);
		return ret;
	}

	INIT_LIST_HEAD(&adsp->alg_regions);

	adsp->wm_coeff = kzalloc(sizeof(*adsp->wm_coeff),
				 GFP_KERNEL);
	if (!adsp->wm_coeff)
		return -ENOMEM;
	adsp->wm_coeff->regmap = adsp->regmap;
	adsp->wm_coeff->dev = adsp->dev;
	INIT_LIST_HEAD(&adsp->wm_coeff->ctl_list);

	if (dvfs) {
		adsp->dvfs = devm_regulator_get(adsp->dev, "DCVDD");
		if (IS_ERR(adsp->dvfs)) {
			ret = PTR_ERR(adsp->dvfs);
			dev_err(adsp->dev, "Failed to get DCVDD: %d\n", ret);
			goto out_coeff;
		}

		ret = regulator_enable(adsp->dvfs);
		if (ret != 0) {
			dev_err(adsp->dev, "Failed to enable DCVDD: %d\n",
				ret);
			goto out_coeff;
		}

		ret = regulator_set_voltage(adsp->dvfs, 1200000, 1800000);
		if (ret != 0) {
			dev_err(adsp->dev, "Failed to initialise DVFS: %d\n",
				ret);
			goto out_coeff;
		}

		ret = regulator_disable(adsp->dvfs);
		if (ret != 0) {
			dev_err(adsp->dev, "Failed to disable DCVDD: %d\n",
				ret);
			goto out_coeff;
		}
	}

	return 0;

out_coeff:
	kfree(adsp->wm_coeff);
	return ret;
}
EXPORT_SYMBOL_GPL(wm_adsp2_init);

bool wm_adsp_compress_supported(const struct wm_adsp* adsp,
				const struct snd_compr_stream* stream)
{
	if (adsp->fw >= 0 && adsp->fw < WM_ADSP_NUM_FW) {
		if (wm_adsp_fw[adsp->fw].num_caps == 0)
			return false;
		if (wm_adsp_fw[adsp->fw].compr_direction == stream->direction)
			return true;
	}
	return false;
}
EXPORT_SYMBOL_GPL(wm_adsp_compress_supported);
bool wm_adsp_format_supported(const struct wm_adsp *adsp,
			      const struct snd_compr_stream *stream,
			      const struct snd_compr_params *params)
{
	const struct wm_adsp_fw_caps *caps;
	int i;
	for (i = 0; i < wm_adsp_fw[adsp->fw].num_caps; i++) {
		caps = &wm_adsp_fw[adsp->fw].caps[i];
		if (caps->id != params->codec.id)
			continue;
		if (stream->direction == SND_COMPRESS_PLAYBACK) {
			if (caps->desc.max_ch < params->codec.ch_out)
				continue;
		} else {
			if (caps->desc.max_ch < params->codec.ch_in)
				continue;
		}
		if ((caps->desc.sample_rates & params->codec.sample_rate) &&
		    (caps->desc.formats & (1 << params->codec.format)))
			return true;
	}
	return false;
}
EXPORT_SYMBOL_GPL(wm_adsp_format_supported);
void wm_adsp_get_caps(const struct wm_adsp *adsp,
		      const struct snd_compr_stream *stream,
		      struct snd_compr_caps *caps)
{
	int i;
	if (wm_adsp_fw[adsp->fw].caps) {
		for (i = 0; i < wm_adsp_fw[adsp->fw].num_caps; i++)
			caps->codecs[i] = wm_adsp_fw[adsp->fw].caps[i].id;
		caps->num_codecs = i;
		caps->direction = wm_adsp_fw[adsp->fw].compr_direction;
	}
}
EXPORT_SYMBOL_GPL(wm_adsp_get_caps);

static int wm_adsp_read_data_block(struct wm_adsp* adsp, int mem_type,
				   unsigned int mem_addr,
				   unsigned int num_words,
				   u32* data)
{
	struct wm_adsp_region const *region = wm_adsp_find_region(adsp,
								  mem_type);
	unsigned int i, reg;
	int ret;

	if (!region)
		return -EINVAL;

	reg = wm_adsp_region_to_reg(region, mem_addr);

	ret = regmap_raw_read(adsp->regmap, reg, data,
			      sizeof(*data) * num_words);
	if (ret < 0)
		return ret;

	for (i = 0; i < num_words; ++i)
		data[i] = be32_to_cpu(data[i]) & 0x00ffffffu;

	return 0;
}

static int wm_adsp_read_data_word(struct wm_adsp* adsp, int mem_type,
				  unsigned int mem_addr, u32* data)
{
	return wm_adsp_read_data_block(adsp, mem_type, mem_addr, 1, data);
}

static int wm_adsp_write_data_word(struct wm_adsp* adsp, int mem_type,
				   unsigned int mem_addr, u32 data)
{
	struct wm_adsp_region const *region = wm_adsp_find_region(adsp,
								  mem_type);
	unsigned int reg;

	if (!region)
		return -EINVAL;

	reg = wm_adsp_region_to_reg(region, mem_addr);

	data = cpu_to_be32(data & 0x00ffffffu);

	return regmap_raw_write(adsp->regmap, reg, &data, sizeof(data));
}

static inline unsigned int wm_adsp_words_to_samps(const struct wm_adsp *adsp,
						  unsigned int words)
{
	return (words * WM_ADSP_DATA_WORD_SIZE) / adsp->sample_size;
}

static inline int wm_adsp_host_buffer_read(struct wm_adsp *adsp,
					   unsigned int field_offset, u32* data)
{
	return wm_adsp_read_data_word(adsp, WMFW_ADSP2_XM,
				      adsp->host_buf_ptr + field_offset, data);
}

static int wm_adsp_populate_buffer_regions(struct wm_adsp *adsp)
{
	int i, ret;
	u32 size;
	u32 cumulative_samps = 0;
	struct wm_adsp_buffer_region_def *host_region_defs =
		wm_adsp_fw[adsp->fw].caps->host_region_defs;
	struct wm_adsp_buffer_region *region;

	for (i = 0; i < wm_adsp_fw[adsp->fw].caps->num_host_regions; ++i) {
		region = &adsp->host_regions[i];

		region->offset_samps = cumulative_samps;
		region->mem_type = host_region_defs[i].mem_type;

		ret = wm_adsp_host_buffer_read(adsp,
					       host_region_defs[i].base_offset,
					       &region->base_addr);
		if (ret < 0)
			return ret;

		ret = wm_adsp_host_buffer_read(adsp,
					       host_region_defs[i].size_offset,
					       &size);
		if (ret < 0)
			return ret;

		cumulative_samps += wm_adsp_words_to_samps(adsp, size);

		region->cumulative_samps = cumulative_samps;
	}

	return 0;
}

int wm_adsp_stream_alloc(struct wm_adsp* adsp,
			 const struct snd_compr_params *params)
{
	unsigned int size;

	switch (params->codec.format) {
	case SNDRV_PCM_FORMAT_S16_LE:
		adsp->sample_size = 2;
		break;
	default:
		return -EINVAL;
	}

	if (!adsp->host_regions) {
		size = wm_adsp_fw[adsp->fw].caps->num_host_regions *
		       sizeof(*adsp->host_regions);
		adsp->host_regions = kzalloc(size, GFP_KERNEL);

		if (!adsp->host_regions)
			return -ENOMEM;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(wm_adsp_stream_alloc);

int wm_adsp_stream_free(struct wm_adsp* adsp)
{
	if (adsp->host_regions) {
		kfree(adsp->host_regions);
		adsp->host_regions = NULL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(wm_adsp_stream_free);

int wm_adsp_stream_start(struct wm_adsp *adsp)
{
	u32 xm_base, magic;
	int ret;

	ret = wm_adsp_read_data_word(adsp, WMFW_ADSP2_XM,
				     ADSP2_SYSTEM_CONFIG_XM_PTR, &xm_base);
	if (ret < 0)
		return ret;

	ret = wm_adsp_read_data_word(adsp, WMFW_ADSP2_XM,
				     xm_base + WM_ADSP_ALG_XM_PTR +
				     ALG_XM_FIELD(magic),
				     &magic);
	if (ret < 0)
		return ret;

	if (magic != WM_ADSP_ALG_XM_STRUCT_MAGIC)
		return -EINVAL;

	ret = wm_adsp_read_data_word(adsp, WMFW_ADSP2_XM,
				     xm_base + WM_ADSP_ALG_XM_PTR +
				     ALG_XM_FIELD(host_buf_ptr),
				     &adsp->host_buf_ptr);
	if (ret < 0)
		return ret;

	ret = wm_adsp_host_buffer_read(adsp,
				       HOST_BUFFER_FIELD(low_water_mark),
				       &adsp->low_water_mark);
	if (ret < 0)
		return ret;

	ret = wm_adsp_populate_buffer_regions(adsp);
	if (ret < 0)
		return ret;

	return 0;
}
EXPORT_SYMBOL_GPL(wm_adsp_stream_start);
MODULE_LICENSE("GPL v2");
