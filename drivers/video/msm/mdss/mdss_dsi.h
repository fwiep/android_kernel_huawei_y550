/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MDSS_DSI_H
#define MDSS_DSI_H

#include <linux/list.h>
#include <linux/mdss_io_util.h>
#include <linux/irqreturn.h>
#include <linux/pinctrl/consumer.h>

#include "mdss_panel.h"
#include "mdss_dsi_cmd.h"

#define MMSS_SERDES_BASE_PHY 0x04f01000 /* mmss (De)Serializer CFG */

#define MIPI_OUTP(addr, data) writel_relaxed((data), (addr))
#define MIPI_INP(addr) readl_relaxed(addr)

#define MIPI_OUTP_SECURE(addr, data) writel_relaxed((data), (addr))
#define MIPI_INP_SECURE(addr) readl_relaxed(addr)

#define MIPI_DSI_PRIM 1
#define MIPI_DSI_SECD 2

#define MIPI_DSI_PANEL_VGA	0
#define MIPI_DSI_PANEL_WVGA	1
#define MIPI_DSI_PANEL_WVGA_PT	2
#define MIPI_DSI_PANEL_FWVGA_PT	3
#define MIPI_DSI_PANEL_WSVGA_PT	4
#define MIPI_DSI_PANEL_QHD_PT 5
#define MIPI_DSI_PANEL_WXGA	6
#define MIPI_DSI_PANEL_WUXGA	7
#define MIPI_DSI_PANEL_720P_PT	8
#define DSI_PANEL_MAX	8

#define MDSS_DSI_HW_REV_100_1		0x10000001	/* 8x26    */
#define MDSS_DSI_HW_REV_100_2		0x10000002	/* 8x26v2  */
#define MDSS_DSI_HW_REV_103_1		0x10030001	/* 8916/8936 */

enum {		/* mipi dsi panel */
	DSI_VIDEO_MODE,
	DSI_CMD_MODE,
};

enum {
	ST_DSI_CLK_OFF,
	ST_DSI_SUSPEND,
	ST_DSI_RESUME,
	ST_DSI_PLAYING,
	ST_DSI_NUM
};

enum {
	EV_DSI_UPDATE,
	EV_DSI_DONE,
	EV_DSI_TOUT,
	EV_DSI_NUM
};

enum {
	LANDSCAPE = 1,
	PORTRAIT = 2,
};

enum dsi_trigger_type {
	DSI_CMD_MODE_DMA,
	DSI_CMD_MODE_MDP,
};

enum dsi_panel_bl_ctrl {
	BL_PWM,
	BL_WLED,
	BL_DCS_CMD,
	UNKNOWN_CTRL,
};

enum dsi_panel_status_mode {
	ESD_BTA,
	ESD_REG,
	ESD_REG_NT35596,
	ESD_MAX,
};

enum dsi_ctrl_op_mode {
	DSI_LP_MODE,
	DSI_HS_MODE,
};

enum dsi_lane_map_type {
	DSI_LANE_MAP_0123,
	DSI_LANE_MAP_3012,
	DSI_LANE_MAP_2301,
	DSI_LANE_MAP_1230,
	DSI_LANE_MAP_0321,
	DSI_LANE_MAP_1032,
	DSI_LANE_MAP_2103,
	DSI_LANE_MAP_3210,
};

enum dsi_pm_type {
	DSI_CORE_PM,
	DSI_CTRL_PM,
	DSI_PANEL_PM,
	DSI_MAX_PM
};

#define CTRL_STATE_UNKNOWN		0x00
#define CTRL_STATE_PANEL_INIT		BIT(0)
#define CTRL_STATE_MDP_ACTIVE		BIT(1)

#define DSI_NON_BURST_SYNCH_PULSE	0
#define DSI_NON_BURST_SYNCH_EVENT	1
#define DSI_BURST_MODE			2

#define DSI_RGB_SWAP_RGB	0
#define DSI_RGB_SWAP_RBG	1
#define DSI_RGB_SWAP_BGR	2
#define DSI_RGB_SWAP_BRG	3
#define DSI_RGB_SWAP_GRB	4
#define DSI_RGB_SWAP_GBR	5

#define DSI_VIDEO_DST_FORMAT_RGB565		0
#define DSI_VIDEO_DST_FORMAT_RGB666		1
#define DSI_VIDEO_DST_FORMAT_RGB666_LOOSE	2
#define DSI_VIDEO_DST_FORMAT_RGB888		3

