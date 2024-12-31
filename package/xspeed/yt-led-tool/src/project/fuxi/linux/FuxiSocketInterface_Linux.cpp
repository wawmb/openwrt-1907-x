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

#include "FuxiSocketInterface_Linux.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "fuxi/FuxiDefine.h"
#include "reg/RegisterHelper.h"


FuxiSocketInterface_Linux::FuxiSocketInterface_Linux()
	: FuxiDriverInterface()
	, m_fdSocket(-1)
	, m_pIfconf(nullptr)
	, m_pIfreq(nullptr)
	, m_pIfaddrs(nullptr)
	, m_strIfname("")
{

}

FuxiSocketInterface_Linux::~FuxiSocketInterface_Linux()
{
	close();
}

int FuxiSocketInterface_Linux::open(uint8_t index)
{
	int ret = 0;

	close();

	if ((ret = initSocket()) != 0)
		return ret;

	/*
	int ifCount = m_pIfconf->ifc_len / (int)sizeof(struct ifreq);

	struct ifreq* pIfreq = (struct ifreq*)m_pIfconf->ifc_ifcu.ifcu_buf;
	for (uint8_t i = 0; i < ifCount; i++)
	{
		printf("Index[%d]: %s\n",i, (pIfreq + i)->ifr_ifrn.ifrn_name);
	}

	if (index >= ifCount)
		return 1;

	m_pIfreq = (struct ifreq*)m_pIfconf->ifc_ifcu.ifcu_buf + index;
	*/

	ret = 1;

	uint8_t uCurrentIndex = 0;
	struct ifaddrs* pCurrentIfaddr = m_pIfaddrs;
	while (pCurrentIfaddr)
	{
		if (uCurrentIndex == index)
		{
			m_strIfname = pCurrentIfaddr->ifa_name;
			ret = 0;
			break;
		}

		pCurrentIfaddr = pCurrentIfaddr->ifa_next;
		uCurrentIndex++;
	}

	return ret;
}

bool FuxiSocketInterface_Linux::isOpen()
{
	return (m_fdSocket >= 0);
}

void FuxiSocketInterface_Linux::close()
{
	if (m_fdSocket >= 0)
	{
		::close(m_fdSocket);
		m_fdSocket = -1;
	}

	if (m_pIfaddrs)
	{
		freeifaddrs(m_pIfaddrs);
		m_pIfaddrs = nullptr;
	}

	m_strIfname = "";

	/*
	m_pIfreq = nullptr;

	if(m_pIfconf)
	{
		if(m_pIfconf->ifc_ifcu.ifcu_buf)
			delete[] m_pIfconf->ifc_ifcu.ifcu_buf;

		delete m_pIfconf;
		m_pIfconf = nullptr;
	}
	*/
}

int FuxiSocketInterface_Linux::getHandle(void* handle)
{
	if (m_fdSocket >= 0)
	{
		*((int*)handle) = m_fdSocket;
		return 0;
	}

	return 1;
}

