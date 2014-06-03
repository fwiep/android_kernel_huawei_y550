/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#ifndef __MSM_CLOCKS_8994_H
#define __MSM_CLOCKS_8994_H

/* clock_rpm controlled clocks */
#define clk_cxo_clk_src 0x79e95308
#define clk_pnoc_clk 0x4325d220
#define clk_ocmemgx_clk 0xc91618fb
#define clk_pnoc_a_clk 0x2808c12b
#define clk_bimc_clk 0x4b80bf00
#define clk_bimc_a_clk 0x4b25668a
#define clk_cnoc_clk 0xd5ccb7f4
#define clk_cnoc_a_clk 0xd8fe2ccc
#define clk_gfx3d_clk_src 0x917f76ef
#define clk_ocmemgx_a_clk 0x310204ad
#define clk_snoc_clk 0x2c341aa0
#define clk_snoc_a_clk 0x8fcef2af
#define clk_bb_clk1 0xf5304268
#define clk_bb_clk1_ao 0xfa113810
#define clk_bb_clk1_pin 0x6dd0a779
#define clk_bb_clk1_pin_ao 0x9b637772
#define clk_bb_clk2 0xfe15cb87
#define clk_bb_clk2_ao 0x59682706
#define clk_bb_clk2_pin 0x498938e5
#define clk_bb_clk2_pin_ao 0x52513787
#define clk_bimc_msmbus_clk 0xd212feea
#define clk_bimc_msmbus_a_clk 0x71d1a499
#define clk_ce1_a_clk 0x44a833fe
#define clk_ce2_a_clk 0x6a30b14b
#define clk_ce3_a_clk 0xa67fa700
#define clk_cnoc_msmbus_clk 0x62228b5d
#define clk_cnoc_msmbus_a_clk 0x67442955
#define clk_cxo_clk_src_ao 0x64eb6004
#define clk_cxo_dwc3_clk 0xf79c19f6
#define clk_cxo_lpm_clk 0x94adbf3d
#define clk_cxo_otg_clk 0x4eec0bb9
#define clk_cxo_pil_lpass_clk 0xe17f0ff6
#define clk_div_clk1 0xaa1157a6
#define clk_div_clk1_ao 0x6b943d68
#define clk_div_clk2 0xd454019f
#define clk_div_clk2_ao 0x53f9e788
#define clk_div_clk3 0xa9a55a68
#define clk_div_clk3_ao 0x3d6725a8
#define clk_gfx3d_clk_src_ao 0x8b5b92f4
#define clk_ipa_clk 0xfa685cda
#define clk_ipa_a_clk 0xeeec2919
#define clk_ln_bb_clk 0x3ab0b36d
#define clk_ln_bb_a_clk 0xc7257ea8
#define clk_mmssnoc_ahb_clk 0xccd4bd4c
#define clk_mmssnoc_ahb_a_clk 0x3f1a62ce
#define clk_ocmemgx_core_clk 0xaad7dbe5
#define clk_ocmemgx_msmbus_clk 0x3968c738
#define clk_ocmemgx_msmbus_a_clk 0x66dd774f
#define clk_oxili_gfx3d_clk_src 0xe0405056
#define clk_pnoc_keepalive_a_clk 0xf8f91f0b
#define clk_pnoc_msmbus_clk 0x38b95c77
#define clk_pnoc_msmbus_a_clk 0x8c9b4e93
#define clk_pnoc_pm_clk 0xd6f7dfb9
#define clk_pnoc_sps_clk 0xd482ecc7
#define clk_qdss_clk 0x1492202a
#define clk_qdss_a_clk 0xdd121669
#define clk_rf_clk1 0xaabeea5a
#define clk_rf_clk1_ao 0x72a10cb8
#define clk_rf_clk1_pin 0x8f463562
#define clk_rf_clk1_pin_ao 0x62549ff6
#define clk_rf_clk2 0x24a30992
#define clk_rf_clk2_ao 0x944d8bbd
#define clk_rf_clk2_pin 0xa7c5602a
#define clk_rf_clk2_pin_ao 0x2d75eb4d
#define clk_snoc_msmbus_clk 0xe6900bb6
#define clk_snoc_msmbus_a_clk 0x5d4683bd
#define clk_ce1_clk 0x42229c55
#define clk_gcc_ce1_ahb_m_clk 0x2eb28c01
#define clk_gcc_ce1_axi_m_clk 0xc174dfba
#define clk_ce2_clk 0x7b80e25b
#define clk_gcc_ce2_ahb_m_clk 0xe57ce2b1
#define clk_gcc_ce2_axi_m_clk 0x4feb55e3
#define clk_ce3_clk 0xb7c009b6
#define clk_gcc_ce3_ahb_m_clk 0x527880ff
#define clk_gcc_ce3_axi_m_clk 0xc8e9a915
#define clk_rpm_debug_mux 0x25cd1f3a

