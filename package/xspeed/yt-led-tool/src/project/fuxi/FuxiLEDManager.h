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

#include "fuxi/FuxiEFuseManager.h"


class FuxiLEDManager
{
public:
	FuxiLEDManager();
	virtual ~FuxiLEDManager();

	virtual int initialize(FuxiEFuseManager* pFuxiEFuseManager);
	virtual void release();

	virtual int applyLEDSolution(const uint8_t& solution);
	virtual int readLEDSolution(uint8_t& solution);

	virtual int setLEDConfigMode();

	virtual int simulateLEDSetting(const led_setting& ledSetting);
	virtual int applyLEDSetting(const led_setting& ledSetting);

	virtual int verifyLEDSetting(const led_setting& ledSetting);

	virtual uint8_t getConfigMaxWriteCount();
	virtual int getConfigWrittenCount(uint8_t& count);

	virtual int loadLEDSettingfromeFuse(led_setting &ledSetting);

protected:
	FuxiEFuseManager* m_pFuxiEFuseManager;
};