#define DSI_CMD_DST_FORMAT_RGB111	0
#define DSI_CMD_DST_FORMAT_RGB332	3
#define DSI_CMD_DST_FORMAT_RGB444	4
#define DSI_CMD_DST_FORMAT_RGB565	6
#define DSI_CMD_DST_FORMAT_RGB666	7
#define DSI_CMD_DST_FORMAT_RGB888	8

#define DSI_INTR_ERROR_MASK		BIT(25)
#define DSI_INTR_ERROR			BIT(24)
#define DSI_INTR_BTA_DONE_MASK          BIT(21)
#define DSI_INTR_BTA_DONE               BIT(20)
#define DSI_INTR_VIDEO_DONE_MASK	BIT(17)
#define DSI_INTR_VIDEO_DONE		BIT(16)
#define DSI_INTR_CMD_MDP_DONE_MASK	BIT(9)
#define DSI_INTR_CMD_MDP_DONE		BIT(8)
#define DSI_INTR_CMD_DMA_DONE_MASK	BIT(1)
#define DSI_INTR_CMD_DMA_DONE		BIT(0)
/* Update this if more interrupt masks are added in future chipsets */
#define DSI_INTR_TOTAL_MASK		0x2222AA02

#define DSI_CMD_TRIGGER_NONE		0x0	/* mdp trigger */
#define DSI_CMD_TRIGGER_TE		0x02
#define DSI_CMD_TRIGGER_SW		0x04
#define DSI_CMD_TRIGGER_SW_SEOF		0x05	/* cmd dma only */
#define DSI_CMD_TRIGGER_SW_TE		0x06

#define DSI_VIDEO_TERM  BIT(16)
#define DSI_MDP_TERM    BIT(8)
#define DSI_BTA_TERM    BIT(1)
#define DSI_CMD_TERM    BIT(0)

#define DSI_DATA_LANES_STOP_STATE	0xF
#define DSI_CLK_LANE_STOP_STATE		BIT(4)
#define DSI_DATA_LANES_ENABLED		0xF0

extern struct device dsi_dev;
extern u32 dsi_irq;
extern struct mdss_dsi_ctrl_pdata *ctrl_list[];

struct dsiphy_pll_divider_config {
	u32 clk_rate;
	u32 fb_divider;
	u32 ref_divider_ratio;
	u32 bit_clk_divider;	/* oCLK1 */
	u32 byte_clk_divider;	/* oCLK2 */
	u32 analog_posDiv;
	u32 digital_posDiv;
};

extern struct dsiphy_pll_divider_config pll_divider_config;

struct dsi_clk_mnd_table {
	u8 lanes;
	u8 bpp;
	u8 pll_digital_posDiv;
	u8 pclk_m;
	u8 pclk_n;
	u8 pclk_d;
};

static const struct dsi_clk_mnd_table mnd_table[] = {
	{ 1, 2,  8, 1, 1, 0},
	{ 1, 3, 12, 1, 1, 0},
	{ 2, 2,  4, 1, 1, 0},
	{ 2, 3,  6, 1, 1, 0},
	{ 3, 2,  1, 3, 8, 4},
	{ 3, 3,  4, 1, 1, 0},
	{ 4, 2,  2, 1, 1, 0},
	{ 4, 3,  3, 1, 1, 0},
};

struct dsi_clk_desc {
	u32 src;
	u32 m;
	u32 n;
	u32 d;
	u32 mnd_mode;
	u32 pre_div_func;
};


struct dsi_panel_cmds {
	char *buf;
	int blen;
	struct dsi_cmd_desc *cmds;
	int cmd_cnt;
	int link_state;
};

struct dsi_kickoff_action {
	struct list_head act_entry;
	void (*action) (void *);
	void *data;
};

struct dsi_drv_cm_data {
	struct regulator *vdd_vreg;
	struct regulator *vdd_io_vreg;
	struct regulator *vdda_vreg;
	int broadcast_enable;
};

struct dsi_pinctrl_res {
	struct pinctrl *pinctrl;
	struct pinctrl_state *gpio_state_active;
	struct pinctrl_state *gpio_state_suspend;
};

struct panel_horizontal_idle {
	int min;
	int max;
	int idle;
};

enum {
	DSI_CTRL_0,
	DSI_CTRL_1,
	DSI_CTRL_MAX,
};

