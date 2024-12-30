/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_device.h

Abstract:

    FUXI device head file.

Environment:

            User mode.

Revision History:
                2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __FUXIDEVICE_H
#define __FUXIDEVICE_H

#define MAP_SIZE                65536UL
#define MAP_MASK                (MAP_SIZE - 1)
#define FUXI_EFUSE_DEBUG        0

#define FUXI_EFUSE_MAX_ENTRY    39

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)


/*******************************************************************
        efuse entry. val31:0 -> offset15:0
        offset7:0
        offset15:8
        val7:0
        val15:8
        val23:16
        val31:24
*******************************************************************/
#define     EFUSE_OP_CTRL_0                 0x1500
#define     EFUSE_OP_WR_DATA_POS            16
#define     EFUSE_OP_WR_DATA_LEN            8
#define     EFUSE_OP_ADDR_POS               8
#define     EFUSE_OP_ADDR_LEN               8
#define     EFUSE_OP_START_POS              2
#define     EFUSE_OP_START_LEN              1
#define     EFUSE_OP_MODE_POS               0
#define     EFUSE_OP_MODE_LEN               2
#define     EFUSE_OP_MODE_ROW_WRITE         0x0
#define     EFUSE_OP_MODE_ROW_READ          0x1
#define     EFUSE_OP_MODE_AUTO_LOAD         0x2
#define     EFUSE_OP_MODE_READ_BLANK        0x3

#define     EFUSE_OP_CTRL_1                 0x1504
#define     EFUSE_OP_RD_DATA_POS            24
#define     EFUSE_OP_RD_DATA_LEN            8
#define     EFUSE_OP_BIST_ERR_ADDR_POS      16
#define     EFUSE_OP_BIST_ERR_ADDR_LEN      8
#define     EFUSE_OP_BIST_ERR_CNT_POS       8
#define     EFUSE_OP_BIST_ERR_CNT_LEN       8
#define     EFUSE_OP_PGM_PASS_POS           2
#define     EFUSE_OP_PGM_PASS_LEN           1
#define     EFUSE_OP_DONE_POS               1
#define     EFUSE_OP_DONE_LEN               1


#define     EFUSE_PATCH_ADDR_START_BYTE     0
#define     EFUSE_PATCH_DATA_START_BYTE     2
#define     EFUSE_REGION_A_B_LENGTH         18
#define     EFUSE_EACH_PATH_SIZE            6

void efuse_load(void *map_base, u32 target);
bool read_patch_from_efuse_per_index(void *map_base, u32 target, u8 index, u32* reg, u32* value);
void read_patch_from_efuse(void *map_base, u32 target, u32 reg);
void write_patch_to_efuse(void *map_base, u32 target, u32 reg, u32 value);
void write_patch_to_efuse_per_index(void *map_base, u32 target, u8 index, u32 reg, u32 value);
void read_mac_addr_from_efuse(void *map_base, u32 target);
void write_mac_addr_to_efuse(void *map_base, u32 target, u8* mac_addr);
void read_subsys_from_efuse(void *map_base, u32 target);
void write_subsys_info_to_efuse(void *map_base, u32 target, u32 subsys, u32 revid);
void read_gmac_register(void *map_base, u32 target, u32 reg); 
void write_gmac_register(void *map_base, u32 target, u32 reg, u32 value);
void read_gmac_config_register(void *map_base, u32 target, u32 reg); 
void write_gmac_config_register(void *map_base, u32 target, u32 reg, u32 value);
bool efuse_read_regionA_regionB(void *map_base, u32 target, u32 reg, u32* value);
bool efuse_write_led(void *map_base, u32 target, u32 value);
bool efuse_write_oob(void *map_base, u32 target);

#endif//__FUXIDEVICE_H
