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

#include "FuxiEFuseManager.h"

#include "reg/RegisterHelper.h"
#include "fuxi/FuxiDefine.h"


#define EFUSE_DEV_LOST_PATCH_ADDR			0x1188
//#define EFUSE_DEV_LOST_PATCH_VALUE			0x00010111
#define EFUSE_DEV_LOST_PATCH_VALUE			0x00000111

#define EFUSE_ASPM_PATCH_ADDR				0x007C
#define EFUSE_ASPM_PATCH_VALUE				0x00474811

#define EFUSE_LED_PATCH_ADDR				0x1004
#define EFUSE_LED_PATCH_VALUE				0x16

#define EFUSE_CONFIG_PATCH_ADDR				0x08BC
#define EFUSE_START_PATCH_VALUE				0x0007FF41
#define EFUSE_END_PATCH_VALUE				0x0007FF40


FuxiEFuseManager::FuxiEFuseManager()
	: m_pFuxiDriverInterface(nullptr)
{

}

FuxiEFuseManager::~FuxiEFuseManager() 
{
}

int FuxiEFuseManager::initialize(FuxiDriverInterface* pFuxiDriverInterface)
{
	int ret = 0;

	release();

	if (pFuxiDriverInterface == nullptr)
		return -1;

	m_pFuxiDriverInterface = pFuxiDriverInterface;

	if ((ret = RefreshPatchItems()) != 0)
		return ret;	

	return ret;
}

void FuxiEFuseManager::release()
{
	if (m_pFuxiDriverInterface)
		m_pFuxiDriverInterface = nullptr;
}

int FuxiEFuseManager::RefreshPatchItems()
{
	int ret = 0;

	m_vecEFusePatchItem.clear();

	for (uint8_t index = 0; index < EFUSE_REGION_C_PATCH_COUNT; index++)
	{
		uint32_t uAddr = 0;
		uint32_t uValue = 0;

		if ((ret = m_pFuxiDriverInterface->readEfusePatch(index, uAddr, uValue)) != 0)
		{
			printf("RefreshPatchItems readEfusePatch index %u failed=%d\n", index, ret);
			return ret;
		}

		m_vecEFusePatchItem.push_back(EFUSE_PATCH_ITEM(index, uAddr, uValue));
	}

	return ret;
}

int FuxiEFuseManager::GetAllPatchItems(std::vector<EFUSE_PATCH_ITEM>& vecPatchItem)
{
	int ret = 0;

	vecPatchItem.clear();

	if ((ret = RefreshPatchItems()) != 0)
		return ret;

	vecPatchItem = m_vecEFusePatchItem;

	return ret;
}

int FuxiEFuseManager::ReadPatchItem(uint8_t index, uint32_t& addr, uint32_t& value)
{
	if (index < 0 || index >= m_vecEFusePatchItem.size())
		return -1;

	addr = m_vecEFusePatchItem[index].address;
	value = m_vecEFusePatchItem[index].value;

	return 0;
}

int FuxiEFuseManager::WritePatchItem(uint8_t index, uint32_t addr, uint32_t value)
{
	return m_pFuxiDriverInterface->writeEfusePatch(index, addr, value);
}

uint8_t FuxiEFuseManager::GetAvaliablePatchItemCount()
{
	uint8_t count = 0;

	for (auto iter = m_vecEFusePatchItem.rbegin(); iter != m_vecEFusePatchItem.rend(); iter++)
	{
		if ((iter->address) == 0 && (iter->value == 0))
			count++;
	}

	return count;
}

bool FuxiEFuseManager::HasAvaliablePatchItem()
{
	return (GetAvaliablePatchItemCount() > 0);
}

bool FuxiEFuseManager::IsPatchUnused(uint8_t index)
{
	if (index < 0 || index >= m_vecEFusePatchItem.size())
		return false;

	if (m_vecEFusePatchItem[index].address == 0 &&
		m_vecEFusePatchItem[index].value == 0)
		return true;

	return false;
}

int FuxiEFuseManager::GetFirstAvaliablePatchIndex(uint8_t& index)
{
	for (auto patchItem : m_vecEFusePatchItem)
	{
		if (IsPatchUnused(patchItem.index))
		{
			index = patchItem.index;
			return 0;
		}
	}

	return 1;
}

int FuxiEFuseManager::GetLatestPatchItem(uint32_t addr, EFUSE_PATCH_ITEM& patchItem)
{
	int index = GetLatestPatchIndex(addr);
		
	if (index == -1)
		return -1;

	patchItem = m_vecEFusePatchItem[index];

	return 0;
}