#define DSI_CTRL_LEFT		DSI_CTRL_0
#define DSI_CTRL_RIGHT		DSI_CTRL_1

/* DSI controller #0 is always treated as a master in broadcast mode */
#define DSI_CTRL_MASTER		DSI_CTRL_0
#define DSI_CTRL_SLAVE		DSI_CTRL_1

#define DSI_BUS_CLKS	BIT(0)
#define DSI_LINK_CLKS	BIT(1)
#define DSI_ALL_CLKS	((DSI_BUS_CLKS) | (DSI_LINK_CLKS))

#define DSI_EV_PLL_UNLOCKED		0x0001
#define DSI_EV_MDP_FIFO_UNDERFLOW	0x0002
#define DSI_EV_DSI_FIFO_EMPTY		0x0004
#define DSI_EV_DLNx_FIFO_OVERFLOW	0x0008
#define DSI_EV_LP_RX_TIMEOUT		0x0010
#define DSI_EV_MDP_BUSY_RELEASE		0x80000000

struct mdss_dsi_ctrl_pdata {
	int ndx;	/* panel_num */
	int (*on) (struct mdss_panel_data *pdata);
	int (*off) (struct mdss_panel_data *pdata);
	int (*set_col_page_addr) (struct mdss_panel_data *pdata);
	int (*check_status) (struct mdss_dsi_ctrl_pdata *pdata);
	int (*check_read_status) (struct mdss_dsi_ctrl_pdata *pdata);
	int (*cmdlist_commit)(struct mdss_dsi_ctrl_pdata *ctrl, int from_mdp);
	void (*switch_mode) (struct mdss_panel_data *pdata, int mode);
	struct mdss_panel_data panel_data;
	unsigned char *ctrl_base;
	struct dss_io_data ctrl_io;
	struct dss_io_data mmss_misc_io;
	struct dss_io_data phy_io;
	int reg_size;
	u32 bus_clk_cnt;
	u32 link_clk_cnt;
	u32 flags;
	struct clk *mdp_core_clk;
	struct clk *ahb_clk;
	struct clk *axi_clk;
	struct clk *mmss_misc_ahb_clk;
	struct clk *byte_clk;
	struct clk *esc_clk;
	struct clk *pixel_clk;
	u8 ctrl_state;
	int panel_mode;
	int irq_cnt;
	int rst_gpio;
	int disp_en_gpio;
/* add bias enable vsp/vsn flag */
	int disp_en_gpio_vsp;
	int disp_en_gpio_vsn;
	int disp_te_gpio;
	int bklt_en_gpio;
	int mode_gpio;
	int bklt_ctrl;	/* backlight ctrl */
	int pwm_period;
	int pwm_pmic_gpio;
	int pwm_lpg_chan;
	int bklt_max;
	int new_fps;
	int pwm_enabled;
	bool panel_bias_vreg;
	struct mdss_rect roi;
	struct pwm_device *pwm_bl;
	struct dsi_drv_cm_data shared_pdata;
	u32 pclk_rate;
	u32 byte_clk_rate;
	struct dss_module_power power_data[DSI_MAX_PM];
	u32 dsi_irq_mask;
	struct mdss_hw *dsi_hw;
	struct mdss_intf_recovery *recovery;

	struct dsi_panel_cmds on_cmds;
	struct dsi_panel_cmds off_cmds;
#ifdef CONFIG_FB_AUTO_CABC
	struct dsi_panel_cmds dsi_panel_cabc_ui_cmds;
	struct dsi_panel_cmds dsi_panel_cabc_video_cmds;
#endif
	struct dsi_panel_cmds status_cmds;
	u32 status_cmds_rlen;
	u32 status_value;
	u32 status_error_count;

	struct dsi_panel_cmds video2cmd;
	struct dsi_panel_cmds cmd2video;

	struct dcs_cmd_list cmdlist;
	struct completion dma_comp;
	struct completion mdp_comp;
	struct completion video_comp;
	struct completion bta_comp;
	spinlock_t irq_lock;
	spinlock_t mdp_lock;
	int mdp_busy;
	struct mutex mutex;
	struct mutex cmd_mutex;
#ifdef CONFIG_HUAWEI_LCD
	struct mutex put_mutex;
	/*not need the mutex,so delete one line*/
#endif

	bool ulps;
	bool mmss_clamp;
	u32 ulps_clamp_ctrl_off;
	u32 ulps_phyrst_ctrl_off;

