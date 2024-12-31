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

#include "YTCommandHandler.h"

#include "debug/LogHelper.h"
#include "file/PathHelper.h"

#ifdef _WIN32

#include "fuxi/windows/FuxiDeviceManager_Win.h"

#else

#include "fuxi/linux/FuxiDeviceManager_Linux.h"

#endif // _WIN32


#define     CMD_HELP_1                          _T("-h")
#define     CMD_HELP_2                          _T("-?")

#define     CMD_LIST_NIC                        _T("-listdev")
#define     CMD_NIC_INDEX                       _T("-index")

#define     CMD_ENABLE_DEVICE                   _T("-enable")
#define     CMD_DISABLE_DEVICE                  _T("-disable")
#define     CMD_RESTART_DEVICE                  _T("-restart")
#define     CMD_RESCAN                          _T("-rescan")
#define     CMD_UNINSTALL                       _T("-uninstall")

#define     CMD_MAC_ADDR                        _T("-mac")
#define     CMD_VIRTUAL_MAC_ADDR                _T("-virmac")
#define     CMD_SUBSYS_ID                       _T("-ssid")

#define     CMD_WOL                             _T("-wol")

#define     CMD_LED_SOLUTION                    _T("-led")
#define     CMD_APPLY_LED_CONFIG                _T("-ledapp")
#define     CMD_SIMULATE_LED_CONFIG             _T("-ledsim")
#define     CMD_EXPORT_LED_CONFIG               _T("-ledexp")

#define     CMD_DUMP_EFUSE_REG                  _T("-efuse")

#define     CMD_CONFIRM                         _T("-confirm")
#define     CMD_TAG                             _T("-tag")

#define     CMD_AUTO                            _T("-auto")
#define     CMD_FILE                            _T("-file")

#define     CMD_DELETE                          _T("-del")




YTCommandHandler::YTCommandHandler()
    : m_pFuxiDeviceManager(nullptr)
    , m_pFuxiConfigFileManager(nullptr)
{
}

YTCommandHandler::~YTCommandHandler()
{
}

int YTCommandHandler::initialize(CmdHelper<TCHAR>& cmdHelper)
{
    int ret = 0;

    release();

    m_cmdHelper = cmdHelper;

#ifdef _WIN32
    m_pFuxiDeviceManager = new FuxiDeviceManager_Win();
#else
    m_pFuxiDeviceManager = new FuxiDeviceManager_Linux();
#endif // _WIN32

    if ((ret = m_pFuxiDeviceManager->initialize()) != 0)
    {
        if (!hasOnlyGlobalCommand())
        {
            printf("Error, Initialize device manager failed=%d!\n", ret);
            return -1;
        }
        else
            ret = 0;
    }

    return ret;
}

void YTCommandHandler::release()
{
    if (m_pFuxiConfigFileManager)
    {
        m_pFuxiConfigFileManager->release();
        delete m_pFuxiConfigFileManager;
        m_pFuxiConfigFileManager = nullptr;
    }

    if (m_pFuxiDeviceManager)
    {
        m_pFuxiDeviceManager->release();
        delete m_pFuxiDeviceManager;
        m_pFuxiDeviceManager = nullptr;
    }

    m_pFuxiDeviceManager = nullptr;
}

int YTCommandHandler::handleCommand()
{
    int ret = 0;

    if (m_cmdHelper.hasCmd(CMD_HELP_1) || m_cmdHelper.hasCmd(CMD_HELP_2) || !m_cmdHelper.hasParameter())
    {
        displayHelp();
        return ret;
    }
    else if (hasOnlyGlobalCommand())
    {
        if((ret = handleGlobalCommand()) != 0)
            // LogHelper::printfError("Operation failed=%d!\n", ret);
            printf("Operation failed=%d!\n", ret);
        else
            // LogHelper::printfSuccess("Operation succeeded!\n");
            printf("Operation succeeded!\n");
        return ret;
    }
    else if (m_cmdHelper.hasCmd(CMD_LIST_NIC))
    {
        displayDevice();
        return ret;
    }
    else if (!hasAvailableCommand())
    {
        printf("Error, Please enter the correct parameters, refer to the following instructions:\n");
        displayHelp();
        return ret;
    }
    else // specific device
    {
        int tempRet = 0;

        uint8_t uDevIndex = 0;
        std::basic_string<TCHAR> tcsValue(_T(""));
        if (m_cmdHelper.getCmdValue(CMD_NIC_INDEX, tcsValue) == 0)
            uDevIndex = (uint8_t)_tcstoul(tcsValue.c_str(), NULL, 10);
        
        if (hasFunctionalCommand())
        {
            if ((tempRet = handleFunctionalCommand(uDevIndex)) != 0)
                ret = tempRet;
        }

        if (ret == 0)
        {
            FuxiDeviceInterface* pFuxiDeviceInterface = m_pFuxiDeviceManager->getDeviceInterface(uDevIndex);
            if(pFuxiDeviceInterface)
                pFuxiDeviceInterface->releaseDriverInterface();

            if ((tempRet = handleManagerCommand(uDevIndex)) != 0)
                ret = tempRet;
        }

        if (ret != 0)
            // LogHelper::printfError("Operation failed=%d!\n", ret);
            printf("Operation failed=%d!\n", ret);
        else
            // LogHelper::printfSuccess("Operation succeeded!\n");
            printf("Operation succeeded!\n");
        return ret;
    }

    return ret;
}

