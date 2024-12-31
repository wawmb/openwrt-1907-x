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

#include "FuxiDeviceHelper_Linux.h"

#include "fuxi/FuxiDefine.h"

#include "reg/RegisterHelper.h"

FuxiDeviceHelper_Linux::FuxiDeviceHelper_Linux()
	: FuxiDeviceHelper()
	, m_pPCIDeviceHelper_Linux(nullptr)
{
}

FuxiDeviceHelper_Linux::~FuxiDeviceHelper_Linux()
{
	release();
}

int FuxiDeviceHelper_Linux::initialize()
{
	int ret = 0;

	release();

	m_pPCIDeviceHelper_Linux = new PCIDeviceHelper_Linux();

	if ((ret = m_pPCIDeviceHelper_Linux->refresh()))
		return ret;

	std::size_t uDeviceCount = m_pPCIDeviceHelper_Linux->getDeviceCount();
	for (std::size_t i = 0; i < uDeviceCount; i++)
	{
		uint16_t uDeviceID = 0, uVendorID = 0;
		m_pPCIDeviceHelper_Linux->getDeviceID(i, uDeviceID);
		m_pPCIDeviceHelper_Linux->getVendorID(i, uVendorID);

		if ((uDeviceID == FUXI_DEVICE_ID) && (uVendorID == FUXI_VENDOR_ID))
			m_vecDeviceIndex.push_back(i);
	}

	if (m_vecDeviceIndex.empty())
	{
		printf("Warning, can't find any YT6801/YT6801S device.\n");
		return 1;
	}


	return ret;
}

void FuxiDeviceHelper_Linux::release()
{
	m_vecDeviceIndex.clear();

	if (m_pPCIDeviceHelper_Linux)
	{
		m_pPCIDeviceHelper_Linux->release();
		m_pPCIDeviceHelper_Linux = nullptr;
	}
}

int FuxiDeviceHelper_Linux::getDeviceFriendlyName(uint8_t index, std::basic_string<TCHAR>& tcsFriendlyName)
{
	if (index >= (uint8_t)m_vecDeviceIndex.size())
		return -1;

	std::size_t uPCIDevIndex = m_vecDeviceIndex[index];

	return m_pPCIDeviceHelper_Linux->getDeviceFriendlyName(uPCIDevIndex, tcsFriendlyName);
}

int FuxiDeviceHelper_Linux::getDriverVersion(uint8_t index, std::basic_string<TCHAR>& tcsDriverVersion)
{
	int ret = 0;

	std::basic_string<TCHAR> tcsCmd = _T("cat /sys/module/yt6801/version | xargs echo -n ");

	if ((ret = getCmdOutput(tcsCmd.c_str(), tcsDriverVersion)) == 0 && !tcsDriverVersion.empty())
		return ret;
	else
		printf("Warning, get driver version from file failed!\n");

	tcsCmd = _T("modinfo fuxi | grep \"^version\" | awk '{print $2}' | xargs echo -n");

	if ((ret = getCmdOutput(tcsCmd.c_str(), tcsDriverVersion)) == 0 && !tcsDriverVersion.empty())
		return ret;
	else
		printf("Warning, get driver version from modinfo failed!\n");

	//if ((ret = m_pPCIDeviceHelper_Linux->getDeviceDriverVersion(index, tcsDriverVersion)) != 0)
	//{
	//	printf("Warning, get driver version failed!\n");
	//	return ret;
	//}

	return ret;
}

int FuxiDeviceHelper_Linux::getDeviceBDF(uint8_t index, uint8_t& bus, uint8_t& device, uint8_t& function)
{
	if (index >= (uint8_t)m_vecDeviceIndex.size())
		return -1;

	std::size_t uPCIDevIndex = m_vecDeviceIndex[index];

	return m_pPCIDeviceHelper_Linux->getDeviceLocationInfo(uPCIDevIndex, bus, device, function);
}

int FuxiDeviceHelper_Linux::getPCIEMemBaseAddr(uint8_t index, uint64_t& address)
{
	int ret = 0;

	if (index >= (uint8_t)m_vecDeviceIndex.size())
		return -1;

	std::size_t uPCIDevIndex = m_vecDeviceIndex[index];

	if ((ret = m_pPCIDeviceHelper_Linux->getMemBaseAddr(uPCIDevIndex, 0, address)) != 0)
		return ret;

	address = SET_REG_BITS(address, 0, 4, 0);

	return ret;
}

int FuxiDeviceHelper_Linux::read32PCIEConfigReg(uint8_t index, uint32_t offset, uint32_t& value)
{
	if (index >= (uint8_t)m_vecDeviceIndex.size())
		return -1;

	std::size_t uPCIDevIndex = m_vecDeviceIndex[index];

	return m_pPCIDeviceHelper_Linux->read32(uPCIDevIndex, (int)offset, value);
}

int FuxiDeviceHelper_Linux::getCmdOutput(const TCHAR* szCmd, std::basic_string<TCHAR>& tcsOutput)
{
	int ret = 0;

	FILE* pFile = popen(szCmd, "r");
	if (pFile == NULL)
		return -1;

	TCHAR szOutput[256] = { 0 };
	if (fgets(szOutput, sizeof(szOutput) / sizeof(TCHAR), pFile) != NULL)
		tcsOutput = szOutput;
	else
		ret = -2;

	pclose(pFile);

	return ret;
}