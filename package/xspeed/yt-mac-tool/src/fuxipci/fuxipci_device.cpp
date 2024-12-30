/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxidevice.cpp

Abstract:

    FUXI Linux efuse function

Environment:

    User mode.

Revision History:
    2022-03-09    xiaogang.fan@motor-comm.com    created

--*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdbool.h>

#ifdef __x86
#include <sys/io.h>
#endif

#include "fuxipci_comm.h"
#include "fuxipci_device.h"

using namespace std;

void writel(u32 value, void *addr)
{
    *((unsigned int *) addr) = value;
}

u32 readl(void *addr)
{
    return(*((unsigned int *) addr));
}


u32 XLGMAC_SET_REG_BITS(u32 var, u32 pos, u32 len, u32 val)
{
    u32 _var = (var);
    u32 _pos = (pos);
    u32 _len = (len);
    u32 _val = (val);
    _val = (_val << _pos) & GENMASK(_pos + _len - 1, _pos);
    _var = (_var & ~(GENMASK(_pos + _len - 1, _pos))) | _val;

    return _var;
}

u32 XLGMAC_GET_REG_BITS(u32 var, u32 pos, u32 len)
{
    u32 _pos = (pos);
    u32 _len = (len);
    u32 v = ((var) & GENMASK(_pos + _len - 1, _pos)) >> (_pos);

    return v;
}

void efuse_load(void *map_base, u32 target)
{
    bool succeed = false;
    unsigned int wait;
    u32 reg_val = 0;
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_AUTO_LOAD);
    writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

    wait = 1000;
    while (wait--) {
        usleep(20);
        reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
        if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
            succeed = true;
            break;
        }
    }
    if (!succeed) {
        printf("Fail to loading efuse, ctrl_1 0x%08x\n", reg_val);
    } else {
        printf("Success to loading efuse, ctrl_1 0x%08x\n", reg_val);
    }
    fflush(stdout);
}