void YTCommandHandler::displayHelp()
{
    _tprintf(_T("\n[Command Option] (Case insensitive)\n"));

    _tprintf(_T("%-20s : %s\n"), CMD_HELP_1, _T("Show help message"));
    _tprintf(_T("%-20s : %s\n"), CMD_HELP_2, _T("Show help message"));

    _tprintf(_T("%-20s : %s\n"), CMD_LIST_NIC, _T("List all YT6801/YT6801S devices on your machine."));

    _tprintf(_T("%s %-13s : %s\n"), CMD_NIC_INDEX, _T("[value]"), _T("Specify the network card index(decimal from 0) to operate. eg: -index 0"));

#ifdef _WIN32
    _tprintf(_T("%-20s : %s\n"), CMD_RESTART_DEVICE, _T("Restart specific device. eg: -index 0 -restart"));
    _tprintf(_T("%-20s : %s\n"), CMD_ENABLE_DEVICE, _T("Enable specific device. eg: -index 0 -enable"));
    _tprintf(_T("%-20s : %s\n"), CMD_DISABLE_DEVICE, _T("Disable specific device. eg: -index 0 -disable"));
    _tprintf(_T("%-20s : %s\n"), CMD_RESCAN, _T("Rescan Windows Device Manager."));
    _tprintf(_T("%-20s : %s\n"), CMD_UNINSTALL, _T("Uninstall specific device(will not uninstall driver, use -rescan to reinstall). eg: -index 0 -uninstall"));
#endif // !_WIN32

    _tprintf(_T("%s %-15s : %s\n"), CMD_MAC_ADDR, _T("[value]"), _T("Burn MAC address. eg: -index 0 -mac 20:C1:9B:0D:C3:C0"));
    _tprintf(_T("%s %-12s : %s\n"), CMD_VIRTUAL_MAC_ADDR, _T("[value]"), _T("Config virtual MAC address. eg: -index 0 -virmac 20:C1:9B:0D:C3:C0"));
    _tprintf(_T("%s %-14s : %s\n"), CMD_SUBSYS_ID, _T("[value]"), _T("Burn subsystem ID (hexadecimal, high 2 bytes is subdevice ID). eg: -index 0 -ssid 0x98062066."));

    _tprintf(_T("%s %-15s : %s\n"), CMD_WOL, _T("[value]"), _T("Read/Write WOL status from/to eFuse. eg: -index 0 -wol 1."));
    _tprintf(_T("%s %-15s : %s\n"), CMD_LED_SOLUTION, _T("[value]"), _T("Read/Write LED solution from/to eFuse. eg: -index 0 -led 0x1F."));

    _tprintf(_T("%-20s : %s\n"), CMD_APPLY_LED_CONFIG, _T("Apply LED configuration from confg file(default use yt6801.cfg) to eFuse."));
    _tprintf(_T("%-20s : %s\n"), CMD_EXPORT_LED_CONFIG, _T("Export LED configuration to confg file(default use yt6801.cfg) from eFuse."));

    _tprintf(_T("%-20s : %s\n"), CMD_SIMULATE_LED_CONFIG, _T("Simulate LED configuration from config file(default use yt6801.cfg)."));


    _tprintf(_T("%-20s : %s\n"), CMD_DUMP_EFUSE_REG, _T("Dump all eFuse registers."));
#ifdef _WIN32
    _tprintf(_T("%-20s : %s\n"), CMD_TAG, _T("Combine with -mac/-virmac command, specify MAC address separator."));
#endif // !_WIN32

    _tprintf(_T("%-20s : %s\n"), CMD_CONFIRM, _T("Combine with -ledapp command, skip confirmation prompt steps."));
    _tprintf(_T("%-20s : %s\n"), CMD_FILE, _T("Combine with other command, specific config file(absolute path) to operate."));
    _tprintf(_T("%-20s : %s\n"), CMD_DELETE, _T("Delete specific information."));
}

