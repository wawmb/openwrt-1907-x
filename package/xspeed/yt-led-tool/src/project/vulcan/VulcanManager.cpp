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

#include "VulcanManager.h"

#ifdef _WIN32
#include <Windows.h>

#include "file/windows/IniHelper_Win.h"
#else
#include <unistd.h>

#include "file/linux/IniHelper_Linux.h"
#endif // _WIN32

#include "reg/RegisterHelper.h"


#define PHY_EXT_REG_ADDR_PORT					0x1E
#define PHY_EXT_REG_DATA_PORT					0x1F

#define PHY_TEMPLATE_DRIVER_CURRENT									PHY_TEMPLATE_EXT_REG_51
#define PHY_TEMPLATE_OUTPUT_AMPLITUDE_1000M							PHY_TEMPLATE_EXT_REG_51
#define PHY_TEMPLATE_OUTPUT_AMPLITUDE_100M							PHY_TEMPLATE_EXT_REG_57
#define PHY_TEMPLATE_OUTPUT_AMPLITUDE_10M							PHY_TEMPLATE_EXT_REG_57
#define PHY_TEMPLATE_OUTPUT_AMPLITUDE_10M_OUTPUT_IMPEDANCE			PHY_TEMPLATE_EXT_REG_51

#define PHY_TEMPLATE_LOW_PASS_FILTER								PHY_TEMPLATE_EXT_REG_51

#define PHY_EXTERNAL_LOOPBACK_CTRL									0x0A
#define PHY_REMOTE_LOOPBACK_CTRL									0xA006
#define PHY_10M_100M_DEPLUX_CTRL									0x04
#define PHY_1000M_DEPLUX_CTRL										0x09
#define PHY_PKG_CFG_0												0xA0

#define	PHY_TEMPLATE_SECTION_NAME									_T("PHY Template")
#define	FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME						_T("PHYExt%02X")


VulcanManager::VulcanManager()
	:m_pFuxiDriverInterface(nullptr)
{
}

VulcanManager::~VulcanManager()
{
}

int VulcanManager::initialize(FuxiDriverInterface* pFuxiDriverInterface)
{
	int ret = 0;

	if (pFuxiDriverInterface == nullptr)
		return -1;

	m_pFuxiDriverInterface = pFuxiDriverInterface;

	return ret;
}

void VulcanManager::release()
{
	m_pFuxiDriverInterface = nullptr;
}

