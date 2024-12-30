/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_linux_msc.cpp

Abstract:

    FUXI Linux driver ioctl function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <linux/mii.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#ifndef CONFIG_MIPS_MUSIC
#include <net/if.h>
#endif
#ifdef __x86
#include <sys/io.h>
#endif

#include <errno.h>

#include "fuxipci_comm.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_linux_driver.h"
#include "fuxipci_linux_msc.h"
#include "fuxipci_err.h"

int ioctl_mem_r32(PADAPTER adapter, u16 reg, u32 *pval)
{
    *pval = readl((unsigned char *)gFuxiMemBase + ((gTarget + reg) & MAP_MASK));
	return 0;
}

int ioctl_mem_w32(PADAPTER adapter, u16 reg, u32 val)
{
    writel(val, (unsigned char *)gFuxiMemBase + ((gTarget + reg) & MAP_MASK));
	return 0;
}
#if 0
int ioctl_mem_w8(PADAPTER adapter, u16 reg, u8 val)
{
	struct ext_ioctl_data ext;

	ext.cmd_type = ALX_DFS_IOCTL_SMAC_REG_8;
	ext.cmd_mac.num = reg;
	ext.cmd_mac.val8 = val;

	if (ioctl(adapter->debugfs_fd, FUXI_IOCTL_DFS_COMMAND, (void *)&ext) < 0) {
        	return STATUS_FAILED;
    	}
	return 0;
}
#endif

int ioctl_cfg_r32(PADAPTER adapter, u16 reg, u32 *pval)
{
#ifdef __x86
    u32 cfg_reg = 0;
    int ret = 0;

    /* The unit is word of reg */
    if(reg >> 6){
        printf("Reading gmac config register out of range, reg %x. reg must be 6 bits.\n", reg);
        fflush(stdout);
        return STATUS_FAILED;
    }
    ret = iopl(3);
    if(ret < 0){
        printf("iopl set to high level error\n");
        return STATUS_FAILED;
    }
    cfg_reg = 0;
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_POS, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_LEN, FUXI_PCI_CONFIG_ADDR_OP_ENABLED_DATA_REG);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_BUS_POS, FUXI_PCI_CONFIG_ADDR_OP_BUS_LEN, gFuxiPciBus);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_DEV_POS, FUXI_PCI_CONFIG_ADDR_OP_DEV_LEN, gFuxiPciDev);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_FUNC_POS, FUXI_PCI_CONFIG_ADDR_OP_FUNC_LEN, gFuxiPciFunc);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_REG_POS, FUXI_PCI_CONFIG_ADDR_OP_REG_LEN, reg / 4);
    
    outl(cfg_reg, FUXI_PCI_CONFIG_ADDR_REGISTER);
    *pval = inl(FUXI_PCI_CONFIG_DATA_REGISTER);

    iopl(0);
    if(ret<0){
        printf("iopl set to low level error\n");
        return STATUS_FAILED;
	}
#endif

	return 0;
}


int ioctl_cfg_w32(PADAPTER adapter, u16 reg, u32 val)
{
#ifdef __x86
    int ret;
    u32 cfg_reg;
    if(reg >> 6){
        printf("Reading gmac config register out of range, reg %x. reg must be 6 bits.\n", reg);
        fflush(stdout);
        return STATUS_FAILED;
    }
    
    ret = iopl(3);
    if(ret < 0){
        printf("iopl set to high level error\n");
        return STATUS_FAILED;
    }

    cfg_reg = 0;
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_POS, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_LEN, FUXI_PCI_CONFIG_ADDR_OP_ENABLED_DATA_REG);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_BUS_POS, FUXI_PCI_CONFIG_ADDR_OP_BUS_LEN, gFuxiPciBus);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_DEV_POS, FUXI_PCI_CONFIG_ADDR_OP_DEV_LEN, gFuxiPciDev);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_FUNC_POS, FUXI_PCI_CONFIG_ADDR_OP_FUNC_LEN, gFuxiPciFunc);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_REG_POS, FUXI_PCI_CONFIG_ADDR_OP_REG_LEN, reg / 4);

    outl(cfg_reg, FUXI_PCI_CONFIG_ADDR_REGISTER);
    outl(val, FUXI_PCI_CONFIG_DATA_REGISTER);

    iopl(0);
    if(ret<0){
        printf("iopl set to low level error\n");
        return STATUS_FAILED;
    }