int YTCommandHandler::displayDevice()
{
    int ret = 0;

    printf("Device list:\n");
    for (uint8_t i = 0; i < m_pFuxiDeviceManager->deviceCount(); i++)
    {
        std::basic_string<TCHAR> tcsDisplayName;
        m_pFuxiDeviceManager->getDeviceDisplayName(i, tcsDisplayName);

        PCIE_BDF pcieBDF = { 0 };
        m_pFuxiDeviceManager->getDevicePcieBDF(i, pcieBDF);

        if (m_pFuxiDeviceManager->getDeviceInterface(i))
        {
            std::basic_string<TCHAR> tcsDriverVersion(_T(""));
            m_pFuxiDeviceManager->getDriverVersion(i, tcsDriverVersion);

            _tprintf(_T("[Index:%u] %s (Bus %u, device %u, function %u) (Driver:%s)\n"), i, tcsDisplayName.c_str(), pcieBDF.bus, pcieBDF.device, pcieBDF.function, tcsDriverVersion.c_str());
        }
        else
            LogHelper::printfWarning(_T("[Index:%u] %s (Bus %u, device %u, function %u) -Not Available\n"), i, tcsDisplayName.c_str(), pcieBDF.bus, pcieBDF.device, pcieBDF.function);
    }

    return ret;
}

bool YTCommandHandler::hasAvailableCommand()
{
    if (m_cmdHelper.hasCmd(CMD_ENABLE_DEVICE)
        || m_cmdHelper.hasCmd(CMD_DISABLE_DEVICE)
        || m_cmdHelper.hasCmd(CMD_RESTART_DEVICE)
        || m_cmdHelper.hasCmd(CMD_RESCAN)
        || m_cmdHelper.hasCmd(CMD_UNINSTALL)
        || hasFunctionalCommand())
        return true;

    return false;
}

bool YTCommandHandler::needRestartDevice()
{
    if (m_cmdHelper.hasCmd(CMD_RESTART_DEVICE))
        return true;

#ifdef _WIN32
    if (m_cmdHelper.hasCmd(CMD_SIMULATE_LED_CONFIG))
        return true;
#endif // _WIN32

    return false;
}

bool YTCommandHandler::hasFunctionalCommand()
{
    if (   m_cmdHelper.hasCmd(CMD_MAC_ADDR)
        || m_cmdHelper.hasCmd(CMD_VIRTUAL_MAC_ADDR)
        || m_cmdHelper.hasCmd(CMD_SUBSYS_ID)
        || m_cmdHelper.hasCmd(CMD_WOL)
        || m_cmdHelper.hasCmd(CMD_LED_SOLUTION)
        || m_cmdHelper.hasCmd(CMD_APPLY_LED_CONFIG)
        || m_cmdHelper.hasCmd(CMD_EXPORT_LED_CONFIG)
        || m_cmdHelper.hasCmd(CMD_SIMULATE_LED_CONFIG)
        || m_cmdHelper.hasCmd(CMD_DUMP_EFUSE_REG)
#ifndef _WIN32
        || m_cmdHelper.hasCmd(CMD_SIMULATE_LED_CONFIG)
#endif // !_WIN32
        )
        return true;

    return false;
}

bool YTCommandHandler::hasOnlyGlobalCommand()
{
#ifdef _WIN32

    if ((m_cmdHelper.parameterCount() == 2) &&
        m_cmdHelper.hasCmd(CMD_RESCAN))
        return true;

#endif // !_WIN32

    return false;
}

int YTCommandHandler::handleFunctionalCommand(uint8_t devIndex)
{
    int ret = 0;

    FuxiDeviceInterface* pFuxiDeviceInterface = m_pFuxiDeviceManager->getDeviceInterface(devIndex);
    if (pFuxiDeviceInterface == nullptr)
    {
        printf("Error, device(index=%u) is not available!\n", devIndex);
        return -1;
    }

    if (m_cmdHelper.hasCmd(CMD_MAC_ADDR))
        ret = handleMACCommand(pFuxiDeviceInterface); 
    else if (m_cmdHelper.hasCmd(CMD_VIRTUAL_MAC_ADDR))
        ret = handleVirtualMACCommand(pFuxiDeviceInterface);
    else if (m_cmdHelper.hasCmd(CMD_SUBSYS_ID))
        ret = handleSubsystemIDCommand(pFuxiDeviceInterface);
    else if(m_cmdHelper.hasCmd(CMD_WOL))
        ret = handleWOLCommand(pFuxiDeviceInterface);
    else if(m_cmdHelper.hasCmd(CMD_LED_SOLUTION))
        ret = handleLEDSolutionCommand(pFuxiDeviceInterface);
    else if (m_cmdHelper.hasCmd(CMD_APPLY_LED_CONFIG))
        ret = handleLEDApplyCommand(pFuxiDeviceInterface); 
    else if (m_cmdHelper.hasCmd(CMD_EXPORT_LED_CONFIG))
        ret = handleLEDExportCommand(pFuxiDeviceInterface);
    else if (m_cmdHelper.hasCmd(CMD_SIMULATE_LED_CONFIG))
        ret = handleLEDSimulateCommand(pFuxiDeviceInterface);
    else if (m_cmdHelper.hasCmd(CMD_DUMP_EFUSE_REG))
        ret = handleEFuseCommand(pFuxiDeviceInterface);

    return ret;
}