int VulcanManager::setSleepMode(bool isEnable)
{
	int ret = 0;

	uint32_t uValue = 0;

	if ((ret = readExtReg(0x27, uValue)) != 0)
		return ret;

	// bit 15
	if (isEnable)
		SET_REG_BITS(uValue, 15, 1, 1);
	else
		SET_REG_BITS(uValue, 15, 1, 0);

	if ((ret = writeExtReg(0x27, uValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setAutoNegotiation(bool isEnable)
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = m_pFuxiDriverInterface->readPHYReg(0x00, uValue)) != 0)
		return ret;

	// bit 15 set to 1
	uValue = SET_REG_BITS(uValue, 15, 1, 1);

	// bit 12 to enable/disable Auto-Negotiation
	uValue = SET_REG_BITS(uValue, 12, 1, (uint32_t)isEnable);

	// bit 9 to restart Auto-Negotiation
	//uValue = SET_REG_BITS(uValue, 9, 1, (uint32_t)isEnable);

	if ((ret = m_pFuxiDriverInterface->writePHYReg(0x00, uValue)) != 0)
		return ret;

	// wait reset finished
	//if ((ret = WaitPHYSoftwareReset(100)) != 0)
	//	return ret;

	return ret;
}

int VulcanManager::setSpeed(ETHERNET_SPEED speed, bool softwareReset)
{
	int ret = 0;

	switch (speed)
	{
	case ETHERNET_SPEED::SPEED_1000M:
		ret = m_pFuxiDriverInterface->writePHYReg(0x00, softwareReset? 0x8140 : 0x0140);
		break;
	case ETHERNET_SPEED::SPEED_100M:
		ret = m_pFuxiDriverInterface->writePHYReg(0x00, softwareReset ? 0xA100 : 0x2100);
		break;
	case ETHERNET_SPEED::SPEED_10M:
		ret = m_pFuxiDriverInterface->writePHYReg(0x00, softwareReset ? 0x8100 : 0x0100);
		break;
	case ETHERNET_SPEED::SPEED_AUTO:
		ret = setAutoNegotiation(true);
		break;
	default:
		ret = 1;
		break;
	}

	if (ret != 0)
		return ret;

	// wait reset finished
	//if ((ret = WaitPHYSoftwareReset(100)) != 0)
	//	return ret;

	return ret;
}

int VulcanManager::checkSoftwareReset(uint32_t timeout_ms, uint32_t intervel_ms)
{
	int ret = 1;

	uint32_t uValue = 0;
	uint32_t waitTime = 0;

	while (waitTime < timeout_ms)
	{
		if ((ret = m_pFuxiDriverInterface->readPHYReg(0x00, uValue)) != 0)
			return ret;

		// bit 15 should be 0
		if (!GET_REG_BITS(uValue, 15, 1))
			return 0;

		sleep_ms(intervel_ms);
		
		waitTime += intervel_ms;
	}

	return ret;
}

int VulcanManager::checkLinkup(uint32_t timeout_ms, uint32_t intervel_ms)
{
	int ret = 0;

	uint32_t uValue = 0;
	uint32_t waitTime = 0;

	while (waitTime < timeout_ms)
	{
		if ((ret = m_pFuxiDriverInterface->readPHYReg(0x01, uValue)) != 0)
			return ret;

		// bit 2 should be 1
		if (GET_REG_BITS(uValue, 2, 1))
			return 0;
		else
			ret = 1;

		sleep_ms(intervel_ms);

		waitTime += intervel_ms;
	}

	return ret;
}

int VulcanManager::readReg(uint32_t addr, uint32_t& value)
{
	return m_pFuxiDriverInterface->readPHYReg(addr, value);
}

int VulcanManager::writeReg(uint32_t addr, uint32_t value)
{
	return m_pFuxiDriverInterface->writePHYReg(addr, value);
}

int VulcanManager::readExtReg(uint32_t addr, uint32_t& value)
{
	int ret = 0;

	if ((ret = m_pFuxiDriverInterface->writePHYReg(PHY_EXT_REG_ADDR_PORT, addr)) != 0)
		return ret;

	if ((ret = m_pFuxiDriverInterface->readPHYReg(PHY_EXT_REG_DATA_PORT, value)) != 0)
		return ret;

	return ret;
}

int VulcanManager::writeExtReg(uint32_t addr, uint32_t value)
{
	int ret = 0;

	if ((ret = m_pFuxiDriverInterface->writePHYReg(PHY_EXT_REG_ADDR_PORT, addr)) != 0)
		return ret;

	if ((ret = m_pFuxiDriverInterface->writePHYReg(PHY_EXT_REG_DATA_PORT, value)) != 0)
		return ret;

	return ret;
}

int VulcanManager::softwareRet()
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = readReg(0x00, uValue)) != 0)
		return ret;

	// bit 15 set to 1
	SET_REG_BITS(uValue, 15 , 1, 1);

	if ((ret = writeReg(0x00, uValue)) != 0)
		return ret;

	// wait reset finished
	if ((ret = checkSoftwareReset(100)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setAccessSerdes(bool enable)
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = readExtReg(0xA000, uValue)) != 0)
		return ret;

	uValue = SET_REG_BITS(uValue, 1, 1, enable? 1 : 0);

	if ((ret = writeExtReg(0xA000, uValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setPolarityReversal(bool enable)
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = readReg(0x10, uValue)) != 0)
		return ret;

	uValue = SET_REG_BITS(uValue, 1, 1, enable ? 1 : 0);

	if ((ret = writeReg(0x10, uValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setTemplateTestMode(ETHERNET_SPEED speed, uint32_t mode)
{
	int ret = 0;

	if ((ret = setAccessSerdes(false)) != 0)
		return ret;

	if ((ret = setSleepMode(false)) != 0)
		return ret;

	switch (speed)
	{
	case SPEED_10M:
		ret = setTemplate10MTestMode((PHY_TEMPLATE_10M_TEST_MODE)mode);
		break;
	case SPEED_100M:
		ret = setTemplate100MTestMode();
		break;
	case SPEED_1000M:
		ret = setTemplate1000MTestMode((PHY_TEMPLATE_1000M_TEST_MODE)mode);
		break;
	case SPEED_AUTO:
	default:
		ret = 1;
		break;
	}

	return ret;
}

int VulcanManager::setTemplate10MTestMode(PHY_TEMPLATE_10M_TEST_MODE mode)
{
	int ret = 0;

	if ((ret = setSpeed(ETHERNET_SPEED::SPEED_10M)) != 0)
		return ret;

	switch (mode)
	{
	case MODE_10M_HARMONIC:
		ret = writeExtReg(0x0A, 0x0209);
		break;
	case MODE_10M_VOLTAGE:
		ret = writeExtReg(0x0A, 0x020A);
		break;
	case MODE_10M_NORMAL_PULSE_ONLY:
		ret = writeExtReg(0x0A, 0x020B);
		break;
	case MODE_10M_5MHZ_SINE_WAVE:
		ret = writeExtReg(0x0A, 0x020C);
		break;
	case MODE_10M_NORMAL:
		ret = writeExtReg(0x0A, 0x020D);
		break;
	default:
		return -1;
	}

	return ret;
}

int VulcanManager::setTemplate100MTestMode()
{
	int ret = 0;

	if ((ret = writeReg(0x10, 0x02)) != 0)
		return ret;

	if ((ret = setSpeed(ETHERNET_SPEED::SPEED_100M)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setTemplate1000MTestMode(PHY_TEMPLATE_1000M_TEST_MODE mode)
{
	int ret = 0;

	if ((ret = writeReg(0x10, 0x02)) != 0)
		return ret;

	if ((ret = setSpeed(ETHERNET_SPEED::SPEED_1000M)) != 0)
		return ret;

	switch (mode)
	{
	case MODE_1000M_TRANS_WAVEFORM:
		if ((ret = writeReg(0x09, 0x2200)) != 0)
			return ret;
		break;
	case MODE_1000M_TRANS_JITTER_MASTER:
		if ((ret = writeReg(0x09, 0x5A00)) != 0)
			return ret;
		break;
	case MODE_1000M_TRANS_JITTER_SLAVE:
		if ((ret = writeReg(0x09, 0x7200)) != 0)
			return ret;
		break;
	case MODE_1000M_TRANS_DISTORTION:
		if ((ret = writeReg(0x09, 0x8200)) != 0)
			return ret;
		break;
	default:
		return -1;
	}

	if ((ret = writeExtReg(0x00, 0x8140)) != 0)
		return ret;

	return ret;
}

int VulcanManager::getTemplateConfig(PHY_TEMPLATE_CONFIG& templateConfig)
{
	int ret = 0;

	PHY_TEMPLATE_REG_MAP mapTemplateReg;

	if ((ret = getTemplateConfig(mapTemplateReg)) != 0)
		return ret;

	if ((ret = convertTemplateRegtoConfig(mapTemplateReg, templateConfig)) != 0)
		return ret;

	return ret;
}

int VulcanManager::setTemplateConfig(const PHY_TEMPLATE_CONFIG& templateConfig)
{
	int ret = 0;

	PHY_TEMPLATE_REG_MAP mapTemplateReg;

	if ((ret = convertTemplateConfigtoReg(templateConfig, mapTemplateReg)) != 0)
		return ret;

	if ((ret = setTemplateConfig(mapTemplateReg)) != 0)
		return ret;

	return ret;
}

int VulcanManager::convertTemplateConfigtoReg(const PHY_TEMPLATE_CONFIG& templateConfig, PHY_TEMPLATE_REG_MAP& mapTemplateReg)
{
	int ret = 0;

	mapTemplateReg.clear();

	// ext 0x51:
	// bit 3:0 1000Mbps Fine Tuning
	// bit 7:4 Driver current
	// bit 11:9 1000Mbps Coarse Tuning
	// bit 13:12 Low Pass Filter
	// bit 14 Output impedance

	// Apply ext 0x51
	uint32_t uExtReg51 = 0;
	if ((ret = readExtReg(PHY_TEMPLATE_EXT_REG_51, uExtReg51)) != 0)
		return ret;

	uExtReg51 = SET_REG_BITS(uExtReg51, 0, 4, templateConfig.outputAmplitude.fineTuning1000M);
	uExtReg51 = SET_REG_BITS(uExtReg51, 4, 4, templateConfig.driverCurrent);
	uExtReg51 = SET_REG_BITS(uExtReg51, 9, 3, templateConfig.outputAmplitude.coarseTuning1000M);
	uExtReg51 = SET_REG_BITS(uExtReg51, 12, 2, templateConfig.lowPassFilter);
	uExtReg51 = SET_REG_BITS(uExtReg51, 14, 1, templateConfig.outputAmplitude.outputImpedance10M);

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_51, uExtReg51));

	// ext 0x57:
	// bit 3:0 10Mbps Fine Tuning
	// bit 6:4 10Mbps Coarse Tuning
	// bit 11:8 100Mbps Fine Tuning
	// bit 14:12 100Mbps Coarse Tuning

	// Apply ext 0x57
	uint32_t uExtReg57 = 0;
	if ((ret = readExtReg(PHY_TEMPLATE_EXT_REG_57, uExtReg57)) != 0)
		return ret;

	uExtReg57 = SET_REG_BITS(uExtReg57, 0, 4, templateConfig.outputAmplitude.fineTuning10M);
	uExtReg57 = SET_REG_BITS(uExtReg57, 4, 3, templateConfig.outputAmplitude.coarseTuning10M);
	uExtReg57 = SET_REG_BITS(uExtReg57, 8, 4, templateConfig.outputAmplitude.fineTuning100M);
	uExtReg57 = SET_REG_BITS(uExtReg57, 12, 3, templateConfig.outputAmplitude.coarseTuning100M);

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_57, uExtReg57));

	// Rise/Fall Time Step enable
	uint32_t uRiseFallTimeStepControl = 0;
	if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRiseFallTimeStepControl)) != 0)
		return ret;

	uRiseFallTimeStepControl = SET_REG_BITS(uRiseFallTimeStepControl, 7, 1, templateConfig.riseFallTimeStep.stepEnabled);

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRiseFallTimeStepControl));

	// ext 0x40~0x47 Rise/Fall Time Step 1~8
	for (int i = 0; i < RISE_FALL_TIME_STEP_COUNT; i++)
	{
		uint32_t uStep = 0;
		if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i, uStep)) != 0)
			return ret;

		uint32_t uLen = 4;
		if (i > 3)
			uLen = 5;

		uStep = SET_REG_BITS(uStep, 0, uLen, templateConfig.riseFallTimeStep.step[i]);

		mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i, uStep));
	}

	//// Rise/Fall Time Step enable
	//if ((ret = setRiseFallTimeStepEnable(templateConfig.riseFallTimeStep.stepEnabled)) != 0)
	//	return ret;

	//// ext 0x40~0x47 Rise/Fall Time Step 1~8
	//for (int i = 0; i < RISE_FALL_TIME_STEP_COUNT; i++)
	//{
	//	if ((ret = setRiseFallTimeStep(i + 1, templateConfig.riseFallTimeStep.step[i])) != 0)
	//		return ret;
	//}

	return ret;
}

