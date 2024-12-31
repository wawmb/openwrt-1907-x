// SPDX-License-Identifier: GPL-3.0+
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

#include "FuxiDriverInterface_Linux.h"

#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>

#include "fuxi/FuxiDefine.h"
#include "reg/RegisterHelper.h"


FuxiDriverInterface_Linux::FuxiDriverInterface_Linux()
	: FuxiDriverInterface()
	, m_fdFuxi(-1)
{

}

FuxiDriverInterface_Linux::~FuxiDriverInterface_Linux()
{
	close();
}

int FuxiDriverInterface_Linux::open(uint8_t index)
{
	int ret = 0;

	close();

	char szDevicePath[256] = { 0 };
	sprintf(szDevicePath, FORMAT_FUXI_DEBUG_DEVICE_PATH, index);

	if ((m_fdFuxi = ::open(szDevicePath, O_RDWR)) < 0)
	{
		//perror("Open FUXI device failed!");
		
		// retry another path
		memset(szDevicePath, 0, sizeof(szDevicePath));
		sprintf(szDevicePath, FORMAT_FUXI_DEBUG_DEVICE_PATH_1, index);

		if ((m_fdFuxi = ::open(szDevicePath, O_RDWR)) < 0)
		{
			printf("open debugfs for index %u failed!\n", index);
			return -1;
		}
			
	}

	return ret;
}

bool FuxiDriverInterface_Linux::isOpen()
{
	return (m_fdFuxi >= 0);
}

void FuxiDriverInterface_Linux::close()
{
	if (m_fdFuxi >= 0)
		::close(m_fdFuxi);
}

int FuxiDriverInterface_Linux::getHandle(void* handle)
{
	if (m_fdFuxi >= 0)
	{
		*((int*)handle) = m_fdFuxi;
		return 0;
	}
		
	return 1;
}

int FuxiDriverInterface_Linux::readEfuseReg(uint32_t addr, uint32_t& value)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_READ_REGIONABC;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->val0 = addr;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_READ_REGIONABC ioctl failed!");
		ret = -1;
	}
	else
		value = pCommandData->val1;

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::readEfusePatch(uint8_t index, uint32_t& addr, uint32_t& value)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(per_regisiter_info);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_READ_PATCH_PER_INDEX;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct per_regisiter_info* pCommandData = (per_regisiter_info*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->data[0] = index;
	
	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_READ_PATCH_PER_INDEX ioctl failed!");
		ret = -1;
	}
	else
	{
		addr = pCommandData->address;
		value = pCommandData->value;
	}

	if (szBuf)
		delete[] szBuf;
	
/*
	struct ext_ioctl_data data = { 0 };
	data.cmd_type = FXGMAC_EFUSE_READ_PATCH_PER_INDEX;
	struct per_regisiter_info pCommandData = {0};
	pCommandData.data[0] = index;
	data.cmd_buf.buf = (void*)&pCommandData;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(&data)) < 0)
	{
		perror("FXGMAC_EFUSE_READ_PATCH_PER_INDEX ioctl failed!");
		ret = -1;
	}
	else
	{
		struct per_regisiter_info* pCommandBuf = (struct per_regisiter_info*)data.cmd_buf.buf;
		addr = pCommandBuf->address;
		value = pCommandBuf->value;
	}*/

	return ret;
}

int FuxiDriverInterface_Linux::writeEfusePatch(uint8_t index, uint32_t addr, uint32_t value)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(per_regisiter_info);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct per_regisiter_info* pCommandData = (per_regisiter_info*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->data[0] = index;
	pCommandData->address = addr;
	pCommandData->value = value;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::readMACAddressfromEFuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	int ret = 0;

	uint32_t uMacAddressLow = 0;
	if (readEfusePatch(MACA0LR_FROM_EFUSE, uMacAddressLow) == 0)
	{
		macAddr[5] = (uint8_t)GET_REG_BITS(uMacAddressLow, 0, 8);
		macAddr[4] = (uint8_t)GET_REG_BITS(uMacAddressLow, 8, 8);
		macAddr[3] = (uint8_t)GET_REG_BITS(uMacAddressLow, 0x10, 8);
		macAddr[2] = (uint8_t)GET_REG_BITS(uMacAddressLow, 0x18, 8);
	}

	uint32_t uMacAddressHigh = 0;
	if (readEfusePatch(MACA0HR_FROM_EFUSE, uMacAddressHigh) == 0)
	{
		macAddr[1] = (uint8_t)GET_REG_BITS(uMacAddressHigh, 0, 8);
		macAddr[0] = (uint8_t)GET_REG_BITS(uMacAddressHigh, 8, 8);
	}

	return ret;
}