#endif
	return 0;
}

int ioctl_io_r32(PADAPTER adapter, u16 reg, u32 *pval)
{
    //*pval = readl((unsigned char *)gFuxiMemBase + ((gTarget + reg) & MAP_MASK));
	return 0;
}


int ioctl_io_w32(PADAPTER adapter, u16 reg, u32 val)
{

    //writel(val, (unsigned char *)gFuxiMemBase + ((gTarget + reg) & MAP_MASK));
	return 0;
}


int ioctl_read_phy_ext_reg(PADAPTER adapter, u8 dev, u16 reg, u16 *pval)
{
	return 0;
}


int ioctl_write_phy_ext_reg(PADAPTER adapter, u8 dev, u16 reg, u16 val)
{
	return 0;
}

int ioctl_read_phy_reg(PADAPTER adapter, u16 reg, u16 *pval)
{
    u32 regval = 0, regret;
    u32 mdioctrl = reg * 0x10000 + 0x800020d;
    int busy = 15;

    writel(mdioctrl, (u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_ADDRESS);
    do {
        regval = readl((u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_ADDRESS);
        busy--;
    }while((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));

    if (0 == (regval & MAC_MDIO_ADDRESS_BUSY)) {
        regret = readl((u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_DATA);
        if(pval) *pval = regret;
        return regret;
    }
    return 0;
}


int ioctl_write_phy_reg(PADAPTER adapter, u16 reg, u16 val)
{
	u32 regval;
	u32 mdioctrl = reg * 0x10000 + 0x8000205;
	int busy = 15;

	writel(val, (u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_DATA);
	writel(mdioctrl, (u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_ADDRESS);
	do {
		regval = readl((u8 *)gFuxiMemBase + 0x2000 + MAC_MDIO_ADDRESS);
		busy--;
	}while((regval & MAC_MDIO_ADDRESS_BUSY) && (busy));
	
	return (regval & MAC_MDIO_ADDRESS_BUSY) ? -1 : 0; //-1 indicates err
}


bool ioctl_send_packets(PADAPTER adapter, u8 *buf, u32 size)
{
	struct ext_ioctl_data * pExt = (struct ext_ioctl_data *)buf;

	pExt->cmd_type = FUXI_DFS_IOCTL_DIAG_TX_PKT;
	pExt->cmd_buf.buf  = buf;
	pExt->cmd_buf.size_in = size;

	if (ioctl(adapter->debugfs_fd, FUXI_IOCTL_DFS_COMMAND, (void *)pExt) < 0) {
        	return false;
    	}
	return true;
}

bool ioctl_receive_packets(PADAPTER adapter, u8 *buf, u32 size)
{
	struct ext_ioctl_data ext;
	ext.cmd_type = FUXI_DFS_IOCTL_DIAG_RX_PKT;
	ext.cmd_buf.buf  = buf;
	ext.cmd_buf.size_in = size;
    ext.cmd_buf.size_out = 0xFFFFFFFF;
    
    if (ioctl(adapter->debugfs_fd, FUXI_IOCTL_DFS_COMMAND, (void *)&ext) < 0) {
        	return false;
    }
	
    return true;
}


bool ioctl_diag_start(PADAPTER adapter)
{
    struct ext_ioctl_data ext;

	ext.cmd_type = FUXI_DFS_IOCTL_DIAG_BEGIN;
	if (ioctl(adapter->debugfs_fd, FUXI_IOCTL_DFS_COMMAND, (void *)&ext) < 0) {
            printf("ioctl send debugfs_fd fail, debugfs_fd is %d, cmd is %x, errno is %s\n ", 
                adapter->debugfs_fd, FIONREAD,strerror(errno));
        	return false;
    	}

	return true;
}

bool ioctl_diag_end(PADAPTER adapter)
{
    struct ext_ioctl_data ext;

	ext.cmd_type = FUXI_DFS_IOCTL_DIAG_END;

	if (ioctl(adapter->debugfs_fd, FUXI_IOCTL_DFS_COMMAND, (void *)&ext) < 0) {
        	return false;
    	}
	
    return true;
}


bool ioctl_diag_device_reset(PADAPTER adapter)
{
	return true;
}