int YTCommandHandler::handleManagerCommand(uint8_t devIndex)
{
    int ret = 0;

#ifdef _WIN32
    if (needRestartDevice())
    {
        if ((ret = restartDevice(devIndex)) != 0)
            return ret;
    }
    else if (m_cmdHelper.hasCmd(CMD_ENABLE_DEVICE))
    {
        if ((ret = enableDevice(devIndex)) != 0)
            return ret;
    }
    else if (m_cmdHelper.hasCmd(CMD_DISABLE_DEVICE))
    {
        if ((ret = disableDevice(devIndex)) != 0)
            return ret;
    }
    else if (m_cmdHelper.hasCmd(CMD_UNINSTALL))
    {
        if ((ret = uninstallDevice(devIndex)) != 0)
            return ret;
    }
    else if (m_cmdHelper.hasCmd(CMD_RESCAN))
    {
        return rescanDeviceManager();
    }
#else
    ret = 0;
#endif // _WIN32

    return ret;
}

int YTCommandHandler::handleGlobalCommand()
{
    int ret = 1;

    if (m_pFuxiDeviceManager == nullptr)
        return -1;

#ifdef WIN32

    if (m_cmdHelper.hasCmd(CMD_RESCAN))
    {
        if ((ret = m_pFuxiDeviceManager->installDevice(0)) != 0)
            printf("Error, rescan device manager failed!\n");
    }
    else
        printf("Error, Unknown command!\n");

#endif // WIN32

    return ret;
}

int YTCommandHandler::handleMACCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiEFuseManager* pFuxiEFuseManager = pFuxiDeviceInterface->getEFuseManager();

    uint8_t macAddr[MAC_ADDR_BYTE_COUNT] = { 0 };
    if ((ret = pFuxiEFuseManager->readMACAddr(macAddr)) != 0)
    {
        printf("Error, read MAC address failed=%d\n", ret);
        return ret;
    }

    std::basic_string<TCHAR> tcsValue(_T(""));
    m_cmdHelper.getCmdValue(CMD_MAC_ADDR, tcsValue);

    std::basic_string<TCHAR> tcsMacAddrSeparator = _T(":");
    if (m_cmdHelper.hasCmd(CMD_TAG))
        m_cmdHelper.getCmdValue(CMD_TAG, tcsMacAddrSeparator);

    if (tcsValue.empty())
    {
        printf("Current MAC address: "); printfMACAddress(macAddr, tcsMacAddrSeparator);
        return 0;
    }

    uint8_t inputMACAddr[MAC_ADDR_BYTE_COUNT] = { 0 };
    
    if ((ret = parseMACAddrfromString(tcsValue, inputMACAddr, tcsMacAddrSeparator)) != 0 || !pFuxiEFuseManager->isMACAddrLegal(inputMACAddr))
    {
        _tprintf(_T("Error, MAC address %s is not available!\n"), tcsValue.c_str());
        return -1;
    }

    // check MAC address
    if (!pFuxiEFuseManager->isMACAddrLegal(inputMACAddr))
    {
        printf("Error, illegal MAC address!\n");
        return -2;
    }

    // check exists
    if (verifyMACAddr(macAddr, inputMACAddr) == 0)
    {
        _tprintf(_T("Warning, MAC address %s has alreay burned, do nothing!\n"), tcsValue.c_str());
        return 0;
    }

    if ((ret = pFuxiEFuseManager->writeMACAddr(inputMACAddr)) != 0)
    {
        _tprintf(_T("Error, Burn MAC address %s failed=%d!\n"), tcsValue.c_str(), ret);
        return ret;
    }
    else
        _tprintf(_T("Success, Burn MAC address %s!\n"), tcsValue.c_str());

    if ((ret = pFuxiEFuseManager->RefreshPatchItems()) != 0)
    {
        printf("Error, refresh eFuse failed!\n");
        return ret;
    }

    // verify
    if ((ret = pFuxiEFuseManager->readMACAddr(macAddr)) != 0)
    {
        printf("Error, read MAC address failed=%d\n", ret);
        return ret;
    }

    if (verifyMACAddr(macAddr, inputMACAddr) != 0)
    {
        _tprintf(_T("Error, Verify MAC address %s failed!\n"), tcsValue.c_str());
        printf("Current MAC address: "); printfMACAddress(macAddr, tcsMacAddrSeparator);
        ret = 1;
    }
    else
        _tprintf(_T("Verify MAC address %s succeeded!\n"), tcsValue.c_str());
    
    return ret;
}