bool read_patch_from_efuse_per_index(void *map_base, u32 target, u8 index, u32* reg, u32* value) /* read patch per index. */
{
    unsigned int wait, i;
    u32 regval = 0;
    bool succeed = false;

    if (index >= FUXI_EFUSE_MAX_ENTRY) {
        printf("Reading efuse out of range, index %d\n", index);
        fflush(stdout);
        return false;
    }

    if (reg) {
        *reg = 0;
    }
    for (i = EFUSE_PATCH_ADDR_START_BYTE; i < EFUSE_PATCH_DATA_START_BYTE; i++) {
        regval = 0;
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_READ);
        writel(regval, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));
        wait = 1000;
        while (wait--) {
            usleep(20);
            regval = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(regval, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (succeed) {
            if (reg) {
                *reg |= (XLGMAC_GET_REG_BITS(regval, EFUSE_OP_RD_DATA_POS, EFUSE_OP_RD_DATA_LEN) << (i << 3));
            }
        }
        else {
            printf("Fail to reading efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return succeed;
        }
    }

    if (value) {
        *value = 0;
    }
    for (i = EFUSE_PATCH_DATA_START_BYTE; i < EFUSE_EACH_PATH_SIZE; i++) {
        regval = 0;
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        regval = XLGMAC_SET_REG_BITS(regval, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_READ);
        writel(regval, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));
        wait = 1000;
        while (wait--) {
            usleep(20);
            regval = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(regval, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (succeed) {
            if (value) {
                *value |= (XLGMAC_GET_REG_BITS(regval, EFUSE_OP_RD_DATA_POS, EFUSE_OP_RD_DATA_LEN) << ((i - 2) << 3));
            }
        }
        else {
            printf("Fail to reading efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return succeed;
        }
    }

    return succeed;
}

void read_patch_from_efuse(void *map_base, u32 target, u32 reg) /* read patch per index. */
{
    u32 cur_reg, reg_val;
    u32 cur_val = 0;
    bool find_flag = false;
    
    if(reg >> 16){
        printf("Reading efuse out of range, reg %x. reg must be 2 bytes.\n", reg);
        fflush(stdout);
        return;
    }

    for (u8 index = 0; index < FUXI_EFUSE_MAX_ENTRY; index++) {
        if (!read_patch_from_efuse_per_index(map_base, target, index, &cur_reg, &reg_val)) {
            printf("Fail to reading efuse, address is  0x%X\n", cur_reg);
            fflush(stdout);
            break;
        } else if (cur_reg == reg) {
            cur_val = reg_val;
            find_flag = true;
        } else if (0 == cur_reg && 0 == reg_val) {
            break; // first blank. We should write here.
        }
    }

    if(find_flag){
        printf("Value at address 0x%X : 0x%X\n", reg, cur_val);
    }else{
        printf("The efuse has not register 0x%X.\n", reg);
    }
    fflush(stdout);
}

void write_patch_to_efuse(void *map_base, u32 target, u32 reg, u32 value)
{
    unsigned int wait, i;
    u32 tmp_reg, reg_val;
    u32 cur_reg = 0, cur_val = 0;
    bool succeed = false;
    u8 index = 0;

    if(reg >> 16){
        printf("Reading efuse out of range, reg %x. reg must be 2 bytes.\n", reg);
        fflush(stdout);
        return;
    }
    
    for (index = 0; ; index++) {
        if (!read_patch_from_efuse_per_index(map_base, target, index, &tmp_reg, &reg_val)) {
            printf("Fail to reading efuse, address is  0x%X\n", tmp_reg);
            fflush(stdout);
            return;
        } else if (tmp_reg == reg) {
            cur_reg = tmp_reg;
            cur_val = reg_val;
        } else if (0 == tmp_reg && 0 == reg_val) {
            break; // first blank. We should write here.
        }
    }

    if (cur_reg == reg) {
        if (cur_val == value) {
            printf("0x%x -> Reg 0x%x already exists, ignore.\n", value, reg);
            fflush(stdout);
            return;
        } else {
            printf("Reg 0x%x entry current value 0x%x, reprogram value 0x%x.\n", reg, cur_val, value);
            fflush(stdout);
        }
    }else{
        printf("Reg 0x%x entry does not exist, program value 0x%x.\n", reg, value);
        fflush(stdout);
    }

    for (i = EFUSE_PATCH_ADDR_START_BYTE; i < EFUSE_PATCH_DATA_START_BYTE; i++) {
        reg_val = 0;
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, (reg >> (i << 3)) & 0xFF );
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
        writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

        succeed = false;
        wait = 1000;
        while (wait--) {
            usleep(20);
            reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (!succeed) {
            printf("Fail to writing efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return;
        }
    }

    for (i = EFUSE_PATCH_DATA_START_BYTE; i < EFUSE_EACH_PATH_SIZE; i++) {
        reg_val = 0;
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, (value >> ((i - 2) << 3)) & 0xFF);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
        writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

        succeed = false;
        wait = 1000;
        while (wait--) {
            usleep(20);
            reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (!succeed) {
            printf("Fail to writing efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return;
        }
    }
}

void write_patch_to_efuse_per_index(void *map_base, u32 target, u8 index, u32 reg, u32 value)
{
    unsigned int    wait, i;
    u32             reg_val;
    bool            succeed = false;

    if(reg >> 16){
        printf("Reading efuse out of range, reg %x. reg must be 2 bytes.\n", reg);
        fflush(stdout);
        return;
    }

    if (index >= FUXI_EFUSE_MAX_ENTRY) {
        printf("Writing efuse out of range, index %d\n", index);
        fflush(stdout);
        return;
    }
    for (i = EFUSE_PATCH_ADDR_START_BYTE; i < EFUSE_PATCH_DATA_START_BYTE; i++) {
        reg_val = 0;
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, (reg >> (i << 3)) & 0xFF);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
        writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));
        succeed = false;
        wait = 1000;
        while (wait--) {
            usleep(20);
            reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (!succeed) {
            printf("Fail to writing efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return;
        }
    }

    for (i = EFUSE_PATCH_DATA_START_BYTE; i < EFUSE_EACH_PATH_SIZE; i++) {
        reg_val = 0;
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, EFUSE_REGION_A_B_LENGTH + index * EFUSE_EACH_PATH_SIZE + i);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, (value >> ((i - 2) << 3)) & 0xFF);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
        reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
        writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

        succeed = false;
        wait = 1000;
        while (wait--) {
            usleep(20);
            reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
            if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
                succeed = true;
                break;
            }
        }
        if (!succeed) {
            printf("Fail to writing efuse Byte%d\n", index * EFUSE_EACH_PATH_SIZE + i);
            fflush(stdout);
            return;
        }
    }
}

void read_mac_addr_from_efuse(void *map_base, u32 target)
{
    u32 reg = 0, value = 0;
    u32 machr = 0, maclr = 0;
    for (u8 index = 0; ; index++) {
        if (!read_patch_from_efuse_per_index(map_base, target, index, &reg, &value)) {
            break; // reach the last item.
        }
        if (0x00 == reg) {
            break; // reach the blank.
        }
        if (MACA0LR_FROM_EFUSE == reg) {
            maclr = value;
        }
        if (MACA0HR_FROM_EFUSE == reg) {
            machr = value;
        }
    }

    printf("Current mac address from efuse is %02x:%02x:%02x:%02x:%02x:%02x.\n", 
            (machr >> 8) & 0xFF, machr & 0xFF, (maclr >> 24) & 0xFF, (maclr >> 16) & 0xFF, (maclr >> 8) & 0xFF, maclr & 0xFF);
    //maclr & 0xFF, (maclr >> 8) & 0xFF, (maclr >> 16) & 0xFF, (maclr >> 24) & 0xFF,
            //machr & 0xFF, (machr >> 8) & 0xFF);
    fflush(stdout);
}

void write_mac_addr_to_efuse(void *map_base, u32 target, u8* mac_addr)
{
    if (mac_addr) {
        read_mac_addr_from_efuse(map_base, target);

        write_patch_to_efuse(map_base, target, MACA0HR_FROM_EFUSE, (((u32)mac_addr[5]) << 8) | mac_addr[4]);
        write_patch_to_efuse(map_base, target, MACA0LR_FROM_EFUSE, (((u32)mac_addr[3]) << 24) | (((u32)mac_addr[2]) << 16) | (((u32)mac_addr[1]) << 8) | mac_addr[0]);
    }
}


void read_subsys_from_efuse(void *map_base, u32 target)
{
    u32 offset = 0, value = 0, tmpValue = 0;
    u16 subsys = 0, revid = 0;
    bool succeed = true;
    u8 index = 0;

    for (index = 0; ; index++) {
        if (true != read_patch_from_efuse_per_index(map_base, target, index, &offset, &value)) {
            succeed = false;
            break; // reach the last item.
        }

        if (0x00 == offset) {
            break; // reach the blank.
        }
        if (EFUSE_SUBSYS_REGISTER == offset) {
            tmpValue = value;
        }
    }
    if (tmpValue) {
        subsys = 0xFFFF & (tmpValue >> 16);
        revid = 0xFFFF & tmpValue;
    }
    printf("read subsys info: subsys is %X, subven id is %X\n", subsys, revid);
}

bool write_mac_subsys_to_efuse(void *map_base, u32 target, u8* mac_addr, u32* subsys, u32* revid)
{
    //u32 machr = 0, maclr = 0,
    u32 pcie_cfg_ctrl= 0x7ff40;
    bool succeed = true;
    if (mac_addr) {
        //machr = readreg(pdata->pAdapter, pdata->base_mem + MACA0HR_FROM_EFUSE);
        //maclr = readreg(pdata->pAdapter, pdata->base_mem + MACA0LR_FROM_EFUSE);
        //FXGMAC_PR("Current mac address from efuse is %02x-%02x-%02x-%02x-%02x-%02x.\n", 
        //    (machr >> 8) & 0xFF, machr & 0xFF, (maclr >> 24) & 0xFF, (maclr >> 16) & 0xFF, (maclr >> 8) & 0xFF, maclr & 0xFF);

        write_patch_to_efuse(map_base, target, MACA0HR_FROM_EFUSE, (((u32)mac_addr[0]) << 8) | mac_addr[1]);
        write_patch_to_efuse(map_base, target, MACA0LR_FROM_EFUSE, (((u32)mac_addr[2]) << 24) | (((u32)mac_addr[3]) << 16) | (((u32)mac_addr[4]) << 8) | mac_addr[5]);
    }

    if (revid) {
        write_patch_to_efuse(map_base, target, EFUSE_REVID_REGISTER, *revid);
    }
    
    if (subsys) {
        pcie_cfg_ctrl = XLGMAC_SET_REG_BITS(pcie_cfg_ctrl, 0, 1, 1);
        write_patch_to_efuse(map_base, target, 0x8BC, pcie_cfg_ctrl);
        write_patch_to_efuse(map_base, target, EFUSE_SUBSYS_REGISTER, *subsys);

        pcie_cfg_ctrl = XLGMAC_SET_REG_BITS(pcie_cfg_ctrl, 0, 1, 0);
        write_patch_to_efuse(map_base, target, 0x8BC, pcie_cfg_ctrl);
    }
    return succeed;
}


void write_subsys_info_to_efuse(void *map_base, u32 target, u32 subsys, u32 revid)
{
    u32 value = 0;
    bool succeed = true;

    /* merge subsys and revied */
    value = (((subsys << 16) & 0xFFFF0000) | (revid & 0xFFFF));

    if(write_mac_subsys_to_efuse(map_base, target, NULL, &value, NULL))
    {
        printf("write subsys info successful\n");
    }
    else
    {
        succeed = false;
    }
}


void read_gmac_register(void *map_base, u32 target, u32 reg) /* read gmac register. */
{
    u32 reg_val;
    
    if(reg >> 16){
        printf("Reading gmac register out of range, reg %x. reg must be 2 bytes.\n", reg);
        fflush(stdout);
        return;
    }
    //printf("map_base is %p, target is %x, reg is %x, MAP_MASK is %lx \n", map_base, target, reg, MAP_MASK);
    reg_val = readl((unsigned char *)map_base + ((target + reg) & MAP_MASK));
    printf("Reading gmac register, reg 0x%x has value 0x%x.\n", reg, reg_val);
    fflush(stdout);
}

void write_gmac_register(void *map_base, u32 target, u32 reg, u32 value)/* write gmac register. */
{
    if(reg >> 16){
        printf("Writing gmac register out of range, reg %x. reg must be 2 bytes.\n", reg);
        fflush(stdout);
        return;
    }

    read_gmac_register(map_base, target, reg);
    writel(value, (unsigned char *)map_base + ((target + reg) & MAP_MASK));
    read_gmac_register(map_base, target, reg);
}

void read_gmac_config_register(void *map_base, u32 target, u32 reg) /* read gmac register. */
{
#ifdef __x86
    u32 cfg_reg = 0;
    u32 reg_val = 0;
    int ret = 0;
    /* The unit is word of reg */
    if(reg >> 6){
        printf("Reading gmac config register out of range, reg %x. reg must be 6 bits.\n", reg);
        fflush(stdout);
        return;
    }
    ret = iopl(3);
    if(ret < 0){
        printf("iopl set to high level error\n");
        return;
    }
    cfg_reg = 0;
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_POS, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_LEN, FUXI_PCI_CONFIG_ADDR_OP_ENABLED_DATA_REG);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_BUS_POS, FUXI_PCI_CONFIG_ADDR_OP_BUS_LEN, gFuxiPciBus);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_DEV_POS, FUXI_PCI_CONFIG_ADDR_OP_DEV_LEN, gFuxiPciDev);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_FUNC_POS, FUXI_PCI_CONFIG_ADDR_OP_FUNC_LEN, gFuxiPciFunc);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_REG_POS, FUXI_PCI_CONFIG_ADDR_OP_REG_LEN, reg);
    
    outl(cfg_reg, FUXI_PCI_CONFIG_ADDR_REGISTER);
    reg_val = inl(FUXI_PCI_CONFIG_DATA_REGISTER);
    
    printf("Reading gmac config register, reg %x. the value is %x.\n", reg, reg_val);
    fflush(stdout);

    iopl(0);
    if(ret<0){
        printf("iopl set to low level error\n");
	}