int VulcanManager::convertTemplateRegtoConfig(const PHY_TEMPLATE_REG_MAP& mapTemplateReg, PHY_TEMPLATE_CONFIG& templateConfig)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	// ext 0x51:
	// bit 3:0 1000Mbps Fine Tuning
	// bit 7:4 Driver current
	// bit 11:9 1000Mbps Coarse Tuning
	// bit 13:12 Low Pass Filter
	// bit 14 10M Output impedance

	auto iter = mapTemplateReg.end();

	if ((iter = mapTemplateReg.find(PHY_TEMPLATE_EXT_REG_51)) == mapTemplateReg.end())
		return 1;

	uRegValue = iter->second;

	templateConfig.outputAmplitude.fineTuning1000M = (uint16_t)GET_REG_BITS(uRegValue, 0, 4);
	templateConfig.driverCurrent = (uint16_t)GET_REG_BITS(uRegValue, 4, 4);
	templateConfig.outputAmplitude.coarseTuning1000M = (uint16_t)GET_REG_BITS(uRegValue, 9, 3);
	templateConfig.lowPassFilter = (uint16_t)GET_REG_BITS(uRegValue, 12, 2);
	templateConfig.outputAmplitude.outputImpedance10M = (uint16_t)GET_REG_BITS(uRegValue, 14, 1);

	// ext 0x57:
	// bit 3:0 10Mbps Fine Tuning
	// bit 6:4 10Mbps Coarse Tuning
	// bit 11:8 100Mbps Fine Tuning
	// bit 14:12 100Mbps Coarse Tuning

	if ((iter = mapTemplateReg.find(PHY_TEMPLATE_EXT_REG_57)) == mapTemplateReg.end())
		return 1;

	uRegValue = iter->second;

	templateConfig.outputAmplitude.fineTuning10M = (uint16_t)GET_REG_BITS(uRegValue, 0, 4);
	templateConfig.outputAmplitude.coarseTuning10M = (uint16_t)GET_REG_BITS(uRegValue, 4, 3);
	templateConfig.outputAmplitude.fineTuning100M = (uint16_t)GET_REG_BITS(uRegValue, 8, 4);
	templateConfig.outputAmplitude.coarseTuning100M = (uint16_t)GET_REG_BITS(uRegValue, 12, 3);


	if ((iter = mapTemplateReg.find(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL)) == mapTemplateReg.end())
		return 1;

	uRegValue = iter->second;

	templateConfig.riseFallTimeStep.stepEnabled = GET_REG_BITS(uRegValue, 7, 1);


	for (int i = 0; i < RISE_FALL_TIME_STEP_COUNT; i++)
	{
		if ((iter = mapTemplateReg.find(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i)) == mapTemplateReg.end())
			return 1;

		uRegValue = iter->second;

		uint32_t uLen = 4;
		if (i > 3)
			uLen = 5;

		templateConfig.riseFallTimeStep.step[i] = (uint16_t)GET_REG_BITS(uRegValue, 0, uLen);
	}

	return ret;
}

