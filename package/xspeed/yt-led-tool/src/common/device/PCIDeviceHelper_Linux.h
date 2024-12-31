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
#include <stdint.h>
#include <string>

extern "C" {
#include <pci/pci.h>
// #include <pci.h>
}


class PCIDeviceHelper_Linux
{
public:
	PCIDeviceHelper_Linux();
	virtual ~PCIDeviceHelper_Linux();

	virtual int refresh();
	virtual void release();

	virtual std::size_t getDeviceCount();

	virtual int getMemBaseAddr(std::size_t index, std::size_t bar, uint64_t& memBaseAddr);

	virtual int read16(std::size_t index, int offset, uint16_t& value);
	virtual int read32(std::size_t index, int offset, uint32_t& value);

	virtual int getDeviceID(std::size_t index, uint16_t& deviceID);
	virtual int getVendorID(std::size_t index, uint16_t& vendorID);

	virtual int getDeviceLocationInfo(std::size_t index, uint8_t& bus, uint8_t& dev, uint8_t& func);
	virtual int getDeviceFriendlyName(std::size_t index, std::string& tcsFriendlyName);

	virtual int getDeviceDriverVersion(std::size_t index, std::string& tcsDriverVersion);

protected:
	struct pci_access* m_pPCIAccess;
	std::vector<pci_dev*> m_vecPCIDev;
};

