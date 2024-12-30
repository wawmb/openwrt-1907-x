/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_linux_msc.h

Abstract:

    FUXI Linux driver ioctl head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __L1C_SW_H___
#define __L1C_SW_H___

#define __ATTRIB_PACK_ 

#define L2CB_ID     0x2060
#define L2CB2_ID    0x2062
#define L1C_ID      0x1063
#define L2C_ID      0x1062
#define L1D_ID      0x1073
#define L1D2_ID     0x1083

#define DEBUG_INFOS(pAdapter, Catlog) 

#define DEBUG_CAT_NONE      0
#define DEBUG_CAT_PME       1    
#define DEBUG_CAT_MAC       2
#define DEBUG_CAT_PHY       4
#define DEBUG_CAT_PCIE      8

#define MAC_MDIO_ADDRESS_BUSY	1 //bit 0

#define MS_DELAY(_Adapter, _ms)     usleep((_ms)*1000)
#define US_DELAY(_Adapter, _us)     usleep(_us)


#define TX_OFFLOAD_THRESHOLD    (6*1024)    /* packet larger than it will no taskload */




#define MAC_TYPE	0	/* 1: FPGA, 0:ASIC */
#define PHY_TYPE	PHY_TYPE_ASIC

#define L1_EN		0
#define L0S_EN		0
#define PS_EN		0 // 1

/*
 * IOCTL function 
 */
int ioctl_mem_r32(PADAPTER adapter, u16 reg, u32 *pval);
int ioctl_mem_w32(PADAPTER adapter, u16 reg, u32 val);
int ioctl_mem_w8(PADAPTER adapter, u16 reg, u8 val);
int ioctl_cfg_r32(PADAPTER adapter, u16 reg, u32 *pval);
int ioctl_cfg_w32(PADAPTER adapter, u16 reg, u32 val);
int ioctl_io_r32(PADAPTER adapter, u16 reg, u32 *pval);
int ioctl_io_w32(PADAPTER adapter, u16 reg, u32 val);

int ioctl_read_phy_ext_reg(PADAPTER adapter, u8 dev, u16 reg, u16 *pval);
int ioctl_write_phy_ext_reg(PADAPTER adapter, u8 dev, u16 reg, u16 val);

int ioctl_read_phy_reg(PADAPTER adapter, u16 reg, u16 *pval);
int ioctl_write_phy_reg(PADAPTER adapter, u16 reg, u16 val);

bool ioctl_send_packets(PADAPTER adapter, u8 *buf, u32 size);
bool ioctl_receive_packets(PADAPTER adapter, u8 *buf, u32 size);

bool ioctl_diag_start(PADAPTER adapter);
bool ioctl_diag_end(PADAPTER adapter);
bool ioctl_diag_device_reset(PADAPTER adapter);


#define MEM_R32     ioctl_mem_r32
#define MEM_W32     ioctl_mem_w32
#define MEM_W8      ioctl_mem_w8

#define IO_R32      ioctl_io_r32
#define IO_W32      ioctl_io_w32

#define CFG_R32     ioctl_cfg_r32
#define CFG_W32     ioctl_cfg_w32

#define PHY_R       ioctl_read_phy_reg
#define PHY_W       ioctl_write_phy_reg

#define PHYEXT_R    ioctl_read_phy_ext_reg
#define PHYEXT_W    ioctl_write_phy_ext_reg

/****************************** IOCTL code ******************************/
#define FUXI_IOCTL_DFS_COMMAND _IOWR('M', 0x80, struct ext_ioctl_data)
struct ext_command_buf {
	void  *buf;
	u32 size_in;
	u32 size_out;
};

struct ext_command_data {
	u32 val0;
	u32 val1;
};

struct ext_command_mac {
	u32  num;
    union {
	    u32  val32;
	    u16  val16;
	    u8   val8;
    };
};

struct ext_command_mii {
	u16  dev;
	u16  num;
	u16  val;
};

struct ext_ioctl_data {
	u32	cmd_type;
#if 1
	struct ext_command_buf cmd_buf;
#else
    union {
	    struct ext_command_buf cmdu_buf;
	    struct ext_command_data cmdu_data;
	    struct ext_command_mac  cmdu_mac;
        struct ext_command_mii  cmdu_mii;
    }cmd_cmdu;
#endif
};
//#define cmd_buf     cmd_cmdu.cmdu_buf
//#define cmd_data   cmd_cmdu.cmdu_data
//#define cmd_mac   cmd_cmdu.cmdu_mac
//#define cmd_mii     cmd_cmdu.cmdu_mii

#define ALX_DFS_IOCTL_GMAC_REG_32       0x0001
#define ALX_DFS_IOCTL_SMAC_REG_32       0x0002
#define ALX_DFS_IOCTL_GMAC_REG_16       0x0003
#define ALX_DFS_IOCTL_SMAC_REG_16       0x0004
#define ALX_DFS_IOCTL_GMAC_REG_8        0x0005
#define ALX_DFS_IOCTL_SMAC_REG_8        0x0006

#define ALX_DFS_IOCTL_GMAC_CFG_32       0x0011
#define ALX_DFS_IOCTL_SMAC_CFG_32       0x0012

#define ALX_DFS_IOCTL_GMAC_IO_32        0x0021
#define ALX_DFS_IOCTL_SMAC_IO_32        0x0022

#define ALX_DFS_IOCTL_GMII_EXT_REG      0x0031
#define ALX_DFS_IOCTL_SMII_EXT_REG      0x0032
#define ALX_DFS_IOCTL_GMII_IDR_REG      0x0033
#define ALX_DFS_IOCTL_SMII_IDR_REG      0x0034

#define FUXI_DFS_IOCTL_DEVICE_INACTIVE   0x10001
#define FUXI_DFS_IOCTL_DEVICE_RESET      0x10002
#define FUXI_DFS_IOCTL_DIAG_BEGIN        0x10003
#define FUXI_DFS_IOCTL_DIAG_END          0x10004
#define FUXI_DFS_IOCTL_DIAG_TX_PKT       0x10005
#define FUXI_DFS_IOCTL_DIAG_RX_PKT       0x10006

#define ALX_DFS_IOCTL_ANNCE_CLEAR       0x20001
#define ALX_DFS_IOCTL_ANNCE_CONFIG      0x20002

#endif/*__L1C_SW_H___*/
