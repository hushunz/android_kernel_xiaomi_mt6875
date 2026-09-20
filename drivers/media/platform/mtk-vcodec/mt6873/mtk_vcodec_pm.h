/*
 * Copyright (c) 2016 MediaTek Inc.
 * Copyright (C) 2021 XiaoMi, Inc.
 * Author: Tiffany Lin <tiffany.lin@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _MTK_VCODEC_PM_H_
#define _MTK_VCODEC_PM_H_

#include "ion_drv.h"
#include "mach/mt_iommu.h"

extern struct ion_device *g_ion_device;

#define MTK_PLATFORM_STR        "platform:mt6873"
#define MTK_VDEC_RACING_INFO_OFFSET  0x100
#define MTK_VDEC_RACING_INFO_SIZE 68

/**
 * struct mtk_vcodec_pm - Power management data structure
 */
struct mtk_vcodec_pm {
	struct clk      *vdec_bus_clk_src;
	struct clk      *vencpll;

	struct clk      *vcodecpll;
	struct clk      *univpll_d2;
	struct clk      *clk_cci400_sel;
	struct clk      *vdecpll;
	struct clk      *vdec_sel;
	struct clk      *vencpll_d2;
	struct clk      *venc_sel;
	struct clk      *univpll1_d2;
	struct clk      *venc_lt_sel;
	struct clk      *img_resz;
	struct device   *larbvdec;
	struct device   *larbvenc;
	struct device   *larbvenclt;
	struct device   *dev;
	struct device_node      *chip_node;
	struct mtk_vcodec_dev   *mtkdev;

	struct clk *clk_MT_CG_SOC;           /* VDEC SOC*/
	struct clk *clk_MT_CG_VDEC0;         /* VDEC core 0*/
	struct clk *clk_MT_CG_VENC0;         /* VENC core 0*/
	struct clk *clk_MT_CG_VDEC1;         /* VDEC core 1*/

	atomic_t dec_active_cnt;
	__u32 vdec_racing_info[MTK_VDEC_RACING_INFO_SIZE];
	struct mutex dec_racing_info_mutex;
};

/* A12 的 vdec@16000000 有 9 个 reg 块，OPPO/A12 源码的枚举与之一一对应：
 *   reg[0]=0x1602f000 SYS        reg[1]=0x16000000 BASE  reg[2]=0x16020000 VLD
 *   reg[3]=0x16021000 MC         reg[4]=0x16023000 MV    reg[5]=0x16025000 MISC
 *   reg[6]=0x16010000 LAT_MISC   reg[7]=0x16011000 LAT_VLD
 *   reg[8]=0x16004000 RACING_CTRL
 * A11 那份枚举只有 6 项（缺 BASE/MC/MV），于是 LAT_MISC/LAT_VLD/RACING_CTRL
 * 全部向前错位：VDEC_MISC 落到 0x16020000、VDEC_RACING_CTRL 落到 0x16025000。
 * mtk_vcodec_dec_clock_off() 读 VDEC_RACING_CTRL + 0x100 就变成读
 * 0x16025100——那是 A12 布局里的 VDEC_MISC 块，DEVAPC 不允许 APMCU 读，
 * 于是 "APMCU_read access violation slave: VDECSYS" -> BUG_ON -> 打开相册
 * （用到 vdec 解码）时整机重启。补回 A12 的三项，使索引与 DTB 对齐。
 */
enum mtk_dec_dtsi_reg_idx {
	VDEC_SYS,
	VDEC_BASE,
	VDEC_VLD,
	VDEC_MC,
	VDEC_MV,
	VDEC_MISC,
	VDEC_LAT_MISC,
	VDEC_LAT_VLD,
	VDEC_RACING_CTRL,
	NUM_MAX_VDEC_REG_BASE,
};

enum mtk_enc_dtsi_reg_idx {
	VENC_SYS,
	VENC_C1_SYS,
	NUM_MAX_VENC_REG_BASE
};

#endif /* _MTK_VCODEC_PM_H_ */
