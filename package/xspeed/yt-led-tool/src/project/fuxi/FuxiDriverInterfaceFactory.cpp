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

#include "FuxiDriverInterfaceFactory.h"

#include <cstdlib>
#include <regex>

#ifdef _WIN32

#include <Windows.h>
#include <setupapi.h>
#pragma comment(lib, "setupapi.lib")

#include "fuxi/windows/FuxiDriverInterface_Win.h"
#include "fuxi/windows/FuxiNetAdapterDriverInterface_Win.h"

#define		WIN_NDIS_MULTI_MIN_DRV_VER		_T("1.0.1.26")

#define		FUXI_MAX_USER_MODE_COUNT		100

#else

#include "fuxi/linux/FuxiDriverInterface_Linux.h"
#include "fuxi/linux/FuxiSocketInterface_Linux.h"
#include "fuxi/linux/FuxiMemInterface_Linux.h"

#define		LINUX_SOCKET_MIN_DRV_VER		_T("1.0.22")

#define		FUXI_MAX_USER_MODE_COUNT		FUXI_MAX_DEV_COUNT

#endif // _WIN32

#include "fuxi/FuxiDefine.h"
#include "utility/CrossPlatformUtility.h"


FuxiDriverInterfaceFactory::FuxiDriverInterfaceFactory(FuxiDeviceHelper* fuxiDeviceHelper)
	: m_pFuxiDeviceHelper(fuxiDeviceHelper)
{
}

FuxiDriverInterfaceFactory::~FuxiDriverInterfaceFactory()
{
}

int FuxiDriverInterfaceFactory::createDriverInterface(uint8_t index, FuxiDriverInterface** pFuxiDriverInterface)
{
	int ret = 0;

	if (m_pFuxiDeviceHelper == nullptr)
		return -1;

	FUXI_DRIVER_INTERFACE_TYPE type = getDriverInterfaceType(index);

	if ((ret = createDriverInterface(type, pFuxiDriverInterface)) != 0)
		return ret;

	if (initDriverInterface(type, *pFuxiDriverInterface, index) != 0)
	{
		delete *pFuxiDriverInterface;
		*pFuxiDriverInterface = nullptr;
		printf("Initialize device %u driver interface failed.\n", index);
	}

	return ret;
}

int FuxiDriverInterfaceFactory::createDriverInterface(FUXI_DRIVER_INTERFACE_TYPE type, FuxiDriverInterface** pFuxiDriverInterface)
{
	if (pFuxiDriverInterface == nullptr)
		return -1;

#ifdef _WIN32

	switch (type)
	{
	case WIN_NIDS:
	case WIN_NIDS_MULTI:
		if (*pFuxiDriverInterface)
		{
			(*pFuxiDriverInterface)->~FuxiDriverInterface();
			*pFuxiDriverInterface = new (*pFuxiDriverInterface)FuxiDriverInterface_Win();
		}
		else
			*pFuxiDriverInterface = new FuxiDriverInterface_Win();
		break;
	case WIN_NETADAPTER:
		if (*pFuxiDriverInterface)
		{
			(*pFuxiDriverInterface)->~FuxiDriverInterface();
			*pFuxiDriverInterface = new (*pFuxiDriverInterface)FuxiNetAdapterDriverInterface_Win();
		}
		else
			*pFuxiDriverInterface = new FuxiNetAdapterDriverInterface_Win();
		break;
	default:
		break;
	}

#else

	switch (type)
	{
	case LINUX_DEBUGFS:
		*pFuxiDriverInterface = new FuxiDriverInterface_Linux();
		break;
	case LINUX_SOCKET:
		*pFuxiDriverInterface = new FuxiSocketInterface_Linux();
		break;
	case LINUX_MEM_MAP:
		*pFuxiDriverInterface = new FuxiMemInterface_Linux();
		break;
	default:
		break;
}

#endif // _WIN32

	return 0;
}

FUXI_DRIVER_INTERFACE_TYPE FuxiDriverInterfaceFactory::getDriverInterfaceType(uint8_t index)
{
#ifdef _WIN32

	FUXI_DRIVER_INTERFACE_TYPE type = FUXI_DRIVER_INTERFACE_TYPE::WIN_NIDS_MULTI;

	std::basic_string<TCHAR> tcsDriverVersion(_T(""));
	if (m_pFuxiDeviceHelper->getDriverVersion(index, tcsDriverVersion) != 0)
		return type;

	bool bNetAdapterExists = false;
	HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_FUXI_PCIE, NULL, NULL, DIGCF_DEVICEINTERFACE);

	if (hDevInfo != INVALID_HANDLE_VALUE)
	{
		SP_DEVINFO_DATA devInfoData = { 0 };
		devInfoData.cbSize = sizeof(devInfoData);
		if (SetupDiEnumDeviceInfo(hDevInfo, 0, &devInfoData))
			bNetAdapterExists = true;
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);

	if (bNetAdapterExists && (_tcscmp(tcsDriverVersion.c_str(), WIN_NETADAPTER_MIN_DRV_VER) >= 0))
		type = FUXI_DRIVER_INTERFACE_TYPE::WIN_NETADAPTER;
	else if (_tcscmp(tcsDriverVersion.c_str(), WIN_NDIS_MULTI_MIN_DRV_VER) >= 0)
		type = FUXI_DRIVER_INTERFACE_TYPE::WIN_NIDS_MULTI;
	else
		type = FUXI_DRIVER_INTERFACE_TYPE::WIN_NIDS;