int FuxiDriverInterface_Linux::writeMACAddresstoEfuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	int ret = 0;

	uint32_t uLowPart = 0;
	uint32_t uHighPart = 0;

	uLowPart = SET_REG_BITS(uLowPart, 0, 8, macAddr[5]);
	uLowPart = SET_REG_BITS(uLowPart, 8, 8, macAddr[4]);
	uLowPart = SET_REG_BITS(uLowPart, 0x10, 8, macAddr[3]);
	uLowPart = SET_REG_BITS(uLowPart, 0x18, 8, macAddr[2]);
	uHighPart = SET_REG_BITS(uHighPart, 0, 8, macAddr[1]);
	uHighPart = SET_REG_BITS(uHighPart, 8, 8, macAddr[0]);

	bool bWriteLowPart = true, bWriteHighPart = true;

	uint32_t uCurrentLowPart = 0;
	if (readEfusePatch(MACA0LR_FROM_EFUSE, uCurrentLowPart) == 0)
	{
		if (uLowPart == uCurrentLowPart)
			bWriteLowPart = false;
	}

	uint32_t uCurrentHighPart = 0;
	if (readEfusePatch(MACA0HR_FROM_EFUSE, uCurrentHighPart) == 0)
	{
		if (uHighPart == uCurrentHighPart)
			bWriteHighPart = false;
	}

	uint8_t uIndex = 0;
	if (getFirstAvailablePatchIndex(uIndex) != 0)
		return 1;

	if (bWriteLowPart && bWriteHighPart && ((EFUSE_MAX_COMMON_PATCH_COUNT - uIndex) < 2))
		return 1;

	if ((ret = writeEfusePatch(uIndex++, MACA0LR_FROM_EFUSE, uLowPart)) != 0)
		return ret;

	if ((ret = writeEfusePatch(uIndex, MACA0HR_FROM_EFUSE, uHighPart)) != 0)
		return ret;

	return ret;
}

int FuxiDriverInterface_Linux::readSubSystemIDfromEFuse(uint32_t& subDeviceID, uint32_t& subVendorID, uint32_t& revisionID)
{
	int ret = 0;

	revisionID = 0;

	uint32_t uSubsystemID = 0;
	if (readEfusePatch(EFUSE_SUBSYS_REGISTER, uSubsystemID) != 0)
	{
		subVendorID = 0;
		subDeviceID = 0;
	}
	else
	{
		subVendorID = GET_REG_BITS(uSubsystemID, 0, 16);
		subDeviceID = GET_REG_BITS(uSubsystemID, 0x10, 16);
	}

	return ret;
}

int FuxiDriverInterface_Linux::writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID)
{
	int ret = 0;

	uint32_t uSubsystemID = 0;
	uSubsystemID = SET_REG_BITS(uSubsystemID, 0, 16, subVendorID);
	uSubsystemID = SET_REG_BITS(uSubsystemID, 0x10, 16, subDeviceID);

	uint32_t uCurrentSubsystemID = 0;
	if (readEfusePatch(EFUSE_SUBSYS_REGISTER, uCurrentSubsystemID) != 0)
	{
		if (uSubsystemID == uCurrentSubsystemID)
			return 0;
	}

	uint8_t uIndex = 0;
	if (getFirstAvailablePatchIndex(uIndex) != 0)
		return 1;

	uint8_t uAvailableCount = (EFUSE_REGION_C_PATCH_COUNT - EFUSE_LED_CONFIG_PATCH_COUNT) - uIndex;
	if (uAvailableCount < 3)
		return 1;

	uint32_t uControlValue = PCIE_CFG_CTRL_DEFAULT_VAL;
	uControlValue = SET_REG_BITS(uControlValue, MGMT_PCIE_CFG_CTRL_CS_EN_POS, MGMT_PCIE_CFG_CTRL_CS_EN_LEN, 1);
	if ((ret = writeEfusePatch(uIndex++, MGMT_PCIE_CFG_CTRL, uControlValue)) != 0)
		return ret;

	if ((ret = writeEfusePatch(uIndex++, EFUSE_SUBSYS_REGISTER, uSubsystemID)) != 0)
		return ret;

	uControlValue = SET_REG_BITS(uControlValue, MGMT_PCIE_CFG_CTRL_CS_EN_POS, MGMT_PCIE_CFG_CTRL_CS_EN_LEN, 0);
	if ((ret = writeEfusePatch(uIndex, MGMT_PCIE_CFG_CTRL, uControlValue)) != 0)
		return ret;

	return ret;
}

//int FuxiDriverInterface::getLEDSolutionfromEfuse(uint8_t& solution)
//{
//	int ret = 0;
//
//	return ret;
//}