#endif
}

void write_gmac_config_register(void *map_base, u32 target, u32 reg, u32 value)/* write gmac register. */
{
#ifdef __x86
    int ret;
    u32 cfg_reg;
    if(reg >> 6){
        printf("Reading gmac config register out of range, reg %x. reg must be 6 bits.\n", reg);
        fflush(stdout);
        return;
    }

    read_gmac_config_register(map_base, target, reg);
    
    ret = iopl(3);
    if(ret < 0){
        printf("iopl set to high level error\n");
        return;
    }

    cfg_reg = 0;
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_POS, FUXI_PCI_CONFIG_ADDR_OP_ENABLE_LEN, FUXI_PCI_CONFIG_ADDR_OP_ENABLED_DATA_REG);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_BUS_POS, FUXI_PCI_CONFIG_ADDR_OP_BUS_LEN, gFuxiPciBus);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_DEV_POS, FUXI_PCI_CONFIG_ADDR_OP_DEV_LEN, gFuxiPciDev);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_FUNC_POS, FUXI_PCI_CONFIG_ADDR_OP_FUNC_LEN, gFuxiPciFunc);
    cfg_reg = XLGMAC_SET_REG_BITS(cfg_reg, FUXI_PCI_CONFIG_ADDR_OP_REG_POS, FUXI_PCI_CONFIG_ADDR_OP_REG_LEN, reg);

    outl(cfg_reg, FUXI_PCI_CONFIG_ADDR_REGISTER);
    outl(value, FUXI_PCI_CONFIG_DATA_REGISTER);

    iopl(0);
    if(ret<0){
        printf("iopl set to low level error\n");
    }