/* clock_gcc controlled clocks */
#define clk_gcc_xo 0x7503042f
#define clk_gcc_xo_a_clk 0x344f46f4
#define clk_debug_mmss_clk 0x977c99b6
#define clk_debug_rpm_clk 0x8e2b07ca
#define clk_gpll0 0x1ebe3bc4
#define clk_gpll0_ao 0xa1368304
#define clk_gpll0_out_main 0xe9374de7
#define clk_gpll4 0xb3b5d85b
#define clk_gpll4_out_main 0xa9a0ab9d
#define clk_ufs_axi_clk_src 0x297ca380
#define clk_usb30_master_clk_src 0xc6262f89
#define clk_blsp1_qup1_i2c_apps_clk_src 0x17f78f5e
#define clk_blsp1_qup1_spi_apps_clk_src 0xf534c4fa
#define clk_blsp1_qup2_i2c_apps_clk_src 0x8de71c79
#define clk_blsp1_qup2_spi_apps_clk_src 0x33cf809a
#define clk_blsp1_qup3_i2c_apps_clk_src 0xf161b902
#define clk_blsp1_qup3_spi_apps_clk_src 0x5e95683f
#define clk_blsp1_qup4_i2c_apps_clk_src 0xb2ecce68
#define clk_blsp1_qup4_spi_apps_clk_src 0xddb5bbdb
#define clk_blsp1_qup5_i2c_apps_clk_src 0x71ea7804
#define clk_blsp1_qup5_spi_apps_clk_src 0x9752f35f
#define clk_blsp1_qup6_i2c_apps_clk_src 0x28806803
#define clk_blsp1_qup6_spi_apps_clk_src 0x44a1edc4
#define clk_blsp1_uart1_apps_clk_src 0xf8146114
#define clk_blsp1_uart2_apps_clk_src 0xfc9c2f73
#define clk_blsp1_uart3_apps_clk_src 0x600497f2
#define clk_blsp1_uart4_apps_clk_src 0x56bff15c
#define clk_blsp1_uart5_apps_clk_src 0x218ef697
#define clk_blsp1_uart6_apps_clk_src 0x8fbdbe4c
#define clk_blsp2_qup1_i2c_apps_clk_src 0xd6d1e95d
#define clk_blsp2_qup1_spi_apps_clk_src 0xcc1b8365
#define clk_blsp2_qup2_i2c_apps_clk_src 0x603b5c51
#define clk_blsp2_qup2_spi_apps_clk_src 0xd577dc44
#define clk_blsp2_qup3_i2c_apps_clk_src 0xea82959c
#define clk_blsp2_qup3_spi_apps_clk_src 0xd04b1e92
#define clk_blsp2_qup4_i2c_apps_clk_src 0x73dc968c
#define clk_blsp2_qup4_spi_apps_clk_src 0x25d4a2b1
#define clk_blsp2_qup5_i2c_apps_clk_src 0xcc3698bd
#define clk_blsp2_qup5_spi_apps_clk_src 0xfa0cf45e
#define clk_blsp2_qup6_i2c_apps_clk_src 0x2fa53151
#define clk_blsp2_qup6_spi_apps_clk_src 0x5ca86755
#define clk_blsp2_uart1_apps_clk_src 0x562c66dc
#define clk_blsp2_uart2_apps_clk_src 0xdd448080
#define clk_blsp2_uart3_apps_clk_src 0x46b2e90f
#define clk_blsp2_uart4_apps_clk_src 0x23a093d2
#define clk_blsp2_uart5_apps_clk_src 0xe067616a
#define clk_blsp2_uart6_apps_clk_src 0xe02d2829
#define clk_gp1_clk_src 0xad85b97a
#define clk_gp2_clk_src 0xfb1f0065
#define clk_gp3_clk_src 0x63b693d6
#define clk_pcie_0_aux_clk_src 0x8f5d3dbf
#define clk_pcie_0_pipe_clk_src 0xba5b3b52
#define clk_pcie_1_aux_clk_src 0x158b62ca
#define clk_pcie_1_pipe_clk_src 0x0dfb9f03
#define clk_pdm2_clk_src 0x31e494fd
#define clk_sdcc1_apps_clk_src 0xd4975db2
#define clk_sdcc2_apps_clk_src 0xfc46c821
#define clk_sdcc3_apps_clk_src 0xea34c7f4
#define clk_sdcc4_apps_clk_src 0x7aaaaa0c
#define clk_tsif_ref_clk_src 0x4e9042d1
#define clk_usb30_mock_utmi_clk_src 0xa024a976
#define clk_usb3_phy_aux_clk_src 0x15eec63c
#define clk_usb_hs_system_clk_src 0x28385546
#define clk_gcc_pcie_phy_0_reset 0x6bb4df33
#define clk_gcc_pcie_phy_1_reset 0x5fc03a70
#define clk_gcc_qusb2_phy_reset 0x3ce5fa84
#define clk_gcc_usb3_phy_reset 0x03d559f1
#define clk_gpll0_out_mmsscc 0x0ded70aa
#define clk_gpll0_out_msscc 0x7d794829
#define clk_pcie_0_phy_ldo 0x1d30d092
#define clk_pcie_1_phy_ldo 0x63474b42
#define clk_ufs_phy_ldo 0x98111fee
#define clk_usb_ss_phy_ldo 0x124410f7
#define clk_gcc_bam_dma_ahb_clk 0xaacf5929
#define clk_gcc_blsp1_ahb_clk 0x8caa5b4f
#define clk_gcc_blsp1_qup1_i2c_apps_clk 0xc303fae9
#define clk_gcc_blsp1_qup1_spi_apps_clk 0x759a76b0
#define clk_gcc_blsp1_qup2_i2c_apps_clk 0x1076f220
#define clk_gcc_blsp1_qup2_spi_apps_clk 0x3e77d48f
#define clk_gcc_blsp1_qup3_i2c_apps_clk 0x9e25ac82
#define clk_gcc_blsp1_qup3_spi_apps_clk 0xfb978880
#define clk_gcc_blsp1_qup4_i2c_apps_clk 0xd7f40f6f
#define clk_gcc_blsp1_qup4_spi_apps_clk 0x80f8722f
#define clk_gcc_blsp1_qup5_i2c_apps_clk 0xacae5604
#define clk_gcc_blsp1_qup5_spi_apps_clk 0xbf3e15d7
#define clk_gcc_blsp1_qup6_i2c_apps_clk 0x5c6ad820
#define clk_gcc_blsp1_qup6_spi_apps_clk 0x780d9f85
#define clk_gcc_blsp1_uart1_apps_clk 0xc7c62f90
#define clk_gcc_blsp1_uart2_apps_clk 0xf8a61c96
#define clk_gcc_blsp1_uart3_apps_clk 0xc3298bd7
#define clk_gcc_blsp1_uart4_apps_clk 0x26be16c0
#define clk_gcc_blsp1_uart5_apps_clk 0x28a6bc74
#define clk_gcc_blsp1_uart6_apps_clk 0x28fd3466
#define clk_gcc_blsp2_ahb_clk 0x8f283c1d
#define clk_gcc_blsp2_qup1_i2c_apps_clk 0x9ace11dd
#define clk_gcc_blsp2_qup1_spi_apps_clk 0xa32604cc
#define clk_gcc_blsp2_qup2_i2c_apps_clk 0x1bf9a57e
#define clk_gcc_blsp2_qup2_spi_apps_clk 0xbf54ca6d
#define clk_gcc_blsp2_qup3_i2c_apps_clk 0x336d4170
#define clk_gcc_blsp2_qup3_spi_apps_clk 0xc68509d6
#define clk_gcc_blsp2_qup4_i2c_apps_clk 0xbd22539d
#define clk_gcc_blsp2_qup4_spi_apps_clk 0x01a72b93
#define clk_gcc_blsp2_qup5_i2c_apps_clk 0xe2b2ce1d
#define clk_gcc_blsp2_qup5_spi_apps_clk 0xf40999cd
#define clk_gcc_blsp2_qup6_i2c_apps_clk 0x894bcea4
#define clk_gcc_blsp2_qup6_spi_apps_clk 0xfe1bd34a
#define clk_gcc_blsp2_uart1_apps_clk 0x8c3512ff
#define clk_gcc_blsp2_uart2_apps_clk 0x1e1965a3
#define clk_gcc_blsp2_uart3_apps_clk 0x382415ab
#define clk_gcc_blsp2_uart4_apps_clk 0x87a44b42
#define clk_gcc_blsp2_uart5_apps_clk 0x5cd30649
#define clk_gcc_blsp2_uart6_apps_clk 0x8feee5ab
#define clk_gcc_boot_rom_ahb_clk 0xde2adeb1
#define clk_gcc_gp1_clk 0x057f7b69
#define clk_gcc_gp2_clk 0x9bf83ffd
#define clk_gcc_gp3_clk 0xec6539ee
#define clk_gcc_lpass_q6_axi_clk 0xa9612654
#define clk_gcc_mss_cfg_ahb_clk 0x111cde81
#define clk_gcc_mss_q6_bimc_axi_clk 0x67544d62
#define clk_gcc_pcie_0_aux_clk 0x3d2e3ece
#define clk_gcc_pcie_0_cfg_ahb_clk 0x4dd325c3
#define clk_gcc_pcie_0_mstr_axi_clk 0x3f85285b
#define clk_gcc_pcie_0_pipe_clk 0x4f37621e
#define clk_gcc_pcie_0_slv_axi_clk 0xd69638a1
#define clk_gcc_pcie_1_aux_clk 0xc9bb962c
#define clk_gcc_pcie_1_cfg_ahb_clk 0xb6338658
#define clk_gcc_pcie_1_mstr_axi_clk 0xc20f6269
#define clk_gcc_pcie_1_pipe_clk 0xc1627422
#define clk_gcc_pcie_1_slv_axi_clk 0xd54e40d6
#define clk_gcc_pdm2_clk 0x99d55711
#define clk_gcc_pdm_ahb_clk 0x365664f6
#define clk_gcc_prng_ahb_clk 0x397e7eaa
#define clk_gcc_sdcc1_ahb_clk 0x691e0caa
#define clk_gcc_sdcc1_apps_clk 0x9ad6fb96
#define clk_gcc_sdcc2_ahb_clk 0x23d5727f
#define clk_gcc_sdcc2_apps_clk 0x861b20ac
#define clk_gcc_sdcc3_ahb_clk 0x565b2c03
#define clk_gcc_sdcc3_apps_clk 0x0b27aeac
#define clk_gcc_sdcc4_ahb_clk 0x64f3e6a8
#define clk_gcc_sdcc4_apps_clk 0xbf7c4dc8
#define clk_gcc_sys_noc_ufs_axi_clk 0x19d38312
#define clk_gcc_sys_noc_usb3_axi_clk 0x94d26800
#define clk_gcc_tsif_ahb_clk 0x88d2822c
#define clk_gcc_tsif_ref_clk 0x8f1ed2c2
#define clk_gcc_ufs_ahb_clk 0x1914bb84
#define clk_gcc_ufs_axi_clk 0x47c743a7
#define clk_gcc_ufs_rx_cfg_clk 0xa6747786
#define clk_gcc_ufs_rx_symbol_0_clk 0x7f43251c
#define clk_gcc_ufs_rx_symbol_1_clk 0x03182fde
#define clk_gcc_ufs_tx_cfg_clk 0xba2cf8b5
#define clk_gcc_ufs_tx_symbol_0_clk 0x6a9f747a
#define clk_gcc_ufs_tx_symbol_1_clk 0x367fef66
#define clk_gcc_usb2_hs_phy_sleep_clk 0x2e4d8839
#define clk_gcc_usb30_master_clk 0xb3b4e2cb
#define clk_gcc_usb30_mock_utmi_clk 0xa800b65a
#define clk_gcc_usb30_sleep_clk 0xd0b65c92
#define clk_gcc_usb3_phy_aux_clk 0x0d9a36e0
#define clk_gcc_usb3_phy_pipe_clk 0xf279aff2
#define clk_gcc_usb_hs_ahb_clk 0x72ce8032
#define clk_gcc_usb_hs_system_clk 0xa11972e5
#define clk_gcc_usb_phy_cfg_ahb2phy_clk 0xd1231a0e