int FuxiEFuseManager::GetLatestPatchIndex(uint32_t addr)
{
	int index = -1;

	for (auto iter = m_vecEFusePatchItem.rbegin(); iter != m_vecEFusePatchItem.rend(); iter++)
	{
		if (iter->address == addr)
		{
			index = (int)iter->index;
			break;
		}
	}

	return index;
}

int FuxiEFuseManager::GetLatestPatchIndex(uint32_t addr, uint32_t value)
{
	int index = -1;

	for (auto iter = m_vecEFusePatchItem.rbegin(); iter != m_vecEFusePatchItem.rend(); iter++)
	{
		if ((iter->address == addr) && (iter->value == value))
		{
			index = (int)iter->index;
			break;
		}
	}

	return index;
	
}

bool FuxiEFuseManager::IsPatchItemExists(uint32_t addr)
{
	return (GetLatestPatchIndex(addr) != -1);
}

bool FuxiEFuseManager::IsPatchItemExists(uint32_t addr, uint32_t value)
{
	return (GetLatestPatchIndex(addr, value) != -1);
}

int FuxiEFuseManager::ApplyNewPatch(uint32_t addr, uint32_t value)
{
	int ret = 0;

	uint8_t index = 0;
	if ((ret = GetFirstAvaliablePatchIndex(index)) != 0)
		return ret;

	if ((ret = WritePatchItem(index, addr, value)) != 0)
		return ret;

	return ret;
}

bool FuxiEFuseManager::IsDeviceLostPatchExists()
{
	EFUSE_PATCH_ITEM patchItem;
	if (GetLatestPatchItem(EFUSE_DEV_LOST_PATCH_ADDR, patchItem) != 0)
		return false;

	// check bit4 == 1?
	return GET_REG_BITS(patchItem.value, 4, 1);

	//return IsPatchItemExists(EFUSE_DEV_LOST_PATCH_ADDR, EFUSE_DEV_LOST_PATCH_VALUE);
}

int FuxiEFuseManager::GetDeviceLostPatchIndex(uint8_t& index)
{
	int nIndex = GetLatestPatchIndex(EFUSE_DEV_LOST_PATCH_ADDR, EFUSE_DEV_LOST_PATCH_VALUE);

	if (nIndex == -1)
		return -1;
	else
		index = (uint8_t)nIndex;

	return 0;
}

int FuxiEFuseManager::ApplyDeviceLostPatch()
{
	return ApplyNewPatch(EFUSE_DEV_LOST_PATCH_ADDR, EFUSE_DEV_LOST_PATCH_VALUE);
}

bool FuxiEFuseManager::IsASPML1PatchExists()
{
	EFUSE_PATCH_ITEM patchItem;
	if (GetLatestPatchItem(EFUSE_DEV_LOST_PATCH_ADDR, patchItem) != 0)
		return false;

	// check bit16 == 1?
	return GET_REG_BITS(patchItem.value, 16, 1);
}

bool FuxiEFuseManager::IsASPMPatchExists()
{
	uint8_t uIndex = 0;
	return (GetASPMPatchIndex(uIndex) == 0);
}

int FuxiEFuseManager::GetASPMPatchIndex(uint8_t& index)
{
	int nIndex = -1;

	for (auto iter = m_vecEFusePatchItem.rbegin(); iter != m_vecEFusePatchItem.rend(); iter++)
	{
		// check end
		if (iter->address != EFUSE_CONFIG_PATCH_ADDR || iter->value != EFUSE_END_PATCH_VALUE)
			continue;

		// check at least 3 patchs
		if ((iter + 2) == m_vecEFusePatchItem.rend())
			return false;

		// check start
		if ((iter + 2)->address != EFUSE_CONFIG_PATCH_ADDR || (iter + 2)->value != EFUSE_START_PATCH_VALUE)
			continue;

		// check patch
		if ((iter + 1)->address == EFUSE_ASPM_PATCH_ADDR)
		{
			if ((iter + 1)->value == EFUSE_ASPM_PATCH_VALUE)
			{
				nIndex = (iter + 1)->index;
				break;
			}
			else
				continue;
		}
	}

	if (nIndex == -1)
		return -1;
	else
		index = (uint8_t)nIndex;

	return 0;
}

