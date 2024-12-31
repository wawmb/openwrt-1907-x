/* SPDX-License-Identifier: GPL-3.0+ */
/* Copyright (c) 2021 Motor-comm Corporation.
 * Confidential and Proprietary. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stdint.h>

#include "utility/CrossPlatformUtility.h"
#include "network/EthernetDefine.h"


#define FUXI_DEVICE_ID				0x6801
#define FUXI_VENDOR_ID				0x1F0A

#define FUXI_MAX_DEV_COUNT			16

#define CONFIG_FILE_NAME							_T("yt6801.cfg")
#define FORMAT_CONFIG_FILE_NAME						_T("yt6801_%d.cfg")


#define MAC_MDIO_ADDRESS						0x0200
#define MAC_MDIO_DATA							0x0204

#define PHY_EXT_REG_ADDR_PORT					0x1E
#define PHY_EXT_REG_DATA_PORT					0x1F

#define MACA0LR_FROM_EFUSE						0x1520 //mac[5]->bit7:0, mac[4]->bit15:8, mac[3]->bit23:16, mac[2]->bit31:24.
#define MACA0HR_FROM_EFUSE						0x1524 //mac[1]->bit7:0, mac[0]->bit15:8. mac[6] = {00, 01, 02, 03, 04, 05} 00-01-02-03-04-05.
#define EFUSE_SUBSYS_REGISTER					0x002C 

#define MGMT_PCIE_CFG_CTRL                      0x8BC
#define PCIE_CFG_CTRL_DEFAULT_VAL               0x7ff40

#define MGMT_PCIE_CFG_CTRL_CS_EN_POS            0
#define MGMT_PCIE_CFG_CTRL_CS_EN_LEN            1

#define EFUSE_REGION_A_BYTE_COUNT				7
#define EFUSE_REGION_B_BYTE_COUNT				11
#define EFUSE_REGION_AB_BYTE_COUNT				(EFUSE_REGION_A_BYTE_COUNT + EFUSE_REGION_B_BYTE_COUNT)

#define EFUSE_REGION_C_PATCH_ADDR_BYTE_COUNT	2
#define EFUSE_REGION_C_PATCH_VALUE_BYTE_COUNT	4
#define EFUSE_REGION_C_PATCH_BYTE_COUNT			(EFUSE_REGION_C_PATCH_ADDR_BYTE_COUNT + EFUSE_REGION_C_PATCH_VALUE_BYTE_COUNT)

#define EFUSE_REGION_C_PATCH_BYTE_COUNT			(EFUSE_REGION_C_PATCH_ADDR_BYTE_COUNT + EFUSE_REGION_C_PATCH_VALUE_BYTE_COUNT)
#define EFUSE_REGION_C_PATCH_COUNT				39
#define EFUSE_REGION_C_BYTE_COUNT				(EFUSE_REGION_C_PATCH_BYTE_COUNT * EFUSE_REGION_C_PATCH_COUNT)

//#define EFUSE_REGION_C_OOB_LED_MODE_COUNT		0x03
#define EFUSE_REGION_A_OOB_LED_MODE_COUNT		0x05	// from dirver v1.0.1.18

#define ADDR_EFUSE_PATCH_SUBSYS_ADDR			0x002C
#define ADDR_EFUSE_PATCH_REV_ADDR				0x0008

#define PCI_CFG_REV_REG_ADDR					0x08
#define PCI_CFG_SUBSYS_REG_ADDR					0x2C

#define MAC_ADDR_BYTE_COUNT						6

#define EFUSE_BYTE_COUNT						256

#define EFUSE_REGION_A_LED_REG_OFFSET			0x00
#define EFUSE_REGION_B_WOL_REG_OFFSET			0x07

#define EFUSE_LED_CONFIG_MODE_VALUE				0x1F

#define EFUSE_LED_CONFIG_PATCH_COUNT			15
#define EFUSE_MAX_COMMON_PATCH_COUNT			(EFUSE_REGION_C_PATCH_COUNT - EFUSE_LED_CONFIG_PATCH_COUNT)

struct PCIE_BDF
{
	uint8_t bus;
	uint8_t device;
	uint8_t function;
};


#ifdef _WIN32

#include <initguid.h>

DEFINE_GUID(GUID_DEVINTERFACE_FUXI_PCIE,
	0xe469e1c9, 0x531f, 0x4aef, 0x95, 0x69, 0xa3, 0xb7, 0xdf, 0xeb, 0xab, 0x70);

#define WIN_NETADAPTER_MIN_DRV_VER					_T("2.0.0.0")

#define	YT6801_REGISTRY_ROOT_KEY					_T("SYSTEM\\ControlSet001\\Control\\Class\\")
#define	YT6801_REGISTRY_NETWORK_ADDRESS				_T("NetworkAddress")


#else

#define	FORMAT_FUXI_DEBUG_DEVICE_PATH				"/sys/kernel/debug/fuxi_%u/fuxi-gmac/netdev_ops"
#define	FORMAT_FUXI_DEBUG_DEVICE_PATH_1				"/sys/kernel/debug/yt6801_%u/yt6801/netdev_ops"

#define FXGMAC_DEV_CMD								SIOCDEVPRIVATE + 1


#define FUXI_DFS_IOCTL_DEVICE_INACTIVE				0x10001
#define FUXI_DFS_IOCTL_DEVICE_RESET					0x10002
#define FUXI_DFS_IOCTL_DIAG_BEGIN					0x10003
#define FUXI_DFS_IOCTL_DIAG_END						0x10004
#define FUXI_DFS_IOCTL_DIAG_TX_PKT					0x10005
#define FUXI_DFS_IOCTL_DIAG_RX_PKT					0x10006

#define FXGMAC_EFUSE_UPDATE_LED_CFG                 0x10007
#define FXGMAC_EFUSE_WRITE_LED                      0x10008
#define FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX          0x1000A
#define FXGMAC_EFUSE_WRITE_OOB                      0x1000B
#define FXGMAC_EFUSE_READ_REGIONABC                 0x1000D
#define FXGMAC_EFUSE_READ_PATCH_PER_INDEX           0x1000F
#define FXGMAC_EFUSE_LED_TEST                       0x10010

#define FXGMAC_GET_MAC_DATA                         0x10011
#define FXGMAC_SET_MAC_DATA                         0x10012
#define FXGMAC_GET_SUBSYS_ID                        0x10013
#define FXGMAC_SET_SUBSYS_ID                        0x10014
#define FXGMAC_GET_GMAC_REG                         0x10015
#define FXGMAC_SET_GMAC_REG                         0x10016
#define FXGMAC_GET_PHY_REG                          0x10017
#define FXGMAC_SET_PHY_REG                          0x10018
#define FXGMAC_GET_PCIE_LOCATION                    0x1001B

#define FXGMAC_GET_GSO_SIZE                         0x1001C
#define FXGMAC_SET_GSO_SIZE                         0x1001D
#define FXGMAC_SET_RX_MODERATION                    0x1001E
#define FXGMAC_SET_TX_MODERATION                    0x1001F
#define FXGMAC_GET_TXRX_MODERATION                  0x10020



/****************************** IOCTL code ******************************/
#define FUXI_IOCTL_DFS_COMMAND _IOWR('M', 0x80, struct ext_ioctl_data)

struct ext_command_buf {
	void* buf;
	uint32_t size_in;
	uint32_t size_out;
};

#define FXGAMC_MAX_DATA_SIZE (1024*4+16)
struct per_regisiter_info
{
	unsigned int           size;
	unsigned int           address;
	unsigned int           value;
	unsigned char          data[FXGAMC_MAX_DATA_SIZE];
};

struct ext_ioctl_data {
	uint32_t	cmd_type;
	struct ext_command_buf cmd_buf;
};

typedef struct ext_command_data {
	uint32_t val0;
	uint32_t val1;
	uint32_t val2;
}CMD_DATA;

typedef struct pci_dev_info {
	unsigned int bus_no;
	unsigned int dev_no;
	unsigned int func_no;
}PCI_DEV_INFO;

#endif