	struct dsi_buf tx_buf;
	struct dsi_buf rx_buf;
#ifdef CONFIG_HUAWEI_LCD
	u32 esd_check_enable;
	struct dsi_panel_cmds esd_cmds;
#endif
	struct dsi_buf status_buf;
	int status_mode;
	int rx_len;

	struct dsi_pinctrl_res pin_res;
/*add vars of dispaly color inversion*/
#ifdef CONFIG_HUAWEI_LCD
	struct dsi_panel_cmds dot_inversion_cmds;
	struct dsi_panel_cmds column_inversion_cmds;
	u32 long_read_flag;
	u32 skip_reg_read;
	char reg_expect_value;
	u32 reg_expect_count;
	u32 inversion_state;
	struct dsi_panel_cmds dsi_panel_inverse_on_cmds;
	struct dsi_panel_cmds dsi_panel_inverse_off_cmds;
	u32 esd_set_cabc_flag;
#endif

	int horizontal_idle_cnt;
	struct panel_horizontal_idle *line_idle;
	struct mdss_util_intf *mdss_util;
};

struct dsi_status_data {
	struct notifier_block fb_notifier;
	struct delayed_work check_status;
	struct msm_fb_data_type *mfd;
};

int dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);

int mdss_dsi_cmds_tx(struct mdss_dsi_ctrl_pdata *ctrl,
		struct dsi_cmd_desc *cmds, int cnt);

int mdss_dsi_cmds_rx(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_cmd_desc *cmds, int rlen);

void mdss_dsi_host_init(struct mdss_panel_data *pdata);
void mdss_dsi_op_mode_config(int mode,
				struct mdss_panel_data *pdata);
