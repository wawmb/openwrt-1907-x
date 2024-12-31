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

#include "FuxiMemInterface_Linux.h"

#include <inttypes.h>

extern "C" {
#include "io/io_helper.h"
}

#include "fuxi/FuxiDefine.h"
#include "reg/RegisterHelper.h"


FuxiMemInterface_Linux::FuxiMemInterface_Linux()
	: FuxiDriverInterface()
{
}

FuxiMemInterface_Linux::~FuxiMemInterface_Linux()
{
    close();
}

int FuxiMemInterface_Linux::open(uint64_t memBaseAddress)
{
    close();

    m_FuxiPCIMemBase = memBaseAddress;

    return 0;
}

bool FuxiMemInterface_Linux::isOpen()
{
    return (m_FuxiPCIMemBase > 0);
}

void FuxiMemInterface_Linux::close()
{
    m_FuxiPCIMemBase = 0;
}

int FuxiMemInterface_Linux::getHandle(void* handle)
{
    if (m_FuxiPCIMemBase >= 0)
    {
        *((uint64_t*)handle) = m_FuxiPCIMemBase;
        return 0;
    }

    return 1;
}

int FuxiMemInterface_Linux::readMemReg(uint32_t addr, uint32_t& value)
{
    return MemRead32(m_FuxiPCIMemBase + addr, &value);
}

int FuxiMemInterface_Linux::writeMemReg(uint32_t addr, uint32_t value)
{
    return MemWrite32(m_FuxiPCIMemBase + addr, value);
}

int FuxiMemInterface_Linux::readGMACReg(uint32_t addr, uint32_t& value)
{
    return MemRead32(m_FuxiPCIMemBase + 0x2000 + addr, &value);
}

int FuxiMemInterface_Linux::writeGMACReg(uint32_t addr, uint32_t value)
{
    return MemWrite32(m_FuxiPCIMemBase + 0x2000 + addr, value);
}

int FuxiMemInterface_Linux::readPHYReg(uint32_t addr, uint32_t& value)
{
    int ret = 0;

    uint64_t uMDIOCtrlAddr = m_FuxiPCIMemBase + 0x2000 + MAC_MDIO_ADDRESS;
    uint64_t uMDIODataAddr = m_FuxiPCIMemBase + 0x2000 + MAC_MDIO_DATA;

    uint32_t uMDIOCtrlValue = addr * 0x10000 + 0x800020D;

    if ((ret = MemWrite32(uMDIOCtrlAddr, uMDIOCtrlValue)) != 0)
        return ret;

    uint32_t uRetryCount = 15;
    do
    {
        uint32_t uRegValue = 0;
        if ((ret = MemRead32(uMDIOCtrlAddr, &uRegValue)) != 0)
            return ret;

        // bit0 = 1 means busy
        if ((uRegValue & 0x01) == 0)
            break;

        uRetryCount--;

    } while (uRetryCount);

    // busy
    if (uRetryCount == 0)
    {
        printf("Read PHY reg busy, YT6801 PCI mem base=0x%" PRIX64"\n", m_FuxiPCIMemBase);
        return 1;
    }      

    uint32_t uRegValue = 0;
    if ((ret = MemRead32(uMDIODataAddr, &uRegValue)) != 0)
        return ret;

    value = (uint32_t)uRegValue;

    return ret;
}

int FuxiMemInterface_Linux::writePHYReg(uint32_t addr, uint32_t value)
{
    int ret = 0;

    uint64_t uMDIOCtrlAddr = m_FuxiPCIMemBase + 0x2000 + MAC_MDIO_ADDRESS;
    uint64_t uMDIODataAddr = m_FuxiPCIMemBase + 0x2000 + MAC_MDIO_DATA;

    uint32_t uMDIOCtrlValue = addr * 0x10000 + 0x8000205;

    if ((ret = MemWrite32(uMDIODataAddr, (uint32_t)value)) != 0)
        return ret;

    if ((ret = MemWrite32(uMDIOCtrlAddr, uMDIOCtrlValue)) != 0)
        return ret;

    uint32_t uRetryCount = 15;
    do
    {
        uint32_t uRegValue = 0;
        if ((ret = MemRead32(uMDIOCtrlAddr, &uRegValue)) != 0)
            return ret;

        // bit0 = 1 means busy
        if ((uRegValue & 0x01) == 0)
            break;

        uRetryCount--;

    } while (uRetryCount);

    // busy
    if (uRetryCount == 0)
    {
        printf("Write PHY reg busy, YT6801 PCI mem base=0x%" PRIX64"\n", m_FuxiPCIMemBase);
        return 1;
    }

    return ret;
}

