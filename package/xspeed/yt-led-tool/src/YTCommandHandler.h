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

#include "utility/CrossPlatformUtility.h"
#include "application/CmdHelper.h"


#include "fuxi/FuxiDeviceManager.h"
#include "fuxi/FuxiConfigFileManager.h"


class YTCommandHandler
{
public:
	YTCommandHandler();
	virtual ~YTCommandHandler();

	virtual int initialize(CmdHelper<TCHAR>& cmdHelper);
	virtual void release();

	virtual int handleCommand();

protected:

	virtual void displayHelp();
	virtual int displayDevice();

	virtual bool hasAvailableCommand();
	virtual bool hasFunctionalCommand();
	virtual bool hasOnlyGlobalCommand();
	virtual bool needRestartDevice();

	virtual int handleFunctionalCommand(uint8_t devIndex);
	virtual int handleManagerCommand(uint8_t devIndex);
	virtual int handleGlobalCommand();

	virtual int handleMACCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleVirtualMACCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleSubsystemIDCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleWOLCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleLEDSolutionCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleLEDApplyCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleLEDExportCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleLEDSimulateCommand(FuxiDeviceInterface* pFuxiDeviceInterface);
	virtual int handleEFuseCommand(FuxiDeviceInterface* pFuxiDeviceInterface);

	virtual int enableDevice(uint8_t devIndex);
	virtual int disableDevice(uint8_t devIndex);
	virtual int restartDevice(uint8_t devIndex);
	virtual int rescanDeviceManager();
	virtual int uninstallDevice(uint8_t devIndex);

	virtual int parseMACAddrfromString(const std::basic_string<TCHAR>& tcsValue, uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], char& separator);
	virtual int parseMACAddrfromString(const std::basic_string<TCHAR>& tcsValue, uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], const std::basic_string<TCHAR>& tcsSeparator);
	virtual int verifyMACAddr(uint8_t(&macAddr1)[MAC_ADDR_BYTE_COUNT], uint8_t(&macAddr2)[MAC_ADDR_BYTE_COUNT]);
	virtual void printfMACAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], char separator);
	virtual void printfMACAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], const std::basic_string<TCHAR>& tcsSeparator);

	virtual int checkConfigFile(bool bCreate);

protected:
	CmdHelper<TCHAR> m_cmdHelper;
	FuxiDeviceManager* m_pFuxiDeviceManager;
	FuxiConfigFileManager* m_pFuxiConfigFileManager;
};