int YTCommandHandler::handleVirtualMACCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiSystemManager* pFuxiSystemManager = pFuxiDeviceInterface->getSystemManager();

    if (pFuxiSystemManager == nullptr) 
    {
        printf("Error, get system manager failed=%d\n", ret);
        return ret;
    }

    if (m_cmdHelper.hasCmd(CMD_DELETE))
    {
        if ((ret = pFuxiSystemManager->removeVirtualMacAddress()) != 0)
        {
            printf("Error, delete virtual MAC address failed=%d\n", ret);
            return ret;
        }
        else
        {
            printf("Success, delete virtual MAC address.\n");
            return 0;
        }
    }

    std::basic_string<TCHAR> tcsMacAddrSeparator(_T(":"));
    if (m_cmdHelper.hasCmd(CMD_TAG))
        m_cmdHelper.getCmdValue(CMD_TAG, tcsMacAddrSeparator);

    std::basic_string<TCHAR> tcsValue(_T(""));
    m_cmdHelper.getCmdValue(CMD_VIRTUAL_MAC_ADDR, tcsValue);

    uint8_t macAddr[MAC_ADDR_BYTE_COUNT] = { 0 };
    if (tcsValue.empty())
    {
        if ((ret = pFuxiSystemManager->getVirtualMacAddress(macAddr)) != 0)
        {
            printf("Virtual MAC address is not used!\n");
            return 0;
        }
        else
        {
            printf("Current virtual MAC address: "); printfMACAddress(macAddr, tcsMacAddrSeparator);
            return 0;
        }
    }
    else
    {
        if ((ret = parseMACAddrfromString(tcsValue, macAddr, tcsMacAddrSeparator)) != 0)
        {
            _tprintf(_T("Error, input virtual MAC address %s is not available!\n"), tcsValue.c_str());
            return -1;
        }
        
        if ((ret = pFuxiSystemManager->setVirtualMacAddress(macAddr)) != 0)
        {
            printf("Error, config virtual MAC address failed=%d\n", ret);
            return ret;
        }
        else
        {
            _tprintf(_T("Success, config virtual MAC address to %s.\n"), tcsValue.c_str());
            return 0;
        }
    }

    return ret;
}

int YTCommandHandler::handleSubsystemIDCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiEFuseManager* pFuxiEFuseManager = pFuxiDeviceInterface->getEFuseManager();

    uint32_t uSubVendorID = 0, uSubDeviceID = 0;
    if ((ret = pFuxiEFuseManager->readSubSysInfo(uSubDeviceID, uSubVendorID)) != 0)
    {
        printf("Error, read subsystem information failed=%d\n", ret);
        return ret;
    }

    bool bRead = true;
    uint32_t uInputSubsystemID = 0;

    if (m_cmdHelper.hasCmd(CMD_FILE))
    {
        if ((ret = checkConfigFile(false)) != 0)
            return ret;

        if ((ret = m_pFuxiConfigFileManager->getSubsystemID(uInputSubsystemID)) != 0)
        {
            printf("Error, read subsystem ID from config file failed=%d\n", ret);
            return ret;
        }

        bRead = false;
    }
    else
    {
        std::basic_string<TCHAR> wcsValue(_T(""));
        m_cmdHelper.getCmdValue(CMD_SUBSYS_ID, wcsValue);

        if (!wcsValue.empty())
        {
            bRead = false;
            uInputSubsystemID = (uint32_t)_tcstoul(wcsValue.c_str(), NULL, 16);
        } 
    }

    if (bRead)
    {
        printf("Current subsystem ID=0x%04X%04X\n", uSubDeviceID, uSubVendorID);
        return 0;
    }

    uint32_t uInputSubDeviceID = (uInputSubsystemID & 0xFFFF0000) >> 16;
    uint32_t uInputSubVendorID = uInputSubsystemID & 0x0000FFFF;

    // check avaliable
    if (uInputSubDeviceID == 0 || uInputSubVendorID == 0)
    {
        _tprintf(_T("Error, Sybsystem ID 0x%08X is not available.\n"), uInputSubsystemID);
        return -1;
    }

    // check exists
    if (uSubDeviceID == uInputSubDeviceID &&
        uSubVendorID == uInputSubVendorID)
    {
        _tprintf(_T("Subsystem ID 0x%08X has alreay burned, do nothing!\n"), uInputSubsystemID);
        return 0;
    }

    if ((ret = pFuxiEFuseManager->writeSubSysInfo(uInputSubDeviceID, uInputSubVendorID)) != 0)
    {
        _tprintf(_T("Error, Burn subsystem ID 0x%08X failed=%d!\n"), uInputSubsystemID, ret);
        return ret;
    }
    else
        _tprintf(_T("Success, Burn subsystem ID 0x%08X!\n"), uInputSubsystemID);

    if ((ret = pFuxiEFuseManager->RefreshPatchItems()) != 0)
    {
        printf("Error, refresh eFuse failed=%d!\n", ret);
        return ret;
    }

    if ((ret = pFuxiEFuseManager->readSubSysInfo(uSubDeviceID, uSubVendorID)) != 0)
    {
        printf("Error, read subsystem information failed=%d\n", ret);
        return ret;
    }

    if (uSubDeviceID != uInputSubDeviceID ||
        uSubVendorID != uInputSubVendorID)
    {
        printf("Error, verify subsystem information failed!\n");
        return -1;
    }
    else
        printf("Verify subsystem information succeeded!\n");

    return ret;
}