int FuxiDriverInterface_Linux::writeLEDStatustoEFuse(uint8_t solution)
{
	int ret = 0;

	//ext_ioctl_data data = { 0 };

	//data.cmd_type = FXGMAC_EFUSE_WRITE_LED;

	////ext_command_data commandData = { 0 };
	////commandData.val0 = solution;

	////data.cmd_buf.buf = (void*)&commandData;

	//ext_command_data* pExt_command_data = new ext_command_data();
	//memset(pExt_command_data, 0, sizeof(ext_command_data));
	//pExt_command_data->val0 = solution;

	//data.cmd_buf.buf = pExt_command_data;

	//// this is total size
	//data.cmd_buf.size_in = sizeof(ext_command_data);
	//data.cmd_buf.size_out = 0xFFFFFFFF;

	//if ((ret = ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(&data))) < 0)
	//{
	//	perror("FUXI_IOCTL_DFS_COMMAND ioctl failed!");
	//	return ret;
	//}

	//if (pExt_command_data)
	//	delete pExt_command_data;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_WRITE_LED;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->val0 = solution;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_WRITE_LED ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::simulateLEDSetting(const led_setting& ledSetting)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(led_setting);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_LED_TEST;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct led_setting* pLEDSetting = (led_setting*)(szBuf + sizeof(ext_ioctl_data));
	memcpy(pLEDSetting, &ledSetting,sizeof(led_setting));

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_LED_TEST ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::updateLEDConfigtoEfuse(const led_setting& ledSetting)
{
	int ret = 0;

	//ext_ioctl_data data = { 0 };
	//data.cmd_type = FXGMAC_EFUSE_UPDATE_LED_CFG;
	//data.cmd_buf.buf = (void*)(&ledSetting);
	//data.cmd_buf.size_in = sizeof(ledSetting);

	//if ((ret = ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(&data))) < 0)
	//{
	//	perror("FUXI_IOCTL_DFS_COMMAND ioctl failed!");
	//	return ret;
	//}

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(led_setting);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_UPDATE_LED_CFG;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct led_setting* pLEDSetting = (led_setting*)(szBuf + sizeof(ext_ioctl_data));
	memcpy(pLEDSetting, &ledSetting, sizeof(led_setting));

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_UPDATE_LED_CFG ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::enableWOLtoEfuse()
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_EFUSE_WRITE_OOB;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_EFUSE_WRITE_OOB ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::getGSOSize(uint32_t& size)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_GET_GSO_SIZE;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_GET_GSO_SIZE ioctl failed!");
		ret = -1;
	}
	else
		size = pCommandData->val0;

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::setGSOSize(uint32_t size)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_SET_GSO_SIZE;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->val0 = size;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_SET_GSO_SIZE ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::getTXRXInterruptModeration(uint32_t& txIntModeration, uint32_t& rxIntModeration)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_GET_TXRX_MODERATION;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_GET_TXRX_MODERATION ioctl failed!");
		ret = -1;
	}
	else
	{
		rxIntModeration = pCommandData->val0;
		txIntModeration = pCommandData->val1;
	}
	
	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::setTXInterruptModeration(uint32_t txIntModeration)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_SET_TX_MODERATION;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->val0 = txIntModeration;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_SET_TX_MODERATION ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

int FuxiDriverInterface_Linux::setRXInterruptModeration(uint32_t rxIntModeration)
{
	int ret = 0;

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(ext_command_data);

	char* szBuf = new char[uTotalSize] {0};

	struct ext_ioctl_data* data = (ext_ioctl_data*)szBuf;

	data->cmd_type = FXGMAC_SET_RX_MODERATION;
	data->cmd_buf.buf = (void*)szBuf;
	data->cmd_buf.size_in = uTotalSize;

	struct ext_command_data* pCommandData = (ext_command_data*)(szBuf + sizeof(ext_ioctl_data));
	pCommandData->val0 = rxIntModeration;

	if (ioctl(m_fdFuxi, FUXI_IOCTL_DFS_COMMAND, (void*)(szBuf)) < 0)
	{
		perror("FXGMAC_SET_RX_MODERATION ioctl failed!");
		ret = -1;
	}

	if (szBuf)
		delete[] szBuf;

	return ret;
}

// for patch
int FuxiDriverInterface_Linux::readEfusePatch(uint32_t addr, uint32_t& value)
{
	int ret = 1;

	for (int i = EFUSE_REGION_C_PATCH_COUNT - 1; i >= 0; i--)
	{
		uint32_t uCurrentAddr = 0, uCurrentValue = 0;
		if ((ret = readEfusePatch((uint8_t)i, uCurrentAddr, uCurrentValue)) != 0)
			return ret;

		if (addr == uCurrentAddr)
		{
			value = uCurrentValue;
			return 0;
		}
	}

	return ret;
}

int FuxiDriverInterface_Linux::getFirstAvailablePatchIndex(uint8_t& index)
{
	int ret = 0;

	const uint8_t uMaxIndex = EFUSE_MAX_COMMON_PATCH_COUNT - 1;
	int nCurrentIndex = uMaxIndex;

	do
	{
		uint32_t uCurrentAddr = 0, uCurrentValue = 0;
		if ((ret = readEfusePatch((uint8_t)nCurrentIndex, uCurrentAddr, uCurrentValue)) != 0)
			return ret;

		if (uCurrentAddr == 0 && uCurrentValue == 0)
			nCurrentIndex--;
		else
			break;

	} while (nCurrentIndex >= 0);

	if (nCurrentIndex < 0)
		nCurrentIndex = 0;
	else
		nCurrentIndex++;

	if (nCurrentIndex > uMaxIndex)
		return 1;
	else
		index = (uint8_t)nCurrentIndex;

	return ret;
}