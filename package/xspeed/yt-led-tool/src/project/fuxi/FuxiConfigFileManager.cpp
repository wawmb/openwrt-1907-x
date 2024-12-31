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

#include "FuxiConfigFileManager.h"

#ifdef _WIN32
#include "file/windows/IniHelper_Win.h"
#else
#include "file/linux/IniHelper_Linux.h"
#endif // _WIN32


#include "file/PathHelper.h"


#define CONFIG_DEVICE_SECTION_NAME  		_T("Device")

#define CONFIG_EFUSE_KEY_NAME_BUS  			_T("Bus")
#define CONFIG_EFUSE_KEY_NAME_DEV  			_T("Device")
#define CONFIG_EFUSE_KEY_NAME_FUNC  		_T("Function")

#define CONFIG_EFUSE_SECTION_NAME  			_T("eFuse")

#define CONFIG_EFUSE_KEY_NAME_SSID  		_T("SubsystemID")

#define CONFIG_LED_SECTION_NAME  			_T("LED")

#define CONFIG_LED_KEY_NAME_S0_1  			_T("S0-A00B")
#define CONFIG_LED_KEY_NAME_S0_2  			_T("S0-A00C")	
#define CONFIG_LED_KEY_NAME_S0_3  			_T("S0-A00D")
#define CONFIG_LED_KEY_NAME_S0_4  			_T("S0-A00E")
#define CONFIG_LED_KEY_NAME_S0_5  			_T("S0-A00F")

#define CONFIG_LED_KEY_NAME_S3_1  			_T("S3-A00B")
#define CONFIG_LED_KEY_NAME_S3_2  			_T("S3-A00C")	
#define CONFIG_LED_KEY_NAME_S3_3  			_T("S3-A00D")
#define CONFIG_LED_KEY_NAME_S3_4  			_T("S3-A00E")
#define CONFIG_LED_KEY_NAME_S3_5  			_T("S3-A00F")

#define CONFIG_LED_KEY_NAME_S5_1  			_T("S5-A00B")
#define CONFIG_LED_KEY_NAME_S5_2  			_T("S5-A00C")	
#define CONFIG_LED_KEY_NAME_S5_3  			_T("S5-A00D")
#define CONFIG_LED_KEY_NAME_S5_4  			_T("S5-A00E")
#define CONFIG_LED_KEY_NAME_S5_5  			_T("S5-A00F")


FuxiConfigFileManager::FuxiConfigFileManager()
	: m_pIniHelper(nullptr)
{

}

FuxiConfigFileManager::~FuxiConfigFileManager()
{
	release();
}

int FuxiConfigFileManager::initialize(const uint8_t& bus, const uint8_t& dev, const uint8_t& func)
{
	int ret = 0;

	release();

#ifdef _WIN32
	m_pIniHelper = new IniHelper_Win();
#else
	m_pIniHelper = new IniHelper_Linux();
#endif // _WIN32

	bool bFind = false;

	for (uint8_t i = 0; i < FUXI_MAX_DEV_COUNT; i++)
	{
		TCHAR szFileName[32] = { 0 };
		_stprintf_s(szFileName, FORMAT_CONFIG_FILE_NAME, i);

		std::basic_string<TCHAR> tcsFilePath = PathHelper::GetExeDir();
		tcsFilePath.append(szFileName);

		if (!PathHelper::IsPathExists(tcsFilePath.c_str()))
			continue;

		if (m_pIniHelper->open(tcsFilePath.c_str(), false) != 0)
			continue;

		if (!isBDFMatched(bus, dev, func))
		{
			m_pIniHelper->close();
			continue;
		}

		bFind = true;
		break;
	}

	if (!bFind)
	{
		std::basic_string<TCHAR> tcsFilePath = PathHelper::GetExeDir();
		tcsFilePath.append(CONFIG_FILE_NAME);

		if ((ret = m_pIniHelper->open(tcsFilePath.c_str(), false)) != 0)
		{
			release();
			return ret;
		}
	}

	return ret;
}

