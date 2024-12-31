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

#include "utility/CrossPlatformUtility.h"


class FuxiDeviceHelper
{
public:
	FuxiDeviceHelper() = default;
	virtual ~FuxiDeviceHelper() = default;

	virtual int initialize() = 0;
	virtual void release() = 0;

	virtual uint8_t getDeviceCount();

	virtual int getDeviceEnabled(uint8_t index, bool& isEnabled) = 0;
	virtual int setDeviceEnable(uint8_t index, bool isEnabled) = 0;

	virtual int restartDevice(uint8_t index, uint32_t interval_ms = 1000) = 0;
	virtual int uninstallDevice(uint8_t index) = 0;
	virtual int installDevice(uint8_t index) = 0;

	virtual int getDeviceID(uint8_t index, std::basic_string<TCHAR>& tcsDeviceID) = 0;
	virtual int getDeviceDescription(uint8_t index, std::basic_string<TCHAR>& tcsDeviceDescription) = 0;
	virtual int getDeviceLocationInfo(uint8_t index, std::basic_string<TCHAR>& tcsLocationInfo) = 0;
	virtual int getDeviceFriendlyName(uint8_t index, std::basic_string<TCHAR>& tcsFriendlyName) = 0;
	virtual int getDeviceInstanceId(std::size_t index, std::basic_string<TCHAR>& tcsInstanceId) = 0;
	virtual int getDriverKey(std::size_t index, std::basic_string<TCHAR>& tcsDriverKey) = 0;

	virtual int getDriverVersion(uint8_t index, std::basic_string<TCHAR>& tcsDriverVersion) = 0;

	virtual int getDeviceBDF(uint8_t index, uint8_t& bus, uint8_t& device, uint8_t& function) = 0;

	virtual int getPCIEMemBaseAddr(uint8_t index, uint64_t& address) = 0;
	virtual int read32PCIEConfigReg(uint8_t index, uint32_t offset, uint32_t& value) = 0;

protected:
	std::vector<std::size_t> m_vecDeviceIndex;
};