int VulcanManager::getTemplateConfig(PHY_TEMPLATE_REG_MAP& mapTemplateReg)
{
	int ret = 0;

	mapTemplateReg.clear();

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_EXT_REG_51, uRegValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_51, uRegValue));

	if ((ret = readExtReg(PHY_TEMPLATE_EXT_REG_57, uRegValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_57, uRegValue));

	if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRegValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRegValue));

	for (int i = 0; i < RISE_FALL_TIME_STEP_COUNT; i++)
	{
		if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i, uRegValue)) != 0)
			return ret;

		mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i, uRegValue));
	}

	return ret;
}

int VulcanManager::setTemplateConfig(const PHY_TEMPLATE_REG_MAP& mapTemplateReg)
{
	int ret = 0;

	for (auto templateReg : mapTemplateReg)
	{
		if ((ret = writeExtReg(templateReg.first, templateReg.second)) != 0)
			return ret;
	}

	return ret;
}

void VulcanManager::getDriverCurrentScope(uint32_t& min, uint32_t& max)
{
	min = 0;
	max = 0x0F;
}

int VulcanManager::getDriverCurrent(uint32_t& value)
{
	int ret = 0;

	uint32_t uRegValue = 0;
	if ((ret = readExtReg(PHY_TEMPLATE_DRIVER_CURRENT, uRegValue)) != 0)
		return ret;

	// bit 7:4
	value = GET_REG_BITS(uRegValue, 4, 4);

	return ret;
}

