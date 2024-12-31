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


#include "file/IniHelper.h"
#include "utility/CrossPlatformUtility.h"

#include "fuxi/FuxiDefine.h"
#include "fuxi/FuxiDriverInterface.h"


class FuxiConfigFileManager
{
public:
	FuxiConfigFileManager();
	virtual ~FuxiConfigFileManager();

	virtual int initialize(const uint8_t& bus, const uint8_t& dev, const uint8_t& func);
	virtual int initialize(const uint8_t& index);
	virtual int initialize(const TCHAR* szFilePath, bool bCreate = false);

	virtual void release();

	virtual bool isOpen();
	virtual int create();

	virtual int getBDF(uint8_t& bus, uint8_t& dev, uint8_t& func);
	virtual int setBDF(const uint8_t& bus, const uint8_t& dev, const uint8_t& func);
	virtual bool isBDFMatched(const uint8_t& bus, const uint8_t& dev, const uint8_t& func);

	virtual int getSubsystemID(uint32_t &subsystemID);

	virtual int getLEDSetting(led_setting& ledSetting);
	virtual int setLEDSetting(const led_setting& ledSetting);

	virtual int copyTo(const TCHAR* szDestPath, bool destIsDir = false);
	

protected:
	IniHelper<TCHAR>* m_pIniHelper;
};

