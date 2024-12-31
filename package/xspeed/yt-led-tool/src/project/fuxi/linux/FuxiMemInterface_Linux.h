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

#include "fuxi/FuxiDriverInterface.h"


class FuxiMemInterface_Linux : public FuxiDriverInterface
{
public:
	FuxiMemInterface_Linux();
	virtual ~FuxiMemInterface_Linux();

	virtual int open(bool retry = false) override { return 1; };
	virtual int open(uint8_t index) override { return 1; };
	virtual int open(uint64_t memBaseAddress) override;
	virtual bool isOpen() override;
	virtual void close() override;

	virtual int getHandle(void* handle) override;

	virtual int readMemReg(uint32_t addr, uint32_t& value) override;
	virtual int writeMemReg(uint32_t addr, uint32_t value) override;

	virtual int readGMACReg(uint32_t addr, uint32_t& value) override;
	virtual int writeGMACReg(uint32_t addr, uint32_t value) override;

	virtual int readGMACConfigReg(uint32_t addr, uint32_t& value) override { return 1; };
	virtual int writeGMACConfigReg(uint32_t addr, uint32_t value) override { return 1; };

	virtual int readPHYReg(uint32_t addr, uint32_t& value) override;
	virtual int writePHYReg(uint32_t addr, uint32_t value) override;

	virtual int readEfuseReg(uint32_t addr, uint32_t& value) override { return 1; };

	virtual int readEfusePatch(uint8_t index, uint32_t& addr, uint32_t& value) override { return 1; };
	virtual int writeEfusePatch(uint8_t index, uint32_t addr, uint32_t value) override { return 1; };

	virtual int readMACAddressfromEFuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) override { return 1; };
	virtual int writeMACAddresstoEfuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) override { return 1; };

	virtual int readSubSystemIDfromEFuse(uint32_t& subDeviceID, uint32_t& subVendorID, uint32_t& revisionID) override { return 1; };
	virtual int writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID, uint32_t revisionID) override { return 1; };
	virtual int writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID) override { return 1; };

	virtual int writeLEDStatustoEFuse(uint8_t status) override { return 1; };

	virtual int enableWOLtoEfuse() override { return 1; };

	virtual int setPHYPackageCounterEnable(bool enable) override { return 1; };

	virtual int getAllPackageCounter(MP_COUNT_STRUC& packageCounter) override { return 1; };

	virtual int initLoopback() override { return 1; };
	virtual int cleanLoopback() override { return 1; };

	virtual int updateLEDConfigtoEfuse(const led_setting& ledSetting) override { return 1; };
	virtual int simulateLEDSetting(const led_setting& ledSetting) override { return 1; };

	virtual int getDeviceLocation(std::wstring& wcsDevLoaction) override { return 1; };

	virtual int setDriverLoopbackMode(uint32_t mode) override { return 1; };

	//virtual int getLEDSolutionfromEfuse(uint8_t& solution);

	virtual int getPCIMemInfo(uint32_t& address, uint32_t& range) override { return 1; };
	virtual int getPCIPortInfo(uint32_t& address, uint32_t& range) override { return 1; };
	virtual int getPCIInterruptLevel(uint32_t& level) override { return 1; };

	virtual int getGSOSize(uint32_t& size) override { return 1; };
	virtual int setGSOSize(uint32_t size) override { return 1; };

	virtual int getTXRXInterruptModeration(uint32_t& txIntModeration, uint32_t& rxIntModeration) override { return 1; };
	virtual int setTXInterruptModeration(uint32_t txIntModeration) override { return 1; };
	virtual int setRXInterruptModeration(uint32_t rxIntModeration) override { return 1; };

protected:
	uint64_t m_FuxiPCIMemBase;
};