void VulcanManager::getOutputAmplitudeCoarseTuningScope(uint32_t& min, uint32_t& max)
{
	min = 0;
	max = 7;
}

void VulcanManager::getOutputAmplitudeFineTuningScope(uint32_t& min, uint32_t& max)
{
	min = 0;
	max = 0x0F;
}

void VulcanManager::getOutputAmplitude10MOutputImpedanceScope(uint32_t& min, uint32_t& max)
{
	min = 0;
	max = 1;
}

int VulcanManager::getOutputAmplitudeCoarseTuning(ETHERNET_SPEED speed, uint32_t& value)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	switch (speed)
	{
	case SPEED_10M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_10M, uRegValue)) != 0)
			return ret;

		// bit 6:4
		value = GET_REG_BITS(uRegValue, 4, 3);

		break;
	case SPEED_100M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_100M, uRegValue)) != 0)
			return ret;

		// bit 14:12
		value = GET_REG_BITS(uRegValue, 12, 3);

		break;
	case SPEED_1000M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_1000M, uRegValue)) != 0)
			return ret;

		// bit 11:9
		value = GET_REG_BITS(uRegValue, 9, 3);

		break;
	case SPEED_AUTO:
	default:
		ret = 1;
		break;
	}

	return ret;
}

int VulcanManager::getOutputAmplitudeFineTuning(ETHERNET_SPEED speed, uint32_t& value)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	switch (speed)
	{
	case SPEED_10M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_10M, uRegValue)) != 0)
			return ret;

		// bit 3:0
		value = GET_REG_BITS(uRegValue, 0, 4);

		break;
	case SPEED_100M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_100M, uRegValue)) != 0)
			return ret;

		// bit 11:8
		value = GET_REG_BITS(uRegValue, 8, 4);

		break;
	case SPEED_1000M:
		if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_1000M, uRegValue)) != 0)
			return ret;

		// bit 3:0
		value = GET_REG_BITS(uRegValue, 0, 4);

		break;
	case SPEED_AUTO:
	default:
		ret = 1;
		break;
	}

	return ret;
}

