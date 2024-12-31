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

#include "FuxiPatchManager.h"


FuxiPatchManager::FuxiPatchManager()
	: m_pFuxiEFuseManager(nullptr)
{
}

FuxiPatchManager::~FuxiPatchManager()
{
}

int FuxiPatchManager::initialize(FuxiEFuseManager* pFuxiEFuseManager)
{
	int ret = 0;

	if (pFuxiEFuseManager == nullptr)
		return -1;

	m_pFuxiEFuseManager = pFuxiEFuseManager;

	return ret;
}

void FuxiPatchManager::release()
{
	m_pFuxiEFuseManager = nullptr;
}

int FuxiPatchManager::applyDevLostPatch(bool bShowLogMsg)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	bool bNeedDevLostPatch = true;
	if (m_pFuxiEFuseManager->IsDeviceLostPatchExists())
	{
		// if LED init pacth exists, must add 0x1188=0x111 pacth
		if (!m_pFuxiEFuseManager->IsASPML1PatchExists())
			bNeedDevLostPatch = false;
	}
	
	if (!bNeedDevLostPatch)
	{
		if (bShowLogMsg) printf("Device stability configuration already exists, do nothing!\n");
		return ret;
	}

	if ((ret = m_pFuxiEFuseManager->ApplyDeviceLostPatch()) != 0)
	{
		if (bShowLogMsg) printf("Apply device stability configuration failed=%d!\n", ret);
		return ret;
	}

	if ((ret = m_pFuxiEFuseManager->RefreshPatchItems()) != 0)
	{
		if (bShowLogMsg) printf("Refresh eFuse failed=%d!\n", ret);
		return ret;
	}

	if (bShowLogMsg) printf("Apply device stability configuration succeeded!\n");

	return ret;
}

int FuxiPatchManager::applyASPMPatch(bool bShowLogMsg)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if (m_pFuxiEFuseManager->IsASPMPatchExists())
	{
		if(bShowLogMsg) printf("Power optimization configuration already exists, do nothing!\n");
		return ret;
	}

	if((ret = m_pFuxiEFuseManager->ApplyASPMPatch()) != 0)
	{
		if (bShowLogMsg) printf("Apply power optimization configuration failed=%d!\n", ret);
		return ret;
	}

	if ((ret = m_pFuxiEFuseManager->RefreshPatchItems()) != 0)
	{
		if (bShowLogMsg) printf("Refresh eFuse failed=%d!\n", ret);
		return ret;
	}

	if (bShowLogMsg) printf("Apply power optimization configuration succeeded!\n");

	return ret;
}

int FuxiPatchManager::applyLEDPatch(bool bShowLogMsg)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if (m_pFuxiEFuseManager->IsLEDPatchExists())
	{
		if (bShowLogMsg) printf("LED initial configuration already exists, do nothing!\n");
		return ret;
	}

	if ((ret = m_pFuxiEFuseManager->ApplyLEDPatch()) != 0)
	{
		if (bShowLogMsg) printf("Apply LED initial configuration failed=%d!\n", ret);
		return ret;
	}

	if ((ret = m_pFuxiEFuseManager->RefreshPatchItems()) != 0)
	{
		if (bShowLogMsg) printf("Refresh eFuse failed=%d!\n", ret);
		return ret;
	}

	if (bShowLogMsg) printf("Apply LED initial configuration succeeded!\n");

	return ret;
}

int FuxiPatchManager::applyLEDInitPatch(bool bShowLogMsg)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if ((ret = applyLEDPatch(bShowLogMsg)) != 0)
		return ret;

	if ((ret = applyDevLostPatch(false)) != 0)
	{
		if (bShowLogMsg) printf("Apply LED initial extend configuration failed=%d!\n", ret);
		return ret;
	}

	return ret;
}

int FuxiPatchManager::applyAllPatch(bool bShowLogMsg)
{
	int final_ret = 0;
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if ((ret = applyDevLostPatch(bShowLogMsg)) != 0)
		final_ret = ret;

	if ((ret = applyASPMPatch(bShowLogMsg)) != 0)
		final_ret = ret;

	return final_ret;
}