#endif
    read_gmac_config_register(map_base, target, reg);
}

bool efuse_read_regionA_regionB(void *map_base, u32 target, u32 reg, u32* value)
{
    bool succeed = false;
    unsigned int wait;
    u32 reg_val = 0;

    if (reg >= EFUSE_REGION_A_B_LENGTH) {
        printf("Read addr out of range %d", reg);
        return succeed;
    }

    if (value) {
        *value = 0;
    }

    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, reg);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_READ);
    writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));
    wait = 1000;
    while (wait--) {
        usleep(20);
        reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
        if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
            succeed = true;
            break;
        }
    }

    if (succeed) {
        if (value) {
            *value = XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_RD_DATA_POS, EFUSE_OP_RD_DATA_LEN);
        }
    }
    else {
        printf("Fail to reading efuse Byte%d\n", reg);
    }

    return succeed;
}

bool efuse_write_led(void *map_base, u32 target, u32 value)
{
    bool succeed = false;
    unsigned int wait;
    u32 reg_val;

    if (!efuse_read_regionA_regionB(map_base, target, 0x00, &reg_val)) {
        return succeed;
    }

    if (reg_val == value) {
        printf("Led Ctrl option already exists");
        return true;
    }

    reg_val = 0;
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, 0x00);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, value & 0xFF);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
    writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

    wait = 1000;
    while (wait--) {
        usleep(20);
        reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
        if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
            succeed = true;
            break;
        }
    }

    if (!succeed) {
        printf("Fail to writing efuse Byte LED");
    }

    return succeed;
}