#else

	FUXI_DRIVER_INTERFACE_TYPE type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_SOCKET;

	std::basic_string<TCHAR> tcsDriverVersion(_T(""));
	if (m_pFuxiDeviceHelper->getDriverVersion(index, tcsDriverVersion) != 0)
		return type;

#ifdef YT_USE_MEM_MAP
	type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_MEM_MAP;
	return type;
#endif // YT_USE_MEM_MAP

#ifdef YT_USE_DEBUGFS
	type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_DEBUGFS;
	return type;
#endif // YT_USE_DEBUGFS

#ifdef YT_USE_SOCKET
	type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_SOCKET;
	return type;
#endif // YT_USE_SOCKET

	if (_tcscmp(tcsDriverVersion.c_str(), LINUX_SOCKET_MIN_DRV_VER) >= 0)
		type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_SOCKET;
	else
		type = FUXI_DRIVER_INTERFACE_TYPE::LINUX_DEBUGFS;

#endif // _WIN32

	return type;
}

int FuxiDriverInterfaceFactory::initDriverInterface(FUXI_DRIVER_INTERFACE_TYPE type, FuxiDriverInterface* pFuxiDriverInterface, uint8_t index)
{
	int ret = 0;

	bool bByDefault = false;
	bool bByIndex = false;
	bool bByMemBaseAddr = false;

#ifdef _WIN32
	if (type == FUXI_DRIVER_INTERFACE_TYPE::WIN_NIDS)
		bByDefault = true;
#else
	if (type == FUXI_DRIVER_INTERFACE_TYPE::LINUX_DEBUGFS)
		bByIndex = true;
	else if (type == FUXI_DRIVER_INTERFACE_TYPE::LINUX_MEM_MAP)
		bByMemBaseAddr = true;
#endif // _WIN32

	if (bByDefault)
		return initDriverInterfaceByDefault(pFuxiDriverInterface);
	else if (bByIndex)
		return initDriverInterfaceByIndex(pFuxiDriverInterface, index);
	else if (bByMemBaseAddr)
		return initDriverInterfaceByMemBase(pFuxiDriverInterface, index);
	else
		return initDriverInterfaceByBDF(pFuxiDriverInterface, index);

	return ret;
}

int FuxiDriverInterfaceFactory::initDriverInterfaceByDefault(FuxiDriverInterface* pFuxiDriverInterface)
{
	if (pFuxiDriverInterface == nullptr)
		return -1;

	return pFuxiDriverInterface->open(true);
}

int FuxiDriverInterfaceFactory::initDriverInterfaceByIndex(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index)
{
	if (pFuxiDriverInterface == nullptr)
		return -1;

	return pFuxiDriverInterface->open(index);
}

int FuxiDriverInterfaceFactory::initDriverInterfaceByBDF(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index)
{
	if (pFuxiDriverInterface == nullptr || m_pFuxiDeviceHelper == nullptr)
		return -1;

	uint8_t targetBus = 0, targetDevice = 0, targetFunction = 0;
	if (m_pFuxiDeviceHelper->getDeviceBDF(index, targetBus, targetDevice, targetFunction) != 0)
		return -2;

	for (uint8_t index = 0; index < FUXI_MAX_USER_MODE_COUNT; index++)
	{
		if (pFuxiDriverInterface->open(index) != 0)
			continue;

		std::wstring wcsDeviceLoaction(L"");
		if (pFuxiDriverInterface->getDeviceLocation(wcsDeviceLoaction) == 0)
		{
			uint8_t currentBus = 0, currentDevice = 0, currentFunction = 0;
			if (parseBDFfromLocationStr(wcsDeviceLoaction, currentBus, currentDevice, currentFunction) == 0)
			{
				if (targetBus == currentBus && targetDevice == currentDevice && targetFunction == currentFunction)
					return 0;			
			}
		}

		pFuxiDriverInterface->close();
	}

	return 1;
}

int FuxiDriverInterfaceFactory::initDriverInterfaceByMemBase(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index)
{
	int ret = 0;

	if (pFuxiDriverInterface == nullptr || m_pFuxiDeviceHelper == nullptr)
		return -1;

	uint64_t uMemBaseAddr = 0;
	if ((ret = m_pFuxiDeviceHelper->getPCIEMemBaseAddr(index, uMemBaseAddr)) != 0)
		return ret;

	return pFuxiDriverInterface->open(uMemBaseAddr);
}

int FuxiDriverInterfaceFactory::parseBDFfromLocationStr(const std::wstring& wcsLoaction, uint8_t& bus, uint8_t& device, uint8_t& function)
{
	std::wstring tcsLocationRegex(L"[^0-9]+(\\d+)[^0-9]+(\\d+)[^0-9]+(\\d+)");

	std::wregex bdf_re(tcsLocationRegex);

	std::wsmatch smatchResult;

	if (std::regex_match(wcsLoaction, smatchResult, bdf_re))
	{
		if (smatchResult.size() != 4)
			return -2;

		bus = (uint8_t)wcstoul(smatchResult[1].str().c_str(), NULL, 10);
		device = (uint8_t)wcstoul(smatchResult[2].str().c_str(), NULL, 10);
		function = (uint8_t)wcstoul(smatchResult[3].str().c_str(), NULL, 10);
	}
	else
		return -1;

	return 0;
}