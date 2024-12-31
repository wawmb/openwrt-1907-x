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

#include <stdint.h>

#include "fuxi/FuxiDeviceHelper.h"
#include "fuxi/FuxiDriverInterface.h"
#include "fuxi/FuxiEFuseManager.h"
#include "fuxi/FuxiPatchManager.h"
#include "fuxi/FuxiLEDManager.h"
#include "vulcan/VulcanManager.h"
#include "fuxi/FuxiSystemManager.h"


class FuxiDeviceInterface
{
public:
	FuxiDeviceInterface(FuxiDeviceHelper* fuxiDeviceHelper);
	virtual ~FuxiDeviceInterface();

	virtual int initialize(uint8_t index);
	virtual void release();

	virtual int releaseDriverInterface();
	virtual int loadDriverInterface();

	virtual FuxiDeviceHelper* getDeviceHelper();
	virtual FuxiDriverInterface* getDriverInterface();
	virtual FuxiEFuseManager* getEFuseManager();
	virtual FuxiPatchManager* getPatchManager();
	virtual FuxiLEDManager* getLEDManager();
	virtual VulcanManager* getVulcanManager();
	virtual FuxiSystemManager* getSystemManager();

protected:
	uint8_t m_index;
	FuxiDeviceHelper* m_pFuxiDeviceHelper;
	FuxiDriverInterface* m_pFuxiDriverInterface;
	FuxiEFuseManager* m_pFuxiEFuseManager;
	FuxiPatchManager* m_pFuxiPatchManager;
	FuxiLEDManager* m_pFuxiLEDManager;
	VulcanManager* m_pVulcanManager;
	FuxiSystemManager* m_fuxiSystemManager;
};