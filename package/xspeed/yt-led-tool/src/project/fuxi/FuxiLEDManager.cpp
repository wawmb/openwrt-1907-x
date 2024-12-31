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

#include "FuxiLEDManager.h"

#include "utility/CrossPlatformUtility.h"


#define EFUSE_LED_CONFIG_MAX_COUNT			2


FuxiLEDManager::FuxiLEDManager()
	: m_pFuxiEFuseManager(nullptr)
{

}

FuxiLEDManager::~FuxiLEDManager()
{
	release();
}

int FuxiLEDManager::initialize(FuxiEFuseManager* pFuxiEFuseManager)
{
	int ret = 0;

	release();

	if (pFuxiEFuseManager == nullptr)
		return -1;

	m_pFuxiEFuseManager = pFuxiEFuseManager;

	return ret;
}

void FuxiLEDManager::release()
{
	m_pFuxiEFuseManager = nullptr;
}

int FuxiLEDManager::applyLEDSolution(const uint8_t& solution)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	uint8_t uCurrentSolution = 0;
	if ((ret = m_pFuxiEFuseManager->ReadLEDSolution(uCurrentSolution)) != 0)
		return ret;

	if (uCurrentSolution != solution)
	{
		if ((ret = m_pFuxiEFuseManager->writeLEDSolution(solution)) != 0)
			return ret;
	}

	return ret;
}

int FuxiLEDManager::readLEDSolution(uint8_t& solution)
{
	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	return m_pFuxiEFuseManager->ReadLEDSolution(solution);
}

int FuxiLEDManager::setLEDConfigMode()
{
	return applyLEDSolution(EFUSE_LED_CONFIG_MODE_VALUE);
}

int FuxiLEDManager::simulateLEDSetting(const led_setting& ledSetting)
{
#ifdef   _WIN32
	return 1;
#endif //   _WIN32

	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if ((ret = applyLEDSolution(EFUSE_LED_CONFIG_MODE_VALUE)) != 0)
		return ret;

	if ((ret = m_pFuxiEFuseManager->simulateLEDSetting(ledSetting)) != 0)
		return ret;

	return ret;
}

int FuxiLEDManager::applyLEDSetting(const led_setting& ledSetting)
{
	int ret = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	if ((ret = applyLEDSolution(EFUSE_LED_CONFIG_MODE_VALUE)) != 0)
		return ret;

	if ((ret = m_pFuxiEFuseManager->updateLEDConfig(ledSetting)) != 0)
		return ret;

	if ((ret = m_pFuxiEFuseManager->RefreshPatchItems()) != 0)
		return ret;

	return ret;
}

int FuxiLEDManager::verifyLEDSetting(const led_setting& ledSetting)
{
	int ret = 0;

	led_setting eFuseLEDSetting = { 0 };
	if ((ret = loadLEDSettingfromeFuse(eFuseLEDSetting)) != 0)
		return ret;

	/*for (int i = 0; i < 5; i++)
	{
		printf("eFuseLEDSetting 0x%04X, 0x%04X, 0x%04X, 0x%04X\n", 
			eFuseLEDSetting.s0_led_setting[i], eFuseLEDSetting.s3_led_setting[i], eFuseLEDSetting.s5_led_setting[i], eFuseLEDSetting.disable_led_setting[i]);

		printf("ledSetting 0x%04X, 0x%04X, 0x%04X, 0x%04X\n",
			ledSetting.s0_led_setting[i], ledSetting.s3_led_setting[i], ledSetting.s5_led_setting[i], ledSetting.disable_led_setting[i]);
	}*/

	return memcmp(&eFuseLEDSetting, &ledSetting, sizeof(ledSetting));
}

uint8_t FuxiLEDManager::getConfigMaxWriteCount()
{
	return EFUSE_LED_CONFIG_MAX_COUNT;
}

int FuxiLEDManager::getConfigWrittenCount(uint8_t& count)
{
	int ret = 0;

	count = 0;

	if (m_pFuxiEFuseManager == nullptr)
		return -1;

	// last 4 bytes
	for (uint32_t i = EFUSE_BYTE_COUNT - 1; i >= EFUSE_BYTE_COUNT - 4; i--)
	{
		uint32_t uValue = 0;
		if ((ret = m_pFuxiEFuseManager->ReadRegValue(i, uValue)) != 0)
			return ret;

		if (uValue != 0)
		{
			count++;
			break;
		}
	}

	// check by patch, first setting use 7 patchs
	if (count == 0)
	{
		for (uint8_t i = EFUSE_REGION_C_PATCH_COUNT - 1; i >= EFUSE_REGION_C_PATCH_COUNT - 7; i--)
		{
			if (!m_pFuxiEFuseManager->IsPatchUnused(i))
			{
				count++;
				break;
			}
		}
	}

	// check by patch, second setting use 8 patchs
	for (uint8_t i = EFUSE_REGION_C_PATCH_COUNT - 7 - 1; i >= EFUSE_REGION_C_PATCH_COUNT - 7 - 8; i--)
	{
		if (!m_pFuxiEFuseManager->IsPatchUnused(i))
		{
			count++;
			break;
		}
	}

	return ret;
}

int FuxiLEDManager::loadLEDSettingfromeFuse(led_setting& ledSetting)
{
	int ret = 0;

	uint8_t uCount = 0;
	if ((ret = getConfigWrittenCount(uCount)) != 0)
		return ret;

	if (uCount == 0)
		return 1;

	uint32_t uStartOffset = 256 - 40;
	if (uCount > 1)
		uStartOffset = 256 - 40 - 6 - 40;
	
	for (std::size_t i = 0; i < sizeof(ledSetting.s0_led_setting) / sizeof(ledSetting.s0_led_setting[0]); i++)
	{
		uint16_t uValue = 0;
		if ((ret = m_pFuxiEFuseManager->ReadRegWORD(uStartOffset, uValue)) != 0)
			return ret;

		ledSetting.s0_led_setting[i] = uValue;
		uStartOffset += 2;
	}

	for (std::size_t i = 0; i < sizeof(ledSetting.s3_led_setting) / sizeof(ledSetting.s3_led_setting[0]); i++)
	{
		uint16_t uValue = 0;
		if ((ret = m_pFuxiEFuseManager->ReadRegWORD(uStartOffset, uValue)) != 0)
			return ret;

		ledSetting.s3_led_setting[i] = uValue;
		uStartOffset += 2;
	}

	for (std::size_t i = 0; i < sizeof(ledSetting.s5_led_setting) / sizeof(ledSetting.s5_led_setting[0]); i++)
	{
		uint16_t uValue = 0;
		if ((ret = m_pFuxiEFuseManager->ReadRegWORD(uStartOffset, uValue)) != 0)
			return ret;

		ledSetting.s5_led_setting[i] = uValue;
		uStartOffset += 2;
	}

	for (std::size_t i = 0; i < sizeof(ledSetting.disable_led_setting) / sizeof(ledSetting.disable_led_setting[0]); i++)
	{
		uint16_t uValue = 0;
		if ((ret = m_pFuxiEFuseManager->ReadRegWORD(uStartOffset, uValue)) != 0)
			return ret;

		ledSetting.disable_led_setting[i] = uValue;
		uStartOffset += 2;
	}

	return ret;
}