bool efuse_write_oob(void *map_base, u32 target)
{
    bool succeed = false;
    unsigned int wait;
    u32 reg_val, value;

    if (!efuse_read_regionA_regionB(map_base, target, 0x07, &reg_val)) {
        return succeed;
    }

    if (XLGMAC_GET_REG_BITS(reg_val, 2, 1)) {
        printf("OOB Ctrl bit already exists");
        return true;
    }

    value = 0;
    value = XLGMAC_SET_REG_BITS(value, 2, 1, 1);

    reg_val = 0;
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_ADDR_POS, EFUSE_OP_ADDR_LEN, 0x07);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_WR_DATA_POS, EFUSE_OP_WR_DATA_LEN, value & 0xFF);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_START_POS, EFUSE_OP_START_LEN, 1);
    reg_val = XLGMAC_SET_REG_BITS(reg_val, EFUSE_OP_MODE_POS, EFUSE_OP_MODE_LEN, EFUSE_OP_MODE_ROW_WRITE);
    writel(reg_val, (unsigned char *)map_base + ((target + EFUSE_OP_CTRL_0) & MAP_MASK));

    wait = 1000;
    while (wait--) {
        usleep(20);
        reg_val = readl((unsigned char *)map_base + ((target + EFUSE_OP_CTRL_1) & MAP_MASK));
        if (XLGMAC_GET_REG_BITS(reg_val, EFUSE_OP_DONE_POS, EFUSE_OP_DONE_LEN)) {
            succeed = true;
            break;
        }
    }

    if (!succeed) {
        printf("Fail to writing efuse Byte OOB");
    }

    return succeed;
}


