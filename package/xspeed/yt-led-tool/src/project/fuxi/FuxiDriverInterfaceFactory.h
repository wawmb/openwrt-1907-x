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

#include <string>

#include "fuxi/FuxiDriverInterface.h"
#include "fuxi/FuxiDeviceHelper.h"


#ifdef _WIN32

enum FUXI_DRIVER_INTERFACE_TYPE
{
	WIN_NIDS = 0,
	WIN_NIDS_MULTI = 1,
	WIN_NETADAPTER = 2
};

#else

enum FUXI_DRIVER_INTERFACE_TYPE
{
	LINUX_DEBUGFS = 0,
	LINUX_SOCKET = 1,
	LINUX_MEM_MAP = 2
};

#endif // _WIN32


class FuxiDriverInterfaceFactory
{
public:
	FuxiDriverInterfaceFactory(FuxiDeviceHelper* fuxiDeviceHelper);
	virtual ~FuxiDriverInterfaceFactory();

	virtual int createDriverInterface(uint8_t index, FuxiDriverInterface** pFuxiDriverInterface);
	virtual int createDriverInterface(FUXI_DRIVER_INTERFACE_TYPE type, FuxiDriverInterface** pFuxiDriverInterface);

protected:
	virtual FUXI_DRIVER_INTERFACE_TYPE getDriverInterfaceType(uint8_t index);

	virtual int initDriverInterface(FUXI_DRIVER_INTERFACE_TYPE type, FuxiDriverInterface* pFuxiDriverInterface, uint8_t index);
	virtual int initDriverInterfaceByDefault(FuxiDriverInterface* pFuxiDriverInterface);
	virtual int initDriverInterfaceByIndex(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index);
	virtual int initDriverInterfaceByBDF(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index);
	virtual int initDriverInterfaceByMemBase(FuxiDriverInterface* pFuxiDriverInterface, uint8_t index);

	virtual int parseBDFfromLocationStr(const std::wstring& wcsLoaction, uint8_t& bus, uint8_t& device, uint8_t& function);

protected:
	FuxiDeviceHelper* m_pFuxiDeviceHelper;
};

