/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_comm.h

Abstract:
    
    FUXI common define

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __FUXIPCI_COMM_H
#define __FUXIPCI_COMM_H

typedef     void *              PVOID;
typedef     unsigned char       u8;
typedef     unsigned short      u16;
typedef     unsigned int        u32;

// to illustrate the IN/OUT of the parameters
#define IN
#define OUT

#define FXG_DBG

#define GENMASK(h, l) \
    (((~0U) - (1U << (l)) + 1) & (~0U >> (BITS_PER_LONG - 1 - (h))))

#define BITS_PER_LONG           32

#define     MACA0LR_FROM_EFUSE              0x1520 //mac[0]->bit7:0, mac[1]->bit15:8, mac[2]->bit23:16, mac[3]->bit31:24.
#define     MACA0HR_FROM_EFUSE              0x1524 //mac[4]->bit7:0, mac[5]->bit15:8
#define     EFUSE_REVID_REGISTER            0x0008 
#define     EFUSE_SUBSYS_REGISTER           0x002C 

#define     MAC_MDIO_ADDRESS                0x0200
#define     MAC_MDIO_DATA                   0x0204

/*******************************************************************
        pci config entry. val31:0 -> offset15:0
        offset7:0
        offset15:8
        val7:0
        val15:8
        val23:16
        val31:24
*******************************************************************/
#define     FUXI_PCI_CONFIG_BASE_ADDR                   0x80000000
#define     FUXI_PCI_CONFIG_ADDR_REGISTER               0xCF8
#define     FUXI_PCI_CONFIG_ADDR_OP_ENABLE_POS          31
#define     FUXI_PCI_CONFIG_ADDR_OP_ENABLE_LEN          1
#define     FUXI_PCI_CONFIG_ADDR_OP_BUS_POS             16
#define     FUXI_PCI_CONFIG_ADDR_OP_BUS_LEN             8
#define     FUXI_PCI_CONFIG_ADDR_OP_DEV_POS             11
#define     FUXI_PCI_CONFIG_ADDR_OP_DEV_LEN             5
#define     FUXI_PCI_CONFIG_ADDR_OP_FUNC_POS            8
#define     FUXI_PCI_CONFIG_ADDR_OP_FUNC_LEN            3
#define     FUXI_PCI_CONFIG_ADDR_OP_REG_POS             2
#define     FUXI_PCI_CONFIG_ADDR_OP_REG_LEN             6

#define     FUXI_PCI_CONFIG_DATA_REGISTER               0xCFC
#define     FUXI_PCI_CONFIG_ADDR_OP_ENABLED_DATA_REG    0x1


#define DBG_PRINT(x) printf x
#define WAIT_KEY() athr_getch()

#define MAP_SIZE                65536UL
#define MAP_MASK                (MAP_SIZE - 1)

extern void * gFuxiMemBase;
extern u32 gTarget;
extern u32 gFuxiPciBus;
extern u32 gFuxiPciDev;
extern u32 gFuxiPciFunc;
extern char gFuxiDfPath[50];
extern u32 gFuxiLpPacketsNum;


void writel(u32 value, void *addr);
u32 readl(void *addr);
u32 XLGMAC_SET_REG_BITS(u32 var, u32 pos, u32 len, u32 val);
u32 XLGMAC_GET_REG_BITS(u32 var, u32 pos, u32 len);


#endif