int VulcanManager::getOutputAmplitude10MOutputImpedance(uint32_t& value)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_OUTPUT_AMPLITUDE_10M_OUTPUT_IMPEDANCE, uRegValue)) != 0)
		return ret;

	// bit 14
	value = GET_REG_BITS(uRegValue, 14, 1);

	return ret;
}

int VulcanManager::getRiseFallTimeStepEnable(bool& enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRegValue)) != 0)
		return ret;

	// bit 7
	enable = GET_REG_BITS(uRegValue, 7, 1);

	return ret;
}

int VulcanManager::setRiseFallTimeStepEnable(bool enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRegValue)) != 0)
		return ret;

	// bit 7
	if (enable)
		SET_REG_BITS(uRegValue, 7, 1, 1);
	else
		SET_REG_BITS(uRegValue, 7, 1, 0);

	if ((ret = writeExtReg(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, uRegValue)) != 0)
		return ret;

	return ret;
}

void VulcanManager::getRiseFallTimeStepScope(uint8_t index, uint32_t& min, uint32_t& max)
{
	min = 0;
	if(index < 4)
		max = 0x0F;
	else
		max = 0x1F;
}

int VulcanManager::getRiseFallTimeStep(uint8_t index, uint32_t& value)
{
	int ret = 0;

	uint32_t uRegAddress = PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + index;
	uint32_t uRegValue = 0;

	if ((ret = readExtReg(uRegAddress, uRegValue)) != 0)
		return ret;

	// bit 4:0
	value = GET_REG_BITS(uRegValue, 0, 4);

	return ret;
}

int VulcanManager::setRiseFallTimeStep(uint8_t index, uint32_t value)
{
	int ret = 0;

	uint32_t uRegAddress = PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + index;
	uint32_t uRegValue = 0;

	if ((ret = readExtReg(uRegAddress, uRegValue)) != 0)
		return ret;

	uRegValue |= value;

	if ((ret = writeExtReg(uRegAddress, uRegValue)) != 0)
		return ret;

	return ret;
}

void VulcanManager::getLowPassFilterScope(uint32_t& min, uint32_t& max)
{
	min = 0;
	max = 3;
}

int VulcanManager::getLowPassFilterScope(uint32_t& value)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_LOW_PASS_FILTER, uRegValue)) != 0)
		return ret;

	// bit 13:12
	value = GET_REG_BITS(uRegValue, 12, 2);

	return ret;
}

