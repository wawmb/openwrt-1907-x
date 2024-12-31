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
#include <string>

#include "fuxi/FuxiDeviceInterface.h"
#include "fuxi/FuxiDeviceHelper.h"
#include "fuxi/FuxiDefine.h"

#include "utility/CrossPlatformUtility.h"


struct FUXI_DEVICE
{
	PCIE_BDF pcieBDF;
	FuxiDeviceInterface* deviceInterface;

	FUXI_DEVICE()
	{
		pcieBDF = { 0 };
		deviceInterface = nullptr;
	}
};


class FuxiDeviceManager
{
public:
	FuxiDeviceManager();
	virtual ~FuxiDeviceManager();

	virtual int initialize();
	virtual void release();

	virtual uint8_t deviceCount();

	virtual int getDeviceDisplayName(uint8_t index, std::basic_string<TCHAR>& tcsName);
	virtual int getDeviceID(uint8_t index, std::basic_string<TCHAR>& tcsDeviceID);
	virtual int getDriverVersion(uint8_t index, std::basic_string<TCHAR>& tcsDriverVersion);
	virtual int getDeviceLocationInfo(uint8_t index, std::basic_string<TCHAR>& tcsDeviceLocation);
	virtual int getDeviceInstanceId(std::size_t index, std::basic_string<TCHAR>& tcsInstanceId);
	virtual int getDriverKey(std::size_t index, std::basic_string<TCHAR>& tcsDriverKey);

	virtual int getDevicePcieBDF(uint8_t index, PCIE_BDF& pcieBDF);

	virtual int enableDevice(uint8_t index) = 0;
	virtual int disableDevice(uint8_t index) = 0;
	virtual int restartDevice(uint8_t index) = 0;
	virtual int uninstallDevice(uint8_t index) = 0;
	virtual int installDevice(uint8_t index) = 0;

	virtual FuxiDeviceInterface* getDeviceInterface(uint8_t index);

protected:
	FuxiDeviceHelper* m_pFuxiDeviceHelper;
	std::vector<FUXI_DEVICE> m_vecFuxiDevice;
};