/* clock_mmss controlled clocks */
#define clk_mmsscc_xo 0x05e63704
#define clk_mmsscc_gpll0 0xe900c515
#define clk_mmsscc_mmssnoc_ahb 0x7b4bd6f7
#define clk_mmpll0 0xdd83b751
#define clk_mmpll0_out_main 0x2f996a31
#define clk_mmpll4 0x22c063c1
#define clk_mmpll4_out_main 0xfb21c2fd
#define clk_mmpll1 0x6da7fb90
#define clk_mmpll1_out_main 0xa0d3a7da
#define clk_mmpll3 0x18c76899
#define clk_mmpll3_out_main 0x6eb6328f
#define clk_axi_clk_src 0x6617efab
#define clk_mmpll5 0xa41e1936
#define clk_mmpll5_out_main 0xcc1897bf
#define clk_csi0_clk_src 0x227e65bc
#define clk_vcodec0_clk_src 0xbc193019
#define clk_csi1_clk_src 0x6a2a6c36
#define clk_csi2_clk_src 0x4113589f
#define clk_csi3_clk_src 0xfd934012
#define clk_vfe0_clk_src 0xa0c2bd8f
#define clk_vfe1_clk_src 0x4e357366
#define clk_cpp_clk_src 0x8382f56d
#define clk_jpeg1_clk_src 0xee282bdf
#define clk_jpeg2_clk_src 0x5ad927f3
#define clk_csi2phytimer_clk_src 0x62ffea9c
#define clk_fd_core_clk_src 0xe4799ab7
#define clk_mdp_clk_src 0x6dc1f8f1
#define clk_ocmemnoc_clk_src 0x20297054
#define clk_cci_clk_src 0x822f3d97
#define clk_mmss_gp0_clk_src 0x081d3661
#define clk_mmss_gp1_clk_src 0xc620f96d
#define clk_jpeg0_clk_src 0x9a0a0ac3
#define clk_jpeg_dma_clk_src 0xb68afcea
#define clk_mclk0_clk_src 0x266b3853
#define clk_mclk1_clk_src 0xa73cad0c
#define clk_mclk2_clk_src 0x42545468
#define clk_mclk3_clk_src 0x2bfbb714
#define clk_csi0phytimer_clk_src 0xc8a309be
#define clk_csi1phytimer_clk_src 0x7c0fe23a
#define clk_esc0_clk_src 0xb41d7c38
#define clk_esc1_clk_src 0x3b0afa42
#define clk_hdmi_clk_src 0xb40aeea9
#define clk_vsync_clk_src 0xecb43940
#define clk_rbbmtimer_clk_src 0x17649ecc
#define clk_camss_ahb_clk 0xc4ff91d4
#define clk_camss_cci_cci_ahb_clk 0x12aec62d
#define clk_camss_cci_cci_clk 0xc9a1bf11
#define clk_camss_vfe_cpp_ahb_clk 0xea097d83
#define clk_camss_vfe_cpp_axi_clk 0xccfc9229
#define clk_camss_vfe_cpp_clk 0x3ca47975
#define clk_camss_csi0_ahb_clk 0x6e29c972
#define clk_camss_csi0_clk 0x30862ddb
#define clk_camss_csi0phy_clk 0x2cecfb84
#define clk_camss_csi0pix_clk 0x6946f77b
#define clk_camss_csi0rdi_clk 0x83645ef5
#define clk_camss_csi1_ahb_clk 0xccc15f06
#define clk_camss_csi1_clk 0xb150f052
#define clk_camss_csi1phy_clk 0xb989f06d
#define clk_camss_csi1pix_clk 0x58d19bf3
#define clk_camss_csi1rdi_clk 0x4d2f3352
#define clk_camss_csi2_ahb_clk 0x92d02d75
#define clk_camss_csi2_clk 0x74fc92e8
#define clk_camss_csi2phy_clk 0xda05d9d8
#define clk_camss_csi2pix_clk 0xf8ed0731
#define clk_camss_csi2rdi_clk 0xdc1b2081
#define clk_camss_csi3_ahb_clk 0xee5e459c
#define clk_camss_csi3_clk 0x39488fdd
#define clk_camss_csi3phy_clk 0x8b6063b9
#define clk_camss_csi3pix_clk 0xd82bd467
#define clk_camss_csi3rdi_clk 0xb6750046
#define clk_camss_csi_vfe0_clk 0x3023937a
#define clk_camss_csi_vfe1_clk 0xe66fa522
#define clk_camss_gp0_clk 0xcee7e51d
#define clk_camss_gp1_clk 0x41f1c2e3
#define clk_camss_ispif_ahb_clk 0x9a212c6d
#define clk_camss_jpeg_dma_clk 0x2336e65d
#define clk_camss_jpeg_jpeg0_clk 0xa1f09a89
#define clk_camss_jpeg_jpeg1_clk 0x32952078
#define clk_camss_jpeg_jpeg2_clk 0xd3a2ff99
#define clk_camss_jpeg_jpeg_ahb_clk 0x2b877145
#define clk_camss_jpeg_jpeg_axi_clk 0x07eeae7b
#define clk_camss_mclk0_clk 0xcf0c61e0
#define clk_camss_mclk1_clk 0xd1410ed4
#define clk_camss_mclk2_clk 0x851286f2
#define clk_camss_mclk3_clk 0x4db11c45
#define clk_camss_micro_ahb_clk 0x33a23277
#define clk_camss_phy0_csi0phytimer_clk 0x02248c8b
#define clk_camss_phy1_csi1phytimer_clk 0x690fe05b
#define clk_camss_phy2_csi2phytimer_clk 0x93daa279
#define clk_camss_top_ahb_clk 0x8f8b2d33
#define clk_camss_vfe_vfe0_clk 0x373027a2
#define clk_camss_vfe_vfe1_clk 0xb8d03898
#define clk_camss_vfe_vfe_ahb_clk 0xbd885885
#define clk_camss_vfe_vfe_axi_clk 0x8412c7db
#define clk_fd_ahb_clk 0x868a2c5c
#define clk_fd_axi_clk 0x60d01d11
#define clk_fd_core_clk 0x3badcae4
#define clk_fd_core_uar_clk 0x7e624e15
#define clk_mdss_ahb_clk 0x684ccb41
#define clk_mdss_axi_clk 0xcc07d687
#define clk_mdss_esc0_clk 0x28cafbe6
#define clk_mdss_esc1_clk 0xc22c6883
#define clk_mdss_hdmi_ahb_clk 0x01cef516
#define clk_mdss_hdmi_clk 0x097a6de9
#define clk_mdss_mdp_clk 0x618336ac
#define clk_mdss_vsync_clk 0x42a022d3
#define clk_mmss_misc_ahb_clk 0xea30b0e7
#define clk_mmss_mmssnoc_axi_clk 0x63753a4b
#define clk_mmss_s0_axi_clk 0xcbd7b001
#define clk_ocmemcx_ocmemnoc_clk 0x37acd041
#define clk_oxili_gfx3d_clk 0x40c75e70
#define clk_oxili_rbbmtimer_clk 0x18e21c57
#define clk_oxilicx_ahb_clk 0xcc8b032c
#define clk_venus0_ahb_clk 0x6694087d
#define clk_venus0_axi_clk 0x34fecbbe
#define clk_venus0_ocmemnoc_clk 0x590416b8
#define clk_venus0_vcodec0_clk 0xaf0dbde4
#define clk_venus0_core0_vcodec_clk 0xbe6e61f9
#define clk_venus0_core1_vcodec_clk 0x6324869c
#define clk_venus0_core2_vcodec_clk 0x24fa20a3
#define clk_mmss_debug_mux 0xe646ffda