int FuxiEFuseManager::ApplyASPMPatch()
{
	int ret = 0;

	uint8_t index = 0;

	bool bFindAvaliablePatch = false;
	for (auto patchItem : m_vecEFusePatchItem)
	{
		index = patchItem.index;

		// need at least 3 patchs
		if (IsPatchUnused(index) && IsPatchUnused((uint8_t)(index + 1)) && IsPatchUnused((uint8_t)(index + 1)))
		{
			bFindAvaliablePatch = true;
			break;
		}
	}

	if (!bFindAvaliablePatch)
		return -1;

	if ((ret = WritePatchItem(index, EFUSE_CONFIG_PATCH_ADDR, EFUSE_START_PATCH_VALUE)) != 0)
		return ret;

	index++;

	if ((ret = WritePatchItem(index, EFUSE_ASPM_PATCH_ADDR, EFUSE_ASPM_PATCH_VALUE)) != 0)
		return ret;

	index++;

	if ((ret = WritePatchItem(index, EFUSE_CONFIG_PATCH_ADDR, EFUSE_END_PATCH_VALUE)) != 0)
		return ret;

	return ret;
}

bool FuxiEFuseManager::IsLEDPatchExists()
{
	return IsPatchItemExists(EFUSE_LED_PATCH_ADDR, EFUSE_LED_PATCH_VALUE);
}

int FuxiEFuseManager::GetLEDPatchIndex(uint8_t& index)
{
	int nIndex = GetLatestPatchIndex(EFUSE_LED_PATCH_ADDR, EFUSE_LED_PATCH_VALUE);

	if (nIndex == -1)
		return -1;
	else
		index = (uint8_t)nIndex;

	return 0;
}

int FuxiEFuseManager::ApplyLEDPatch()
{
	return ApplyNewPatch(EFUSE_LED_PATCH_ADDR, EFUSE_LED_PATCH_VALUE);
}

bool FuxiEFuseManager::HasErrorPatchItem()
{
	for (auto patchItem : m_vecEFusePatchItem)
	{
		if (patchItem.address == 0xFFFF ||
			patchItem.value == 0xFFFFFFFF)
			return true;
	}

	return false;
}

bool FuxiEFuseManager::isMACAddrLegal(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	// Unicast MAC address or Broadcast MAC address
	if (macAddr[0] & 0x01)
		return false;

	//// Broadcast MAC address
	//bool isBroadcast = true;
	//for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
	//{
	//	if (macAddr[i] != 0xFF)
	//	{
	//		isBroadcast = false;
	//		break;
	//	}
	//}

	//if (isBroadcast)
	//	return false;

	// Unknown MAC address
	bool isUnknown = true;
	for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
	{
		if (macAddr[i] != 0x00)
		{
			isUnknown = false;
			break;
		}
	}

	if (isUnknown)
		return false;

	return true;
}

int FuxiEFuseManager::readMACAddr(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	return m_pFuxiDriverInterface->readMACAddressfromEFuse(macAddr);
}

int FuxiEFuseManager::writeMACAddr(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	if (!isMACAddrLegal(macAddr))
		return 1;

	return m_pFuxiDriverInterface->writeMACAddresstoEfuse(macAddr);
}

int FuxiEFuseManager::readSubSysInfo(uint32_t& subSystemID, uint32_t& subVendorID, uint32_t& revisionID)
{
	return m_pFuxiDriverInterface->readSubSystemIDfromEFuse(subSystemID, subVendorID, revisionID);
}

int FuxiEFuseManager::readSubSysInfo(uint32_t& subSystemID, uint32_t& subVendorID)
{
	uint32_t uRevisionID = 0;
	return m_pFuxiDriverInterface->readSubSystemIDfromEFuse(subSystemID, subVendorID, uRevisionID);
}

int FuxiEFuseManager::writeSubSysInfo(const uint32_t& subSystemID, const uint32_t& subVendorID, const uint32_t& revisionID)
{
	return m_pFuxiDriverInterface->writeSubSystemIDtoEFuse(subSystemID, subVendorID, revisionID);
}

int FuxiEFuseManager::writeSubSysInfo(const uint32_t& subSystemID, const uint32_t& subVendorID)
{
	return m_pFuxiDriverInterface->writeSubSystemIDtoEFuse(subSystemID, subVendorID);
}