int FuxiSocketInterface_Linux::initSocket()
{
	int ret = 0;

	if (m_pIfaddrs && (m_fdSocket >= 0))
		return 0;

	if ((ret = getifaddrs(&m_pIfaddrs)) != 0) 
	{
		perror("getifaddrs failed: ");
		return ret;
	}

	if ((m_fdSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket failed: ");
		::close(m_fdSocket);
		return -1;
	}

	/*
	m_pIfconf = new ifconf();

	int nSize = sizeof(struct ifreq) * 1;

	do
	{
		nSize *= 2;

		if (m_pIfconf->ifc_ifcu.ifcu_buf)
			delete[] m_pIfconf->ifc_ifcu.ifcu_buf;

		m_pIfconf->ifc_len = nSize;
		m_pIfconf->ifc_ifcu.ifcu_buf = new char[m_pIfconf->ifc_len] { 0 };

		if (ioctl(m_fdSocket, SIOCGIFCONF, m_pIfconf) < 0)
		{
			perror("ioctl SIOCGIFCONF failed:");
			::close(m_fdSocket);
			return -2;
		}

	} while (m_pIfconf->ifc_len >= nSize);

	if (m_pIfconf->ifc_ifcu.ifcu_buf == nullptr)
		return -3;
	*/

	return ret;
}

int FuxiSocketInterface_Linux::readGMACReg(uint32_t addr, uint32_t& value)
{
	return readReg(FXGMAC_GET_GMAC_REG, addr, value);
}

int FuxiSocketInterface_Linux::writeGMACReg(uint32_t addr, uint32_t value)
{
	return writeReg(FXGMAC_SET_GMAC_REG, addr, value);
}

int FuxiSocketInterface_Linux::readPHYReg(uint32_t addr, uint32_t& value)
{
	return readReg(FXGMAC_GET_PHY_REG, addr, value);
}

int FuxiSocketInterface_Linux::writePHYReg(uint32_t addr, uint32_t value)
{
	return writeReg(FXGMAC_SET_PHY_REG, addr, value);
}

int FuxiSocketInterface_Linux::readEfuseReg(uint32_t addr, uint32_t& value)
{
	return readReg(FXGMAC_EFUSE_READ_REGIONABC, addr, value);
}

int FuxiSocketInterface_Linux::getAllPackageCounter(MP_COUNT_STRUC& packageCounter)
{
	int ret = 0;

	return ret;
}

int FuxiSocketInterface_Linux::readEfusePatch(uint8_t index, uint32_t& addr, uint32_t& value)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_EFUSE_READ_PATCH_PER_INDEX;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;
	
	pCommandData->val0 = index;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_EFUSE_READ_PATCH_PER_INDEX ioctl failed: ");
		ret = -1;
	}
	else
	{
		if (pCommandData)
		{
			addr = pCommandData->val1;
			value = pCommandData->val2;
		}
		else
		{
			printf("FXGMAC_EFUSE_READ_PATCH_PER_INDEX failed, pCommandData is nullptr\n");
			ret = -2;
		}
			
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::writeEfusePatch(uint8_t index, uint32_t addr, uint32_t value)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	pCommandData->val0 = index;
	pCommandData->val1 = addr;
	pCommandData->val2 = value;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_EFUSE_WRITE_PATCH_PER_INDEX ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::readMACAddressfromEFuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(macAddr);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	uint8_t* pCommandData = (uint8_t*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_GET_MAC_DATA;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_GET_MAC_DATA ioctl failed: ");
		ret = -1;
	}
	else
	{
		if (pCommandData)
			memcpy(macAddr, pCommandData, sizeof(macAddr));
		else
			ret = -2;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::writeMACAddresstoEfuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT])
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(macAddr);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	uint8_t* pCommandData = (uint8_t*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_SET_MAC_DATA;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	memcpy(pCommandData, macAddr, sizeof(macAddr));

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_SET_MAC_DATA ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::readSubSystemIDfromEFuse(uint32_t& subDeviceID, uint32_t& subVendorID, uint32_t& revisionID)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_GET_SUBSYS_ID;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_GET_SUBSYS_ID ioctl failed: ");
		ret = -1;
	}
	else
	{
		if (pCommandData)
		{
			uint32_t uSubSystemID = pCommandData->val0;

			subDeviceID = GET_REG_BITS(uSubSystemID, 0, 16);
			subVendorID = GET_REG_BITS(uSubSystemID, 16, 16);
		}
		else
			ret = -2;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_SET_SUBSYS_ID;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	uint32_t uSubSystemID = subDeviceID;
	uSubSystemID = SET_REG_BITS(uSubSystemID, 16, 16, subVendorID);
	pCommandData->val0 = uSubSystemID;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_SET_SUBSYS_ID ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::writeLEDStatustoEFuse(uint8_t status)
{
	return setValue(FXGMAC_EFUSE_WRITE_LED, status);
}

int FuxiSocketInterface_Linux::enableWOLtoEfuse()
{
	return sendCommand(FXGMAC_EFUSE_WRITE_OOB);
}

int FuxiSocketInterface_Linux::initLoopback()
{
	return sendCommand(FUXI_DFS_IOCTL_DIAG_BEGIN);
}

int FuxiSocketInterface_Linux::cleanLoopback()
{
	return sendCommand(FUXI_DFS_IOCTL_DIAG_END);
}

int FuxiSocketInterface_Linux::updateLEDConfigtoEfuse(const led_setting& ledSetting)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(struct led_setting);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	struct led_setting* pCommandData = (struct led_setting*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_EFUSE_UPDATE_LED_CFG;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	memcpy(pCommandData, &ledSetting, sizeof(ledSetting));

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_EFUSE_UPDATE_LED_CFG ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::simulateLEDSetting(const led_setting& ledSetting)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(struct led_setting);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	struct led_setting* pCommandData = (struct led_setting*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_EFUSE_LED_TEST;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	memcpy(pCommandData, &ledSetting, sizeof(ledSetting));

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("FXGMAC_EFUSE_LED_TEST ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::getDeviceLocation(std::wstring& wcsDevLoaction)
{
	int ret = 0;

	wcsDevLoaction = L"";

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(PCI_DEV_INFO);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	PCI_DEV_INFO* pCommandData = (PCI_DEV_INFO*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = FXGMAC_GET_PCIE_LOCATION;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		//perror("FXGMAC_GET_PCIE_LOCATION ioctl failed: ");
		ret = -1;
	}
	else
	{
		if (pCommandData)
		{
			wchar_t szDevLoaction[64] = { 0 };
			swprintf(szDevLoaction, sizeof(szDevLoaction)/sizeof(wchar_t), L"PCI bus %u, device %u, function %u", pCommandData->bus_no, pCommandData->dev_no, pCommandData->func_no);
			wcsDevLoaction = szDevLoaction;
		}
		else
			ret = -2;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::readReg(uint32_t cmdType, uint32_t addr, uint32_t& value)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = cmdType;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	pCommandData->val0 = addr;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("readReg ioctl failed: ");
		ret = -1;
	}
	else
	{
		if (pCommandData)
			value = pCommandData->val1;
		else
			ret = -2;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::writeReg(uint32_t cmdType, uint32_t addr, uint32_t value)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = cmdType;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	pCommandData->val0 = addr;
	pCommandData->val1 = value;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("writeReg ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::setValue(uint32_t cmdType, uint32_t value)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data) + sizeof(CMD_DATA);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;
	CMD_DATA* pCommandData = (CMD_DATA*)(ifreqCommand.ifr_data + sizeof(ext_ioctl_data));

	pExtIOCtlData->cmd_type = cmdType;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = (void*)pCommandData;

	pCommandData->val0 = value;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("setValue ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}

int FuxiSocketInterface_Linux::sendCommand(uint32_t cmdType)
{
	int ret = 0;

	if (!isOpen())
		return -1;

	struct ifreq ifreqCommand = { 0 };
	strcpy(ifreqCommand.ifr_ifrn.ifrn_name, m_strIfname.c_str());

	uint32_t uTotalSize = sizeof(ext_ioctl_data);
	ifreqCommand.ifr_data = new char[uTotalSize] { 0 };

	struct ext_ioctl_data* pExtIOCtlData = (struct ext_ioctl_data*)ifreqCommand.ifr_data;

	pExtIOCtlData->cmd_type = cmdType;
	pExtIOCtlData->cmd_buf.size_in = uTotalSize;
	pExtIOCtlData->cmd_buf.buf = nullptr;

	if (ioctl(m_fdSocket, FXGMAC_DEV_CMD, &ifreqCommand) < 0)
	{
		perror("sendCommand ioctl failed: ");
		ret = -1;
	}

	if (ifreqCommand.ifr_data)
		delete[] ifreqCommand.ifr_data;

	return ret;
}
