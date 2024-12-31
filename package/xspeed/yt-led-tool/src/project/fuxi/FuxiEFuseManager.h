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

#include <vector>

#include "fuxi/FuxiDriverInterface.h"


struct EFUSE_PATCH_ITEM
{
	uint8_t index;
	uint32_t address;
	uint32_t value;

	EFUSE_PATCH_ITEM()
	{
		this->index = 0;
		this->address = 0;
		this->value = 0;
	}

	EFUSE_PATCH_ITEM(uint8_t uIndex, uint32_t uAddr, uint32_t uValue)
	{
		this->index = uIndex;
		this->address = uAddr;
		this->value = uValue;
	}
};

class FuxiEFuseManager
{
public:
	FuxiEFuseManager();
	virtual ~FuxiEFuseManager();

	virtual int initialize(FuxiDriverInterface* pFuxiDriverInterface);
	virtual void release();

	virtual int RefreshPatchItems();
	virtual int GetAllPatchItems(std::vector<EFUSE_PATCH_ITEM>& vecPatchItem);

	virtual int ReadPatchItem(uint8_t index, uint32_t& addr, uint32_t& value);
	virtual int WritePatchItem(uint8_t index, uint32_t addr, uint32_t value);

	virtual uint8_t GetAvaliablePatchItemCount();
	virtual bool HasAvaliablePatchItem();
	virtual bool IsPatchUnused(uint8_t index);
	virtual int GetFirstAvaliablePatchIndex(uint8_t& index);

	virtual int GetLatestPatchItem(uint32_t addr, EFUSE_PATCH_ITEM& patchItem);

	virtual int GetLatestPatchIndex(uint32_t addr);
	virtual int GetLatestPatchIndex(uint32_t addr, uint32_t value);

	virtual bool IsPatchItemExists(uint32_t addr);
	virtual bool IsPatchItemExists(uint32_t addr, uint32_t value);

	//customer
	virtual int ApplyNewPatch(uint32_t addr, uint32_t value);

	virtual bool IsDeviceLostPatchExists();
	virtual int GetDeviceLostPatchIndex(uint8_t& index);
	virtual int ApplyDeviceLostPatch();

	virtual bool IsASPML1PatchExists();
	virtual bool IsASPMPatchExists();
	virtual int GetASPMPatchIndex(uint8_t& index);
	virtual int ApplyASPMPatch();

	virtual bool IsLEDPatchExists();
	virtual int GetLEDPatchIndex(uint8_t& index);
	virtual int ApplyLEDPatch();

	virtual bool HasErrorPatchItem();

	//function
	virtual bool isMACAddrLegal(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]);
	virtual int readMACAddr(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]);
	virtual int writeMACAddr(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]);

	virtual int readSubSysInfo(uint32_t& subSystemID, uint32_t& subVendorID, uint32_t& revisionID);
	virtual int readSubSysInfo(uint32_t& subSystemID, uint32_t& subVendorID);
	virtual int writeSubSysInfo(const uint32_t& subSystemID, const uint32_t& subVendorID, const uint32_t& revisionID);
	virtual int writeSubSysInfo(const uint32_t& subSystemID, const uint32_t& subVendorID);

	virtual int ReadLEDSolution(uint8_t& solution);
	virtual int writeLEDSolution(uint8_t solution);
	virtual bool IsLEDinConfigMode();
	virtual int SetLEDtoConfigMode();
	virtual int updateLEDConfig(const led_setting& ledSetting);
	virtual int simulateLEDSetting(const led_setting& ledSetting);
	
	virtual int ReadWOLStatus(bool& isEnable);
	virtual int WriteWOLStatus(bool isEnable);

	virtual int ReadRegValue(uint32_t addr, uint32_t& value);
	virtual int ReadRegWORD(uint32_t addr, uint16_t& value, bool littleEndian = true);
	virtual int ReadAllRegValue(uint8_t(&eFuseReg)[EFUSE_BYTE_COUNT]);


protected:
	FuxiDriverInterface* m_pFuxiDriverInterface;

	std::vector<EFUSE_PATCH_ITEM> m_vecEFusePatchItem;
};