int YTCommandHandler::handleWOLCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiEFuseManager* pFuxiEFuseManager = pFuxiDeviceInterface->getEFuseManager();

    if (pFuxiEFuseManager == nullptr)
        return -1;

   
    std::basic_string<TCHAR> tcsValue(_T(""));
    m_cmdHelper.getCmdValue(CMD_WOL, tcsValue);

    bool bCurrentWOLEnable = false;
    if ((ret = pFuxiEFuseManager->ReadWOLStatus(bCurrentWOLEnable)) != 0)
    {
        printf("Error, read WOL status failed=%d!\n", ret);
        return ret;
    }

    if (!tcsValue.empty())
    {
        if (bCurrentWOLEnable)
        {
            printf("Warning, WOL has alreay enabled, do nothing!\n");
            return 0;
        }

        if ((ret = pFuxiEFuseManager->WriteWOLStatus(true)) != 0)
        {
            printf("Error, enable WOL failed=%d!\n", ret);
            return ret;
        }
        else
            printf("Enable WOL succeed!\n");
        
        if ((ret = pFuxiEFuseManager->ReadWOLStatus(bCurrentWOLEnable)) != 0)
        {
            printf("Error, read WOL status failed=%d, verify failed!\n", ret);
            return ret;
        }

        if (!bCurrentWOLEnable)
        {
            printf("Error, verify WOL status failed, WOL is still disabled!\n");
            return ret;
        }
        else
            printf("Verify WOL succeed!\n");
    }
    else
    {
        printf("Current WOL status: %s\n", bCurrentWOLEnable? "Enabled":"Disabled");
        return 0;
    }

    return ret;
}

int YTCommandHandler::handleLEDSolutionCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiLEDManager* pFuxiLEDManager = pFuxiDeviceInterface->getLEDManager();

    if (pFuxiLEDManager == nullptr)
        return -1;

    uint8_t uLEDSolution = 0;
    std::basic_string<TCHAR> tcsValue(_T(""));
    m_cmdHelper.getCmdValue(CMD_LED_SOLUTION, tcsValue);

    uint8_t uCurrentLEDSolution = 0;
    if ((ret = pFuxiLEDManager->readLEDSolution(uCurrentLEDSolution)) != 0)
    {
        printf("Error, read LED solution failed=%d!\n", ret);
        return ret;
    }

    if (!tcsValue.empty())
    {
        uLEDSolution = (uint8_t)_tcstoul(tcsValue.c_str(), NULL, 16);

        if (uCurrentLEDSolution == uLEDSolution)
        {
            _tprintf(_T("Warning, LED solution %s has alreay burned, do nothing!\n"), tcsValue.c_str());
            return 0;
        }

        if (uCurrentLEDSolution != 0)
        {
            _tprintf(_T("Error, LED solution has alreay burned to 0x%X, can't be modified!\n"), uCurrentLEDSolution);
            return 1;
        }

        if ((ret = pFuxiLEDManager->applyLEDSolution(uLEDSolution)) != 0)
        {
            _tprintf(_T("Error, write LED solution %s failed=%d!\n"), tcsValue.c_str(), ret);
            return ret;
        }
        else
            _tprintf(_T("Write LED solution %s succeeded!\n"), tcsValue.c_str());

        if ((ret = pFuxiLEDManager->readLEDSolution(uCurrentLEDSolution)) != 0)
        {
            printf("Error, read LED solution failed=%d, verify failed!\n", ret);
            return ret;
        }

        if (uCurrentLEDSolution == uLEDSolution)
        {
            _tprintf(_T("Verify LED solution succeeded!\n"));
            return 0;
        }
        else
        {
            _tprintf(_T("Verify LED solution failed, current LED solution=0x%X!\n"), uCurrentLEDSolution);
            return 2;
        }
    }
    else
        _tprintf(_T("Current LED solution: 0x%X\n"), uCurrentLEDSolution);
    
    return ret;
}

int YTCommandHandler::handleLEDApplyCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    if ((ret = checkConfigFile(false)) != 0)
        return ret;

    FuxiLEDManager* pFuxiLEDManager = pFuxiDeviceInterface->getLEDManager();

    uint8_t uWrittenCount = 0;
    if ((ret = pFuxiLEDManager->getConfigWrittenCount(uWrittenCount)) != 0)
    {
        printf("Error, get LED written count failed=%d!\n", ret);
        return ret;
    }
    
    uint8_t uMaxWrittenCount = pFuxiLEDManager->getConfigMaxWriteCount();
    if (uMaxWrittenCount <= uWrittenCount)
    {
        printf("Error, The maximum number(%u) of write LED configurations has been exceeded!\n", uMaxWrittenCount);
        return -1;
    }
    else if((uMaxWrittenCount - uWrittenCount) == 1)
    {
        if (!m_cmdHelper.hasCmd(CMD_CONFIRM))
        {
            printf("Warning, There is only one opportunity left to write the LED configuration, are you sure to continue(y/n) ? \n");
            char cConfirm = (char)getchar();
            if (cConfirm != 'y' && cConfirm != 'Y')
                return 1;
        }    
    }

    led_setting inputLEDSetting = { 0 };
    if ((ret = m_pFuxiConfigFileManager->getLEDSetting(inputLEDSetting)) != 0)
    {
        printf("Error, get LED configuration from file failed=%d!\n", ret);
        return ret;
    }

    if (uWrittenCount > 0 && pFuxiLEDManager->verifyLEDSetting(inputLEDSetting) == 0)
    {
        printf("Warning, The LED setting in your yt6801.cfg file has already been applied, do nothing!\n");
        return 1;
    }

    if ((ret = pFuxiLEDManager->applyLEDSetting(inputLEDSetting)) != 0)
    {
        printf("Error, apply LED configuration failed=%d!\n", ret);
        return ret;
    }
    else
        printf("Success, apply LED configuration!\n");

    if (pFuxiLEDManager->verifyLEDSetting(inputLEDSetting) != 0)
    {
        printf("Error, verify LED setting failed!\n");
        return -1;
    }
    else
        printf("Success, verify LED setting pass!\n");

    return ret;
}