int FuxiConfigFileManager::initialize(const uint8_t& index)
{
	int ret = 0;

	release();

#ifdef _WIN32
	m_pIniHelper = new IniHelper_Win();
#else
	m_pIniHelper = new IniHelper_Linux();
#endif // _WIN32

	TCHAR szFileName[32] = { 0 };
	_stprintf_s(szFileName, FORMAT_CONFIG_FILE_NAME, index);

	std::basic_string<TCHAR> tcsFilePath = PathHelper::GetExeDir();
	tcsFilePath.append(szFileName);

	if (!PathHelper::IsPathExists(tcsFilePath.c_str()))
		return -1;

	if ((ret = m_pIniHelper->open(tcsFilePath.c_str(), false)) != 0)
		return ret;

	return ret;
}

void FuxiConfigFileManager::release()
{
	if (m_pIniHelper)
	{
		m_pIniHelper->close();
		delete m_pIniHelper;
		m_pIniHelper = nullptr;
	}
}

bool FuxiConfigFileManager::isOpen()
{
	if (m_pIniHelper && m_pIniHelper->isOpen())
		return true;

	return false;
}

int FuxiConfigFileManager::create()
{
	int ret = 0;

	if (m_pIniHelper && m_pIniHelper->isOpen())
		return 0;

	release();



	return ret;
}

int FuxiConfigFileManager::getBDF(uint8_t& bus, uint8_t& dev, uint8_t& func)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	std::basic_string<TCHAR> tcsValue(_T(""));

	if((ret = m_pIniHelper->getValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_BUS, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	bus = (uint8_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_DEV, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	dev = (uint8_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_FUNC, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	func = (uint8_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	return ret;
}

int FuxiConfigFileManager::setBDF(const uint8_t& bus, const uint8_t& dev, const uint8_t& func)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	TCHAR szValue[256] = { 0 };

	_stprintf_s(szValue, _T("0x%02X"), bus);
	if ((ret = m_pIniHelper->setValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_BUS, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%02X"), dev);
	if ((ret = m_pIniHelper->setValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_DEV, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%02X"), func);
	if ((ret = m_pIniHelper->setValue(CONFIG_DEVICE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_FUNC, szValue)) != 0)
		return ret;

	return ret;
}

bool FuxiConfigFileManager::isBDFMatched(const uint8_t& bus, const uint8_t& dev, const uint8_t& func)
{
	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return false;

	uint8_t uConfigBus = 0;
	uint8_t uConfigDev = 0;
	uint8_t uConfigFunc = 0;

	if (getBDF(uConfigBus, uConfigDev, uConfigFunc) != 0)
		return false;

	if (bus == uConfigBus &&
		dev == uConfigDev &&
		func == uConfigFunc)
		return true;

	return false;
}

int FuxiConfigFileManager::getSubsystemID(uint32_t& subsystemID)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	std::basic_string<TCHAR> tcsValue(_T(""));

	if ((ret = m_pIniHelper->getValue(CONFIG_EFUSE_SECTION_NAME, CONFIG_EFUSE_KEY_NAME_SSID, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	subsystemID = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	return ret;
}

int FuxiConfigFileManager::getLEDSetting(led_setting& ledSetting)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	std::basic_string<TCHAR> tcsValue;

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_1, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s0_led_setting[0] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_2, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s0_led_setting[1] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_3, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s0_led_setting[2] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_4, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s0_led_setting[3] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_5, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s0_led_setting[4] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	//
	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_1, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s3_led_setting[0] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_2, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s3_led_setting[1] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_3, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s3_led_setting[2] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_4, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s3_led_setting[3] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_5, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s3_led_setting[4] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	//
	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_1, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s5_led_setting[0] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_2, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s5_led_setting[1] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_3, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s5_led_setting[2] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_4, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s5_led_setting[3] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);

	if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_5, tcsValue)) != 0)
		return ret;

	if (tcsValue.empty())
		return 1;

	ledSetting.s5_led_setting[4] = (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16);


	//// linux driver can only recognize S0 and S3, so S5 use default Windows-S5
	//ledSetting.s5_led_setting[0] = 0xE000;
	//ledSetting.s5_led_setting[1] = 0x0000;
	//ledSetting.s5_led_setting[2] = 0x0000;
	//ledSetting.s5_led_setting[3] = 0x0000;
	//ledSetting.s5_led_setting[4] = 0x0006;

	////
	//if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, "Disable-A00B", tcsValue)) != 0)
	//	return ret;
	//ledSetting.disable_led_setting[0] = (uint32_t)strtoul(tcsValue.c_str(), NULL, 16);

	//if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, "Disable-A00C", tcsValue)) != 0)
	//	return ret;
	//ledSetting.disable_led_setting[1] = (uint32_t)strtoul(tcsValue.c_str(), NULL, 16);

	//if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, "Disable-A00D", tcsValue)) != 0)
	//	return ret;
	//ledSetting.disable_led_setting[2] = (uint32_t)strtoul(tcsValue.c_str(), NULL, 16);

	//if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, "Disable-A00E", tcsValue)) != 0)
	//	return ret;
	//ledSetting.disable_led_setting[3] = (uint32_t)strtoul(tcsValue.c_str(), NULL, 16);

	//if ((ret = m_pIniHelper->getValue(CONFIG_LED_SECTION_NAME, "Disable-A00F", tcsValue)) != 0)
	//	return ret;
	//ledSetting.disable_led_setting[4] = (uint32_t)strtoul(tcsValue.c_str(), NULL, 16);

	// customer change request, remove diable state, copy s0 config
	for (int i = 0; i < 5; i++)
		ledSetting.disable_led_setting[i] = ledSetting.s0_led_setting[i];

	return ret;
}

int FuxiConfigFileManager::setLEDSetting(const led_setting& ledSetting)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	TCHAR szValue[32] = { 0 };

	//
	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s0_led_setting[0]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_1, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s0_led_setting[1]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_2, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s0_led_setting[2]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_3, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s0_led_setting[3]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_4, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s0_led_setting[4]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S0_5, szValue)) != 0)
		return ret;

	//
	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s3_led_setting[0]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_1, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s3_led_setting[1]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_2, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s3_led_setting[2]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_3, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s3_led_setting[3]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_4, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s3_led_setting[4]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S3_5, szValue)) != 0)
		return ret;

	//
	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s5_led_setting[0]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_1, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s5_led_setting[1]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_2, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s5_led_setting[2]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_3, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s5_led_setting[3]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_4, szValue)) != 0)
		return ret;

	_stprintf_s(szValue, _T("0x%04X"), ledSetting.s5_led_setting[4]);

	if ((ret = m_pIniHelper->setValue(CONFIG_LED_SECTION_NAME, CONFIG_LED_KEY_NAME_S5_5, szValue)) != 0)
		return ret;

	//
	if((ret = m_pIniHelper->save()) != 0)
		return ret;

	return ret;
}