int VulcanManager::setLowPassFilterScope(uint32_t value)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_TEMPLATE_LOW_PASS_FILTER, uRegValue)) != 0)
		return ret;

	uRegValue |= (value << 12);

	if ((ret = writeExtReg(PHY_TEMPLATE_LOW_PASS_FILTER, uRegValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::exportTemplateConfig(const TCHAR* szFilePath)
{
	int ret = 0;

	PHY_TEMPLATE_REG_MAP mapTemplateReg;
	if ((ret = getTemplateConfig(mapTemplateReg)) != 0)
		return ret;

#ifdef _WIN32
	IniHelper_Win iniHelper;
#else
	IniHelper_Linux iniHelper;
#endif // _WIN32

	if ((ret = iniHelper.open(szFilePath, true)) != 0)
		return ret;

	TCHAR szKey[256] = { 0 };
	TCHAR szValue[256] = { 0 };
	
	for (auto iter : mapTemplateReg)
	{
		_stprintf_s(szKey, FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME, iter.first);
		_stprintf_s(szValue, _T("0x%04X"), iter.second);

		iniHelper.setValue(PHY_TEMPLATE_SECTION_NAME, szKey, szValue);
	}

	iniHelper.save();

	iniHelper.close();

	return ret;
}

int VulcanManager::importTemplateConfig(const TCHAR* szFilePath, PHY_TEMPLATE_CONFIG& templateConfig)
{
	int ret = 0;

#ifdef _WIN32
	IniHelper_Win iniHelper;
#else
	IniHelper_Linux iniHelper;
#endif // _WIN32

	if ((ret = iniHelper.open(szFilePath, false)) != 0)
		return ret;

	PHY_TEMPLATE_REG_MAP mapTemplateReg;

	TCHAR szKey[256] = { 0 };
	std::basic_string<TCHAR> tcsValue(_T(""));

	// ext 51
	_stprintf_s(szKey, FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME, PHY_TEMPLATE_EXT_REG_51);
	if ((ret = iniHelper.getValue(PHY_TEMPLATE_SECTION_NAME, szKey, tcsValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_51, (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16)));

	// ext 57
	_stprintf_s(szKey, FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME, PHY_TEMPLATE_EXT_REG_57);
	if ((ret = iniHelper.getValue(PHY_TEMPLATE_SECTION_NAME, szKey, tcsValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_EXT_REG_57, (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16)));

	// Rise/Fall Time Step enable
	_stprintf_s(szKey, FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME, PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL);
	if ((ret = iniHelper.getValue(PHY_TEMPLATE_SECTION_NAME, szKey, tcsValue)) != 0)
		return ret;

	mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL, (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16)));

	for (int i = 0; i < RISE_FALL_TIME_STEP_COUNT; i++)
	{
		_stprintf_s(szKey, FORMAT_PHY_TEMPLATE_REG_EXT_KEY_NAME, PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i);
		if ((ret = iniHelper.getValue(PHY_TEMPLATE_SECTION_NAME, szKey, tcsValue)) != 0)
			return ret;

		mapTemplateReg.insert(PHY_TEMPLATE_REG_PAIR(PHY_TEMPLATE_RISE_FALL_TIME_STEP_1 + i, (uint32_t)_tcstoul(tcsValue.c_str(), NULL, 16)));
	}

	iniHelper.close();

	if ((ret = setTemplateConfig(mapTemplateReg)) != 0)
		return ret;

	return ret;
}

//int VulcanManager::setFullDepluxMode(ETHERNET_SPEED speed, bool enable)
//{
//	int ret = 0;
//
//	uint32_t uRegValue = 0;
//
//	switch (speed)
//	{
//	case SPEED_10M:
//
//		if ((ret = readReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		uRegValue = SET_REG_BITS(uRegValue, 6, 1, (uint32_t)enable);
//
//		if ((ret = writeReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		break;
//	case SPEED_100M:
//
//		if ((ret = readReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		uRegValue = SET_REG_BITS(uRegValue, 8, 1, (uint32_t)enable);
//
//		if ((ret = writeReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		break;
//	case SPEED_1000M:
//
//		if ((ret = readReg(PHY_1000M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		uRegValue = SET_REG_BITS(uRegValue, 9, 1, (uint32_t)enable);
//
//		if ((ret = writeReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
//			return ret;
//
//		break;
//	case SPEED_AUTO:
//	default:
//		ret = 1;
//		break;
//	}
//
//	return ret;
//}
//
//int VulcanManager::getFullDepluxMode(ETHERNET_SPEED speed, bool& enable)
//{
//	int ret = 0;
//
//
//
//	return ret;
//}
//
//int VulcanManager::setHalfDepluxMode(ETHERNET_SPEED speed, bool enable)
//{
//	int ret = 0;
//
//
//
//	return ret;
//}
//
//int VulcanManager::getHalfDepluxMode(ETHERNET_SPEED speed, bool& enable)
//{
//	int ret = 0;
//
//
//
//	return ret;
//}

int VulcanManager::getInternalLoopbackStatus(bool& enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readReg(0x0, uRegValue)) != 0)
		return ret;

	enable = GET_REG_BITS(uRegValue, 14, 1);

	return ret;
}

int VulcanManager::setInternalLoopbackStatus(bool enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readReg(0x0, uRegValue)) != 0)
		return ret;

	bool bCurrentEnable = GET_REG_BITS(uRegValue, 14, 1);

	if (bCurrentEnable == enable)
		return 0;

	uRegValue = SET_REG_BITS(uRegValue, 14, 1, (uint32_t)enable);

	if ((ret = writeReg(0x0, uRegValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::getExternalLoopbackStatus(bool& enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_EXTERNAL_LOOPBACK_CTRL, uRegValue)) != 0)
		return ret;

	enable = GET_REG_BITS(uRegValue, 4, 1);
	
	return ret;
}

int VulcanManager::setExternalLoopbackStatus(bool enable)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_EXTERNAL_LOOPBACK_CTRL, uRegValue)) != 0)
		return ret;

	bool bCurrentEnable = GET_REG_BITS(uRegValue, 4, 1);

	if (bCurrentEnable == enable)
		return 0;

	uRegValue = SET_REG_BITS(uRegValue, 4, 1, (uint32_t)enable);

	if ((ret = writeExtReg(PHY_EXTERNAL_LOOPBACK_CTRL, uRegValue)) != 0)
		return ret;

	return ret;
}