void mdss_dsi_cmd_mode_ctrl(int enable);
void mdp4_dsi_cmd_trigger(void);
void mdss_dsi_cmd_mdp_start(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_dsi_cmd_bta_sw_trigger(struct mdss_panel_data *pdata);
void mdss_dsi_ack_err_status(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_dsi_clk_ctrl(struct mdss_dsi_ctrl_pdata *ctrl,
	u8 clk_type, int enable);
void mdss_dsi_clk_req(struct mdss_dsi_ctrl_pdata *ctrl,
				int enable);
void mdss_dsi_controller_cfg(int enable,
				struct mdss_panel_data *pdata);
void mdss_dsi_sw_reset(struct mdss_panel_data *pdata, bool restore);

irqreturn_t mdss_dsi_isr(int irq, void *ptr);
void mdss_dsi_irq_handler_config(struct mdss_dsi_ctrl_pdata *ctrl_pdata);

void mdss_dsi_set_tx_power_mode(int mode, struct mdss_panel_data *pdata);
int mdss_dsi_clk_div_config(struct mdss_panel_info *panel_info,
			    int frame_rate);
int mdss_dsi_clk_init(struct platform_device *pdev,
		      struct mdss_dsi_ctrl_pdata *ctrl_pdata);
void mdss_dsi_clk_deinit(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int mdss_dsi_enable_bus_clocks(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
void mdss_dsi_disable_bus_clocks(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
void mdss_dsi_phy_disable(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_dsi_phy_init(struct mdss_panel_data *pdata);
void mdss_dsi_phy_sw_reset(unsigned char *ctrl_base);
void mdss_dsi_cmd_test_pattern(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_dsi_video_test_pattern(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_dsi_panel_pwm_cfg(struct mdss_dsi_ctrl_pdata *ctrl);

void mdss_dsi_ctrl_init(struct device *ctrl_dev,
			struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_dsi_cmd_mdp_busy(struct mdss_dsi_ctrl_pdata *ctrl);
void mdss_dsi_wait4video_done(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_dsi_cmdlist_commit(struct mdss_dsi_ctrl_pdata *ctrl, int from_mdp);
void mdss_dsi_cmdlist_kickoff(int intf);
int mdss_dsi_bta_status_check(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_dsi_reg_status_check(struct mdss_dsi_ctrl_pdata *ctrl);
bool __mdss_dsi_clk_enabled(struct mdss_dsi_ctrl_pdata *ctrl, u8 clk_type);
int mdss_dsi_ulps_config(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
int mdss_dsi_clamp_ctrl(struct mdss_dsi_ctrl_pdata *ctrl, int enable);
void mdss_dsi_ctrl_setup(struct mdss_panel_data *pdata);
u32 mdss_dsi_panel_cmd_read(struct mdss_dsi_ctrl_pdata *ctrl, char cmd0,
		char cmd1, void (*fxn)(int), char *rbuf, int len);
int mdss_dsi_panel_init(struct device_node *node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata,
		bool cmd_cfg_cont_splash);
int mdss_panel_get_dst_fmt(u32 bpp, char mipi_mode, u32 pixel_packing,
				char *dst_format);
void mdss_dsi_dln0_phy_err(struct mdss_dsi_ctrl_pdata *ctrl);
int mdss_dsi_register_recovery_handler(struct mdss_dsi_ctrl_pdata *ctrl,
		struct mdss_intf_recovery *recovery);

static inline const char *__mdss_dsi_pm_name(enum dsi_pm_type module)
{
	switch (module) {
	case DSI_CORE_PM:	return "DSI_CORE_PM";
	case DSI_CTRL_PM:	return "DSI_CTRL_PM";
	case DSI_PANEL_PM:	return "PANEL_PM";
	default:		return "???";
	}
}

static inline const char *__mdss_dsi_pm_supply_node_name(
	enum dsi_pm_type module)
{
	switch (module) {
	case DSI_CORE_PM:	return "qcom,core-supply-entries";
	case DSI_CTRL_PM:	return "qcom,ctrl-supply-entries";
	case DSI_PANEL_PM:	return "qcom,panel-supply-entries";
	default:		return "???";
	}
}

static inline bool mdss_dsi_broadcast_mode_enabled(void)
{
	return ctrl_list[DSI_CTRL_MASTER]->shared_pdata.broadcast_enable &&
		ctrl_list[DSI_CTRL_SLAVE] &&
		ctrl_list[DSI_CTRL_SLAVE]->shared_pdata.broadcast_enable;
}

static inline struct mdss_dsi_ctrl_pdata *mdss_dsi_get_master_ctrl(void)
{
	if (mdss_dsi_broadcast_mode_enabled())
		return ctrl_list[DSI_CTRL_MASTER];
	else
		return NULL;
}

static inline struct mdss_dsi_ctrl_pdata *mdss_dsi_get_slave_ctrl(void)
{
	if (mdss_dsi_broadcast_mode_enabled())
		return ctrl_list[DSI_CTRL_SLAVE];
	else
		return NULL;
}

static inline bool mdss_dsi_is_master_ctrl(struct mdss_dsi_ctrl_pdata *ctrl)
{
	return mdss_dsi_broadcast_mode_enabled() &&
		(ctrl->ndx == DSI_CTRL_MASTER);
}

static inline bool mdss_dsi_is_slave_ctrl(struct mdss_dsi_ctrl_pdata *ctrl)
{
	return mdss_dsi_broadcast_mode_enabled() &&
		(ctrl->ndx == DSI_CTRL_SLAVE);
}

static inline bool mdss_dsi_is_left_ctrl(struct mdss_dsi_ctrl_pdata *ctrl)
{
	return ctrl->ndx == DSI_CTRL_LEFT;
}

static inline bool mdss_dsi_is_right_ctrl(struct mdss_dsi_ctrl_pdata *ctrl)
{
	return ctrl->ndx == DSI_CTRL_RIGHT;
}

static inline struct mdss_dsi_ctrl_pdata *mdss_dsi_get_other_ctrl(
					struct mdss_dsi_ctrl_pdata *ctrl)
{
	if (ctrl->ndx == DSI_CTRL_RIGHT)
		return ctrl_list[DSI_CTRL_LEFT];

	return ctrl_list[DSI_CTRL_RIGHT];
}

static inline struct mdss_dsi_ctrl_pdata *mdss_dsi_get_ctrl_by_index(int ndx)
{
	if (ndx >= DSI_CTRL_MAX)
		return NULL;

	return ctrl_list[ndx];
}

static inline bool mdss_dsi_ulps_feature_enabled(
	struct mdss_panel_data *pdata)
{
	return pdata->panel_info.ulps_feature_enabled;
}

static inline bool mdss_dsi_split_display_enabled(void)
{
        /*
         * currently the only supported mode is split display.
         * So, if both controllers are initialized, then assume that
         * split display mode is enabled.
         */
        return ctrl_list[DSI_CTRL_MASTER] && ctrl_list[DSI_CTRL_SLAVE];
}

#endif /* MDSS_DSI_H */
