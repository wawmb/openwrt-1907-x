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

#include "PCIDeviceHelper_Linux.h"

#include <dirent.h>
#include <cstring>


PCIDeviceHelper_Linux::PCIDeviceHelper_Linux()
	: m_pPCIAccess(nullptr)
{
}

PCIDeviceHelper_Linux::~PCIDeviceHelper_Linux()
{
}

int PCIDeviceHelper_Linux::refresh()
{
    release();

    m_pPCIAccess = pci_alloc();
    if (m_pPCIAccess == nullptr)
    {
        perror("pci_alloc failed:");
        return -1;
    }

    pci_init(m_pPCIAccess);

    pci_scan_bus(m_pPCIAccess);

	pci_dev* pPCIDev = m_pPCIAccess->devices;

    while (pPCIDev != nullptr)
    {
        pci_fill_info(pPCIDev, 0x001FFFFF);
        m_vecPCIDev.push_back(pPCIDev);
        pPCIDev = pPCIDev->next;
    }

    if (m_vecPCIDev.empty())
    {
        printf("Warning, can't find any PCI device!\n");
        return 1;
    }

    return 0;
}

void PCIDeviceHelper_Linux::release()
{
    m_vecPCIDev.clear();

    if (m_pPCIAccess)
    {
        pci_cleanup(m_pPCIAccess);
        m_pPCIAccess = nullptr;
    }
}

std::size_t PCIDeviceHelper_Linux::getDeviceCount()
{
    return m_vecPCIDev.size();
}

int PCIDeviceHelper_Linux::getMemBaseAddr(std::size_t index, std::size_t bar, uint64_t& memBaseAddr)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    memBaseAddr = (uint64_t)m_vecPCIDev[index]->base_addr[bar];

    // avoid pciutils bug
    if (memBaseAddr == 0)
    {
        printf("Warning, get device[%ld] bar[%ld] base address from PCIutils falied!\n", index, bar);
        int offset = 0x10 + (int)(0x04 * bar);
        memBaseAddr = (uint64_t)pci_read_long(m_vecPCIDev[index], offset);
    }

    return 0;
}

int PCIDeviceHelper_Linux::read16(std::size_t index, int offset, uint16_t& value)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    value = pci_read_word(m_vecPCIDev[index], offset);

    return 0;
}

int PCIDeviceHelper_Linux::read32(std::size_t index, int offset, uint32_t& value)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    value = pci_read_long(m_vecPCIDev[index], offset);

    return 0;
}

int PCIDeviceHelper_Linux::getDeviceID(std::size_t index, uint16_t& deviceID)
{
    return read16(index, 2, deviceID);
}

int PCIDeviceHelper_Linux::getVendorID(std::size_t index, uint16_t& vendorID)
{
    return read16(index, 0, vendorID);
}

int PCIDeviceHelper_Linux::getDeviceLocationInfo(std::size_t index, uint8_t& bus, uint8_t& dev, uint8_t& func)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    bus = m_vecPCIDev[index]->bus;
    dev = m_vecPCIDev[index]->dev;
    func = m_vecPCIDev[index]->func;

    return 0;
}

int PCIDeviceHelper_Linux::getDeviceFriendlyName(std::size_t index, std::string& tcsFriendlyName)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    tcsFriendlyName = "";
    //if(m_vecPCIDev[index]->module_alias)
    //    tcsFriendlyName = m_vecPCIDev[index]->module_alias;

    char szDirPath[256] = {0};
    sprintf(szDirPath, "/sys/bus/pci/devices/%04x:%02x:%02x.%x/net",
        m_vecPCIDev[index]->domain, m_vecPCIDev[index]->bus, m_vecPCIDev[index]->dev, m_vecPCIDev[index]->func);

    DIR* pDir = opendir(szDirPath);
    if (pDir)
    {
        struct dirent* pDirent = nullptr;
        do
        {
            pDirent = readdir(pDir);

            if (pDirent->d_type != DT_DIR)
                continue;

            if(strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
                continue;
            
            tcsFriendlyName = pDirent->d_name;
            break;
            
        } while (pDirent);

        closedir(pDir);
    }

    return 0;
}

int PCIDeviceHelper_Linux::getDeviceDriverVersion(std::size_t index, std::string& tcsDriverVersion)
{
    if (index >= m_vecPCIDev.size())
        return 1;

    //tcsDriverVersion = pci_get_string_property(m_vecPCIDev[index], PCI_FILL_DRIVER);

    return 0;
}