int FuxiEFuseManager::ReadLEDSolution(uint8_t& solution)
{
	int ret = 0;

	// byte 0, bit 0~4
	uint32_t uValue = 0;
	if ((ret = m_pFuxiDriverInterface->readEfuseReg(EFUSE_REGION_A_LED_REG_OFFSET, uValue)) != 0)
		return ret;

	solution = (uint8_t)(uValue & 0x1F);

	return ret;
}

int FuxiEFuseManager::writeLEDSolution(uint8_t solution)
{
	return m_pFuxiDriverInterface->writeLEDStatustoEFuse(solution);
}

bool FuxiEFuseManager::IsLEDinConfigMode()
{
	bool bRet = false;

	uint8_t uSolution = 0;
	if (ReadLEDSolution(uSolution) != 0)
		return bRet;

	bRet = (uSolution == EFUSE_LED_CONFIG_MODE_VALUE);

	return bRet;
}

int FuxiEFuseManager::SetLEDtoConfigMode()
{
	int ret = 0;

	uint8_t uSolution = 0;
	if ((ret = ReadLEDSolution(uSolution)) != 0)
		return ret;

	if (uSolution != EFUSE_LED_CONFIG_MODE_VALUE)
		ret = writeLEDSolution(EFUSE_LED_CONFIG_MODE_VALUE);

	return ret;
}

int FuxiEFuseManager::updateLEDConfig(const led_setting& ledSetting)
{
	return m_pFuxiDriverInterface->updateLEDConfigtoEfuse(ledSetting);
}

int FuxiEFuseManager::simulateLEDSetting(const led_setting& ledSetting)
{
	return m_pFuxiDriverInterface->simulateLEDSetting(ledSetting);
}

int FuxiEFuseManager::ReadWOLStatus(bool& isEnable)
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = m_pFuxiDriverInterface->readEfuseReg(EFUSE_REGION_B_WOL_REG_OFFSET, uValue)) != 0)
		return ret;

	isEnable = (uValue & 0x04);

	return ret;
}

int FuxiEFuseManager::WriteWOLStatus(bool isEnable)
{
	int ret = 0;

	if (isEnable)
	{
		if ((ret = m_pFuxiDriverInterface->enableWOLtoEfuse()) != 0)
			return ret;
	}
	else
	{
		// no api, efuse default is 0, so do nothing
		ret = 0;
	}

	return ret;
}

int FuxiEFuseManager::ReadRegValue(uint32_t addr, uint32_t& value)
{
	return m_pFuxiDriverInterface->readEfuseReg(addr, value);
}

int FuxiEFuseManager::ReadRegWORD(uint32_t addr, uint16_t& value, bool littleEndian)
{
	int ret = 0;

	value = 0;

	uint32_t uLowValue = 0;
	if ((ret = m_pFuxiDriverInterface->readEfuseReg(addr, uLowValue)) != 0)
		return ret;

	uint32_t uHightValue = 0;
	if ((ret = m_pFuxiDriverInterface->readEfuseReg(++addr, uHightValue)) != 0)
		return ret;

	if (littleEndian)
		value = (uint16_t)uLowValue + (uint16_t)(uHightValue << 8);
	else
		value = (uint16_t)uHightValue + (uint16_t)(uLowValue << 8);

	return ret;
}

int FuxiEFuseManager::ReadAllRegValue(uint8_t(&eFuseReg)[EFUSE_BYTE_COUNT])
{
	int ret = 0;

	for (uint32_t addr = 0; addr < EFUSE_BYTE_COUNT; addr++)
	{
		uint32_t value = 0;
		if ((ret = m_pFuxiDriverInterface->readEfuseReg(addr, value)) != 0)
			return ret;

		eFuseReg[addr] = (uint8_t)value;
	}

	//std::vector<EFUSE_PATCH_ITEM> vecEFusePatchItem;
	//if ((ret = ReadEFusePatchItems(vecEFusePatchItem)) != 0)
	//	return ret;
	//
	//int offset = EFUSE_REGION_AB_BYTE_COUNT;
	//for (auto patchItem : vecEFusePatchItem)
	//{
	//	memcpy(eFuseReg + offset, &patchItem.address, EFUSE_REGION_C_PATCH_ADDR_BYTE_COUNT);
	//	offset += EFUSE_REGION_C_PATCH_ADDR_BYTE_COUNT;
	//	memcpy(eFuseReg + offset, &patchItem.value, EFUSE_REGION_C_PATCH_VALUE_BYTE_COUNT);
	//	offset += EFUSE_REGION_C_PATCH_VALUE_BYTE_COUNT;
	//}

	return ret;
}