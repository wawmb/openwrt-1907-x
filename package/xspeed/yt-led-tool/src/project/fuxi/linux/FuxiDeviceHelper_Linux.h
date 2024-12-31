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

#include "fuxi/FuxiDeviceHelper.h"

#include "device/PCIDeviceHelper_Linux.h"


class FuxiDeviceHelper_Linux : public FuxiDeviceHelper
{
public:
	FuxiDeviceHelper_Linux();
	virtual ~FuxiDeviceHelper_Linux();

	virtual int initialize() override;
	virtual void release() override;

	virtual int getDeviceEnabled(uint8_t index, bool& isEnabled) override { return 1; };
	virtual int setDeviceEnable(uint8_t index, bool isEnabled) override { return 1; };

	virtual int restartDevice(uint8_t index, uint32_t interval_ms = 1000) override { return 1; };
	virtual int uninstallDevice(uint8_t index) override { return 1; };
	virtual int installDevice(uint8_t index) override { return 1; };

	virtual int getDeviceID(uint8_t index, std::basic_string<TCHAR>& tcsDeviceID) override { return 1; };
	virtual int getDeviceDescription(uint8_t index, std::basic_string<TCHAR>& tcsDeviceDescription) override { return 1; };
	virtual int getDeviceLocationInfo(uint8_t index, std::basic_string<TCHAR>& tcsLocationInfo) override { return 1; };
	virtual int getDeviceFriendlyName(uint8_t index, std::basic_string<TCHAR>& tcsFriendlyName) override;
	virtual int getDeviceInstanceId(std::size_t index, std::basic_string<TCHAR>& tcsInstanceId) override { return 1; };
	virtual int getDriverKey(std::size_t index, std::basic_string<TCHAR>& tcsDriverKey) override { return 1; };

	virtual int getDriverVersion(uint8_t index, std::basic_string<TCHAR>& tcsDriverVersion) override;

	virtual int getDeviceBDF(uint8_t index, uint8_t& bus, uint8_t& device, uint8_t& function) override;

	virtual int getPCIEMemBaseAddr(uint8_t index, uint64_t& address) override;
	virtual int read32PCIEConfigReg(uint8_t index, uint32_t offset, uint32_t& value) override;

protected:
	virtual int getCmdOutput(const TCHAR* szCmd, std::basic_string<TCHAR>& tcsOutput);

protected:
	PCIDeviceHelper_Linux* m_pPCIDeviceHelper_Linux;
};