/* clock_mdss controlled clocks */
#define clk_pclk0_clk_src 0xccac1f35
#define clk_pclk1_clk_src 0x090f68ac
#define clk_byte0_clk_src 0x75cc885b
#define clk_byte1_clk_src 0x63c2c955
#define clk_extpclk_clk_src 0xb2c31abd
#define clk_mdss_byte0_clk 0xf5a03f64
#define clk_mdss_byte1_clk 0xb8c7067d
#define clk_mdss_extpclk_clk 0xfa5aadb0
#define clk_mdss_pclk0_clk 0x3487234a
#define clk_mdss_pclk1_clk 0xd5804246

/* clock_cpu controlled clocks */
#define clk_a57_pll0 0xd01177bc
#define clk_a57_pll1 0x546813fa
#define clk_a53_pll0 0xa24d446b
#define clk_a53_pll1 0xdc2957a1
#define clk_a53_hf_mux 0xae9fcd1a
#define clk_a53_lf_mux 0x541f1e40
#define clk_a57_hf_mux 0x11a12cf4
#define clk_a57_lf_mux 0xc4923785
#define clk_sys_apcsaux_clk 0x5ac5f5db
#define clk_a53_lf_mux_pll0_div 0xdd845061
#define clk_a53_lf_mux_pll1_div 0xd8218380
#define clk_a53_pll0_main 0xb2cc34c0
#define clk_a53_pll1_main 0x4fea3e81
#define clk_a53_clk 0x5c9f8836
#define clk_a57_clk 0x6c7dc3ea
#define clk_cci_clk 0x96854074
#define clk_cci_pll 0x9f1ea9a6
#define clk_cci_hf_mux 0x1e4a0b42
#define clk_cci_lf_mux 0x78b92bfd
#define clk_xo_ao 0x480207b3
#define clk_a57_debug_mux 0x0a9d77c3
#define clk_a53_debug_mux 0x034d8e87
#define clk_cpu_debug_mux 0x3ae8bcb2
#define clk_a57_div_clk 0x4fdce8aa
#define clk_a53_div_clk 0x6006022b

/* clock_debug controlled clocks */
#define clk_gcc_debug_mux 0x8121ac15

#endif