int VulcanManager::getRemoteLoopbackStatus(bool& enable, ETHERNET_SPEED& speed)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_REMOTE_LOOPBACK_CTRL, uRegValue)) != 0)
		return ret;

	enable = GET_REG_BITS(uRegValue, 5, 1);

	if ((ret = readReg(PHY_1000M_DEPLUX_CTRL, uRegValue)) != 0)
		return ret;

	bool b1000MFullDepluxEnable = GET_REG_BITS(uRegValue, 9, 1);
	
	if ((ret = readReg(PHY_10M_100M_DEPLUX_CTRL, uRegValue)) != 0)
		return ret;

	bool b100MFullDepluxEnable = GET_REG_BITS(uRegValue, 8, 1);
	bool b10MFullDepluxEnable = GET_REG_BITS(uRegValue, 6, 1);
	
	if (b1000MFullDepluxEnable && !b100MFullDepluxEnable && !b10MFullDepluxEnable)
		speed = ETHERNET_SPEED::SPEED_1000M;
	else if (b100MFullDepluxEnable && !b1000MFullDepluxEnable && !b10MFullDepluxEnable)
		speed = ETHERNET_SPEED::SPEED_100M;
	else if (b10MFullDepluxEnable && !b1000MFullDepluxEnable && !b100MFullDepluxEnable)
		speed = ETHERNET_SPEED::SPEED_10M;
	else
		speed = ETHERNET_SPEED::SPEED_AUTO;

	return ret;
}

int VulcanManager::setRemoteLoopbackStatus(bool enable, ETHERNET_SPEED speed)
{
	int ret = 0;

	uint32_t uRegValue = 0;

	if ((ret = readExtReg(PHY_REMOTE_LOOPBACK_CTRL, uRegValue)) != 0)
		return ret;

	bool bIsEnabled = GET_REG_BITS(uRegValue, 5, 1);

	if (bIsEnabled != enable)
	{
		uRegValue = SET_REG_BITS(uRegValue, 5, 1, (uint32_t)enable);

		if ((ret = writeExtReg(PHY_REMOTE_LOOPBACK_CTRL, uRegValue)) != 0)
			return ret;
	}

	if (enable)
	{
		uint32_t u1000MRegValue = 0;
		uint32_t u10M100MRegValue = 0;

		if ((ret = readReg(PHY_1000M_DEPLUX_CTRL, u1000MRegValue)) != 0)
			return ret;

		if ((ret = readReg(PHY_10M_100M_DEPLUX_CTRL, u10M100MRegValue)) != 0)
			return ret;

		if (speed == ETHERNET_SPEED::SPEED_1000M)
		{
			u1000MRegValue = SET_REG_BITS(u1000MRegValue, 8, 2, 0x02);
			u10M100MRegValue = SET_REG_BITS(u10M100MRegValue, 5, 4, 0);
		}
		else if (speed == ETHERNET_SPEED::SPEED_100M)
		{
			u1000MRegValue = SET_REG_BITS(u1000MRegValue, 8, 2, 0x00);
			u10M100MRegValue = SET_REG_BITS(u10M100MRegValue, 5, 4, 0x08);
		}
		else if (speed == ETHERNET_SPEED::SPEED_10M)
		{
			u1000MRegValue = SET_REG_BITS(u1000MRegValue, 8, 2, 0x00);
			u10M100MRegValue = SET_REG_BITS(u10M100MRegValue, 5, 4, 0x02);
		}
		else
		{
			u1000MRegValue = SET_REG_BITS(u1000MRegValue, 8, 2, 0x02);
			u10M100MRegValue = SET_REG_BITS(u10M100MRegValue, 5, 4, 0x0A);
		}

		if ((ret = writeReg(PHY_1000M_DEPLUX_CTRL, u1000MRegValue)) != 0)
			return ret;

		if ((ret = writeReg(PHY_10M_100M_DEPLUX_CTRL, u10M100MRegValue)) != 0)
			return ret;
	}

	// Sotfware Reset
	if ((ret = softwareRet()) != 0)
		return ret;

	return ret;
}

int VulcanManager::getUTPCounterStatus(bool& enable)
{
	int ret = 0;

	uint32_t uValue = 0;
	if ((ret = readExtReg(PHY_PKG_CFG_0, uValue)) != 0)
		return ret;

	// check bit 15
	enable = GET_REG_BITS(uValue, 15, 1);

	return ret;
}

int VulcanManager::setUTPCounterStatus(bool enable)
{
	return m_pFuxiDriverInterface->setPHYPackageCounterEnable(enable);
}