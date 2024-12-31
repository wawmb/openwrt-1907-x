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

#include "FuxiDeviceInterface.h"

#include "fuxi/FuxiDriverInterfaceFactory.h"
#ifdef _WIN32
#include "fuxi/windows/FuxiSystemManager_Win.h"
#endif // _WIN32


FuxiDeviceInterface::FuxiDeviceInterface(FuxiDeviceHelper* fuxiDeviceHelper)
	: m_index(0)
	, m_pFuxiDeviceHelper(fuxiDeviceHelper)
	, m_pFuxiDriverInterface(nullptr)
	, m_pFuxiEFuseManager(nullptr)
	, m_pFuxiPatchManager(nullptr)
	, m_pFuxiLEDManager(nullptr)
	, m_pVulcanManager(nullptr)
	, m_fuxiSystemManager(nullptr)
{
}

FuxiDeviceInterface::~FuxiDeviceInterface()
{
}

int FuxiDeviceInterface::initialize(uint8_t index)
{
	int ret = 0;

	m_index = index;

	if (m_pFuxiDeviceHelper == nullptr)
		return -1;

	release();

	if ((ret = loadDriverInterface()) != 0)
	{
		printf("Device %u loadDriverInterface failed=%d\n", index, ret);
		return ret;
	}
		

#ifndef YT_USE_MEM_MAP

	m_pFuxiEFuseManager = new FuxiEFuseManager();
	if ((ret = m_pFuxiEFuseManager->initialize(m_pFuxiDriverInterface)) != 0)
	{
		printf("Device %u initialize EFuseManager failed=%d\n", index, ret);
		return ret;
	}

	m_pFuxiPatchManager = new FuxiPatchManager();
	if ((ret = m_pFuxiPatchManager->initialize(m_pFuxiEFuseManager)) != 0)
	{
		printf("Device %u initialize PatchManager failed=%d\n", index, ret);
		return ret;
	}

	m_pFuxiLEDManager = new FuxiLEDManager();
	if ((ret = m_pFuxiLEDManager->initialize(m_pFuxiEFuseManager)) != 0)
	{
		printf("Device %u initialize LEDManager failed=%d\n", index, ret);
		return ret;
	}

	//m_pFuxiConfigFileManager = new FuxiConfigFileManager();
	//m_pFuxiConfigFileManager->initialize(bus, device, function);

#endif // YT_USE_MEM_MAP

	m_pVulcanManager = new VulcanManager();
	if ((ret = m_pVulcanManager->initialize(m_pFuxiDriverInterface)) != 0)
	{
		printf("Device %u initialize VulcanManager failed=%d\n", index, ret);
		return ret;
	}

#ifdef _WIN32
	m_fuxiSystemManager = new FuxiSystemManager_Win();
	if ((ret = m_fuxiSystemManager->initialize(index, m_pFuxiDeviceHelper)) != 0)
	{
		//printf("Device %u initialize SystemManager failed=%d\n", index, ret);
		//return ret;
	}
#endif // _WIN32
	

	return ret;
}

void FuxiDeviceInterface::release()
{
	if (m_fuxiSystemManager)
	{
		m_fuxiSystemManager->release();
		delete m_fuxiSystemManager;
		m_fuxiSystemManager = nullptr;
	}

	if (m_pVulcanManager)
	{
		m_pVulcanManager->release();
		delete m_pVulcanManager;
		m_pVulcanManager = nullptr;
	}

	if (m_pFuxiLEDManager)
	{
		m_pFuxiLEDManager->release();
		delete m_pFuxiLEDManager;
		m_pFuxiLEDManager = nullptr;
	}

	if (m_pFuxiPatchManager)
	{
		m_pFuxiPatchManager->release();
		delete m_pFuxiPatchManager;
		m_pFuxiPatchManager = nullptr;
	}

	if (m_pFuxiEFuseManager)
	{
		m_pFuxiEFuseManager->release();
		delete m_pFuxiEFuseManager;
		m_pFuxiEFuseManager = nullptr;
	}

	if (m_pFuxiDriverInterface)
	{
		m_pFuxiDriverInterface->close();
		delete m_pFuxiDriverInterface;
		m_pFuxiDriverInterface = nullptr;
	}
}

int FuxiDeviceInterface::releaseDriverInterface()
{
	int ret = 0;

	if (m_pFuxiDriverInterface)
	{
		m_pFuxiDriverInterface->close();
	}

	return ret;
}

int FuxiDeviceInterface::loadDriverInterface()
{
	if (m_pFuxiDriverInterface == nullptr || !m_pFuxiDriverInterface->isOpen())
	{
		FuxiDriverInterfaceFactory fuxiDriverInterfaceFactory(m_pFuxiDeviceHelper);
		if(fuxiDriverInterfaceFactory.createDriverInterface(m_index, &m_pFuxiDriverInterface) == 0)
			return 0;
	}

	return 1;
}

FuxiDeviceHelper* FuxiDeviceInterface::getDeviceHelper()
{
	return m_pFuxiDeviceHelper;
}

FuxiDriverInterface* FuxiDeviceInterface::getDriverInterface()
{
	return m_pFuxiDriverInterface;
}

FuxiEFuseManager* FuxiDeviceInterface::getEFuseManager()
{
	return m_pFuxiEFuseManager;
}

FuxiPatchManager* FuxiDeviceInterface::getPatchManager()
{
	return m_pFuxiPatchManager;
}

FuxiLEDManager* FuxiDeviceInterface::getLEDManager()
{
	return m_pFuxiLEDManager;
}

VulcanManager* FuxiDeviceInterface::getVulcanManager()
{
	return m_pVulcanManager;
}

FuxiSystemManager* FuxiDeviceInterface::getSystemManager()
{
	return m_fuxiSystemManager;
}