int YTCommandHandler::handleLEDExportCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    if ((ret = checkConfigFile(true)) != 0)
        return ret;
    
    FuxiLEDManager* pFuxiLEDManager = pFuxiDeviceInterface->getLEDManager();

    if (pFuxiLEDManager == nullptr)
        return -1;

    uint8_t uWrittenCount = 0;
    if ((ret = pFuxiLEDManager->getConfigWrittenCount(uWrittenCount)) != 0)
    {
        printf("Error, get LED setting written count failed=%d!\n", ret);
        return ret;
    }
    
    if (uWrittenCount == 0)
    {
        printf("Warning, there is no LED setting in efuse!\n");
        return 1;
    }

    led_setting eFuseLEDSetting = { 0 };
    if ((ret = pFuxiLEDManager->loadLEDSettingfromeFuse(eFuseLEDSetting)) != 0)
    {
        printf("Error, load LED setting from eFuse failed=%d!\n", ret);
        return ret;
    }

    if ((ret = m_pFuxiConfigFileManager->setLEDSetting(eFuseLEDSetting)) != 0)
    {
        printf("Error, export LED setting failed=%d!\n", ret);
        return ret;
    }
    else
        printf("Success, export LED setting succeeded!\n");  

    return ret;
}

int YTCommandHandler::enableDevice(uint8_t devIndex)
{
    int ret = 0;

    if ((ret = m_pFuxiDeviceManager->enableDevice(devIndex)) != 0)
        printf("Error, enable device(index=%u) failed=%d!\n", devIndex, ret);
    else
        printf("Success, enable device(index=%u).\n", devIndex);

    return ret;
}

int YTCommandHandler::disableDevice(uint8_t devIndex)
{
    int ret = 0;

    if ((ret = m_pFuxiDeviceManager->disableDevice(devIndex)) != 0)
        printf("Error, disable device(index=%u) failed=%d!\n", devIndex, ret);
    else
        printf("Success, disable device(index=%u).\n", devIndex);

    return ret;
}

int YTCommandHandler::restartDevice(uint8_t devIndex)
{
    int ret = 0;

    if ((ret = m_pFuxiDeviceManager->restartDevice(devIndex)) != 0)
        printf("Error, restart device(index=%u) failed=%d!\n", devIndex, ret);
    else
        printf("Success, restart device(index=%u).\n", devIndex);

    return ret;
}

int YTCommandHandler::rescanDeviceManager()
{
    int ret = 0;

    if ((ret = m_pFuxiDeviceManager->installDevice(0)) != 0)
        printf("Error, rescan device manager failed=%d!\n", ret);
    else
        printf("Success, rescan device manager.\n");

    return ret;
}

int YTCommandHandler::uninstallDevice(uint8_t devIndex)
{
    int ret = 0;

    if ((ret = m_pFuxiDeviceManager->uninstallDevice(devIndex)) != 0)
        printf("Error, uninstall device(index=%u) failed=%d!\n", devIndex, ret);
    else
        printf("Success, uninstall device(index=%u).\n", devIndex);

    return ret;
}

int YTCommandHandler::handleLEDSimulateCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    if ((ret = checkConfigFile(false)) != 0)
        return ret;

#ifdef _WIN32
    std::basic_string<TCHAR> tcsDestFilePath = PathHelper::GetSystemDir();
    tcsDestFilePath.append(_T("\\drivers\\"));

    if ((ret = m_pFuxiConfigFileManager->copyTo(tcsDestFilePath.c_str(), true)) != 0)
    {
        _tprintf(_T("Error, copy config file to %s failed=%d!\n"), tcsDestFilePath.c_str(), ret);
        return ret;
    }
    else
        printf("Success, driver will simulate LED configuration after restarting device!\n");