int FuxiConfigFileManager::initialize(const TCHAR* szFilePath, bool bCreate)
{
	int ret = 0;

	release();

#ifdef _WIN32
	m_pIniHelper = new IniHelper_Win();
#else
	m_pIniHelper = new IniHelper_Linux();
#endif // _WIN32

	std::basic_string<TCHAR> tcsFilePath(_T(""));
	if (szFilePath)
		tcsFilePath = szFilePath;

	if (tcsFilePath.empty())
	{
		tcsFilePath = PathHelper::GetExeDir();
		tcsFilePath.append(CONFIG_FILE_NAME);
		//_tprintf(_T("%s\n"), tcsFilePath.c_str());
	}

	if ((ret = m_pIniHelper->open(tcsFilePath.c_str(), bCreate)))
		return ret;

	return ret;
}

int FuxiConfigFileManager::copyTo(const TCHAR* szDestPath, bool destIsDir)
{
	int ret = 0;

	if (m_pIniHelper == nullptr || !m_pIniHelper->isOpen())
		return -1;

	std::basic_string<TCHAR> tcsSrcFilePath = m_pIniHelper->getFilePath();
	std::basic_string<TCHAR> tcsDestFilePath = szDestPath;

	if (destIsDir)
		tcsDestFilePath.append(PathHelper::ExtractFileName(tcsSrcFilePath));
	
	//printf("copyFile %ls %ls\n", tcsSrcFilePath.c_str(), tcsDestFilePath.c_str());
	if ((ret = PathHelper::copyFile(tcsSrcFilePath.c_str(), tcsDestFilePath.c_str())) != 0)
		return ret;

	return ret;
}