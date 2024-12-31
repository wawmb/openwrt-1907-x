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

#include "FuxiDeviceManager.h"

#ifdef _WIN32
#include"fuxi/windows/FuxiDeviceHelper_Win.h"
#else
#include"fuxi/linux/FuxiDeviceHelper_Linux.h"
#endif // _WIN32


FuxiDeviceManager::FuxiDeviceManager()
	: m_pFuxiDeviceHelper(nullptr)
{
}

FuxiDeviceManager::~FuxiDeviceManager()
{
	if (m_pFuxiDeviceHelper)
	{
		m_pFuxiDeviceHelper->release();
		delete m_pFuxiDeviceHelper;
		m_pFuxiDeviceHelper = nullptr;
	}
}

int FuxiDeviceManager::initialize()
{
	int ret = 0;

	release();

#ifdef _WIN32
	m_pFuxiDeviceHelper = new FuxiDeviceHelper_Win();
#else
	m_pFuxiDeviceHelper = new FuxiDeviceHelper_Linux();
#endif // _WIN32

	if ((ret = m_pFuxiDeviceHelper->initialize()) != 0)
		return ret;

	uint8_t uDeviceCount = m_pFuxiDeviceHelper->getDeviceCount();
	for (uint8_t i = 0; i < uDeviceCount; i++)
	{
		uint8_t bus = 0;
		uint8_t device = 0;
		uint8_t function = 0;

		if (m_pFuxiDeviceHelper->getDeviceBDF(i, bus, device, function) != 0)
		{
			m_vecFuxiDevice.push_back(FUXI_DEVICE());
			continue;
		}

		FUXI_DEVICE fuxiDevice;
		fuxiDevice.pcieBDF.bus = bus;
		fuxiDevice.pcieBDF.device = device;
		fuxiDevice.pcieBDF.function = function;

		fuxiDevice.deviceInterface = new FuxiDeviceInterface(m_pFuxiDeviceHelper);
		if (fuxiDevice.deviceInterface->initialize(i) != 0)
		{
			delete fuxiDevice.deviceInterface;
			fuxiDevice.deviceInterface = nullptr;
		}

		m_vecFuxiDevice.push_back(fuxiDevice);
	}

	return ret;
}

void FuxiDeviceManager::release()
{
	if (m_pFuxiDeviceHelper)
	{
		m_pFuxiDeviceHelper->release();
		delete m_pFuxiDeviceHelper;
		m_pFuxiDeviceHelper = nullptr;
	}

	for (auto fuxiDevice : m_vecFuxiDevice)
	{
		if (fuxiDevice.deviceInterface)
		{
			fuxiDevice.deviceInterface->release();
			delete fuxiDevice.deviceInterface;
			fuxiDevice.deviceInterface = nullptr;
		}
	}

	m_vecFuxiDevice.clear();
}

uint8_t FuxiDeviceManager::deviceCount()
{
	return (uint8_t)m_vecFuxiDevice.size();
}

int FuxiDeviceManager::getDeviceDisplayName(uint8_t index, std::basic_string<TCHAR>& tcsName)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDeviceFriendlyName(index, tcsName);
}

int FuxiDeviceManager::getDeviceID(uint8_t index, std::basic_string<TCHAR>& tcsDeviceID)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDeviceID(index, tcsDeviceID);
}

int FuxiDeviceManager::getDriverVersion(uint8_t index, std::basic_string<TCHAR>& tcsDriverVersion)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDriverVersion(index, tcsDriverVersion);
}

int FuxiDeviceManager::getDeviceLocationInfo(uint8_t index, std::basic_string<TCHAR>& tcsDeviceLocation)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDeviceLocationInfo(index, tcsDeviceLocation);
}

int FuxiDeviceManager::getDeviceInstanceId(std::size_t index, std::basic_string<TCHAR>& tcsInstanceId)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDeviceInstanceId(index, tcsInstanceId);
}

int FuxiDeviceManager::getDriverKey(std::size_t index, std::basic_string<TCHAR>& tcsDriverKey)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	return m_pFuxiDeviceHelper->getDriverKey(index, tcsDriverKey);
}

int FuxiDeviceManager::getDevicePcieBDF(uint8_t index, PCIE_BDF& pcieBDF)
{
	if (index >= (uint8_t)m_vecFuxiDevice.size())
		return -1;

	pcieBDF = m_vecFuxiDevice[index].pcieBDF;

	return 0;
}

FuxiDeviceInterface* FuxiDeviceManager::getDeviceInterface(uint8_t index)
{
	if (index < (uint8_t)m_vecFuxiDevice.size())
		return m_vecFuxiDevice[index].deviceInterface;

	return nullptr;
}