#else
    led_setting inputLEDSetting = { 0 };
    if ((ret = m_pFuxiConfigFileManager->getLEDSetting(inputLEDSetting)) != 0)
    {
        printf("Error, get LED configuration from file failed=%d!\n", ret);
        return ret;
    }

    FuxiLEDManager* pFuxiLEDManager = pFuxiDeviceInterface->getLEDManager();

    if ((ret = pFuxiLEDManager->simulateLEDSetting(inputLEDSetting)) != 0)
        printf("Error, simulate LED configuration failed=%d!\n", ret);
    else
        printf("Success, simulate LED configuration!\n");
#endif // _WIN32

    return ret;
}

int YTCommandHandler::handleEFuseCommand(FuxiDeviceInterface* pFuxiDeviceInterface)
{
    int ret = 0;

    FuxiEFuseManager* pFuxiEFuseManager = pFuxiDeviceInterface->getEFuseManager();

    uint8_t eFuseReg[EFUSE_BYTE_COUNT] = { 0 };

    if ((ret = pFuxiEFuseManager->ReadAllRegValue(eFuseReg)) != 0)
    {
        printf("Error, Read eFuse register failed=%d!\n", ret);
        return ret;
    }

    printf("%s\n", "eFuse Register:");
    printf("\n%2s", "");
    for (int col = 0; col < 0x10; col++)
        LogHelper::printfWarning(" %02X", col);

    for (int row = 0; row < 0x10; row++)
    {
        printf("\n");
        LogHelper::printfWarning("%02X", row);
        for (int col = 0; col < 0x10; col++)
            printf(" %02X", eFuseReg[row * 0x10 + col]);
    }
    printf("\n\n");

    return ret;
}

int YTCommandHandler::parseMACAddrfromString(const std::basic_string<TCHAR>& tcsValue, uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], char& separator)
{
    std::size_t tcsLen = _tcslen(tcsValue.c_str());

    if (tcsLen == 17 &&
        stscanf(tcsValue.c_str(), _T("%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"),
            &macAddr[0], &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]) == MAC_ADDR_BYTE_COUNT)
    {
        separator = ':';
        return 0;
    }
        

    if (tcsLen == 17 &&
        stscanf(tcsValue.c_str(), _T("%02hhx-%02hhx-%02hhx-%02hhx-%02hhx-%02hhx"),
            &macAddr[0], &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]) == MAC_ADDR_BYTE_COUNT)
    {
        separator = '-';
        return 0;
    }

    if (tcsLen == 12 &&
        stscanf(tcsValue.c_str(), _T("%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx"),
            &macAddr[0], &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]) == MAC_ADDR_BYTE_COUNT)
    {
        separator = '\0';
        return 0;
    }

    return -1;
}

int YTCommandHandler::parseMACAddrfromString(const std::basic_string<TCHAR>& tcsValue, uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], const std::basic_string<TCHAR>& tcsSeparator)
{
    int ret = 0;

    std::basic_string<TCHAR> tcsFormat(_T(""));
    for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
    {
        tcsFormat.append(_T("%02hhx"));
        if(i != (MAC_ADDR_BYTE_COUNT - 1))
            tcsFormat.append(tcsSeparator);
    }

    if (stscanf(tcsValue.c_str(), tcsFormat.c_str(),
        &macAddr[0], &macAddr[1], &macAddr[2], &macAddr[3], &macAddr[4], &macAddr[5]) != MAC_ADDR_BYTE_COUNT)
        ret = 1;

    return ret;
}

int YTCommandHandler::verifyMACAddr(uint8_t(&macAddr1)[MAC_ADDR_BYTE_COUNT], uint8_t(&macAddr2)[MAC_ADDR_BYTE_COUNT])
{
    for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
    {
        if (macAddr1[i] != macAddr2[i])
            return 1;
    }

    return 0;
}

void YTCommandHandler::printfMACAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], char separator)
{
    for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
    {
        printf("%02X", macAddr[i]);
        if (i != MAC_ADDR_BYTE_COUNT - 1)
            printf("%c", separator);
        else
            printf("\n");
    }
}

void YTCommandHandler::printfMACAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT], const std::basic_string<TCHAR>& tcsSeparator)
{
    for (int i = 0; i < MAC_ADDR_BYTE_COUNT; i++)
    {
        printf("%02hhX", macAddr[i]);
        if (i != (MAC_ADDR_BYTE_COUNT - 1))
            _tprintf(_T("%s"), tcsSeparator.c_str());
        else
            printf("\n");
    }
}

int YTCommandHandler::checkConfigFile(bool bCreate)
{
    int ret = 0;

    if (m_pFuxiConfigFileManager && m_pFuxiConfigFileManager->isOpen())
        return 0;
    else if (m_pFuxiConfigFileManager == nullptr)
        m_pFuxiConfigFileManager = new FuxiConfigFileManager();

    std::basic_string<TCHAR> tcsValue(_T(""));
    m_cmdHelper.getCmdValue(CMD_FILE, tcsValue);

    if ((ret = m_pFuxiConfigFileManager->initialize(tcsValue.c_str(), bCreate)) != 0)
    {
        printf("Error, initialize config file failed=%d\n", ret);
        return -1;
    }

    return ret;
}