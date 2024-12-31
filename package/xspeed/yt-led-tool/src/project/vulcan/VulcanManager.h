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

#include <map>

#include "fuxi/FuxiDriverInterface.h"
#include "network/EthernetDefine.h"
#include "utility/CrossPlatformUtility.h"


#define RISE_FALL_TIME_STEP_COUNT									8

#define PHY_TEMPLATE_EXT_REG_51										0x51
#define PHY_TEMPLATE_EXT_REG_57										0x57

#define PHY_TEMPLATE_RISE_FALL_TIME_STEP_CONTROL					0x4A
#define PHY_TEMPLATE_RISE_FALL_TIME_STEP_1							0x40


enum PHY_TEMPLATE_10M_TEST_MODE
{
	MODE_10M_HARMONIC = 1,
	MODE_10M_VOLTAGE = 2,
	MODE_10M_NORMAL_PULSE_ONLY = 3,
	MODE_10M_5MHZ_SINE_WAVE = 4,
	MODE_10M_NORMAL = 5,

	MODE_10M_UNKNOWN = 0xFF
};

enum PHY_TEMPLATE_1000M_TEST_MODE
{
	MODE_1000M_TRANS_WAVEFORM = 1,
	MODE_1000M_TRANS_JITTER_MASTER = 2,
	MODE_1000M_TRANS_JITTER_SLAVE = 3,
	MODE_1000M_TRANS_DISTORTION = 4,

	MODE_1000M_UNKNOWN = 0xFF
};

struct PHY_TEMPLATE_OUTPUT_AMPLITUDE
{
	uint16_t coarseTuning1000M;
	uint16_t fineTuning1000M;

	uint16_t coarseTuning100M;
	uint16_t fineTuning100M;

	uint16_t coarseTuning10M;
	uint16_t fineTuning10M;

	uint16_t outputImpedance10M;
};

struct PHY_TEMPLATE_RISE_FALL_TIME_STEP
{
	bool stepEnabled;
	uint16_t step[RISE_FALL_TIME_STEP_COUNT];
};

struct PHY_TEMPLATE_CONFIG
{
	uint16_t driverCurrent;
	PHY_TEMPLATE_OUTPUT_AMPLITUDE outputAmplitude;
	PHY_TEMPLATE_RISE_FALL_TIME_STEP riseFallTimeStep;
	uint16_t lowPassFilter;
};

typedef std::map<uint32_t, uint32_t>	PHY_TEMPLATE_REG_MAP;
typedef std::pair<uint32_t, uint32_t>	PHY_TEMPLATE_REG_PAIR;


class VulcanManager
{
public:
	VulcanManager();
	virtual ~VulcanManager();

	virtual int initialize(FuxiDriverInterface* pFuxiDriverInterface);
	virtual void release();

	virtual int setSleepMode(bool isEnable);
	virtual int setAutoNegotiation(bool isEnable);

	virtual int setSpeed(ETHERNET_SPEED speed, bool softwareReset = true);

	virtual int checkSoftwareReset(uint32_t timeout_ms, uint32_t intervel_ms = 10);
	virtual int checkLinkup(uint32_t timeout_ms, uint32_t intervel_ms = 10);

	virtual int readReg(uint32_t addr, uint32_t& value);
	virtual int writeReg(uint32_t addr, uint32_t value);

	virtual int readExtReg(uint32_t addr, uint32_t& value);
	virtual int writeExtReg(uint32_t addr, uint32_t value);

	virtual int softwareRet();

	// Template
	virtual int setAccessSerdes(bool enable);
	virtual int setPolarityReversal(bool enable);

	virtual int setTemplateTestMode(ETHERNET_SPEED speed, uint32_t mode);
	virtual int setTemplate10MTestMode(PHY_TEMPLATE_10M_TEST_MODE mode);
	virtual int setTemplate100MTestMode();
	virtual int setTemplate1000MTestMode(PHY_TEMPLATE_1000M_TEST_MODE mode);

	virtual int getTemplateConfig(PHY_TEMPLATE_CONFIG& templateConfig);
	virtual int setTemplateConfig(const PHY_TEMPLATE_CONFIG& templateConfig);

	virtual int convertTemplateConfigtoReg(const PHY_TEMPLATE_CONFIG& templateConfig, PHY_TEMPLATE_REG_MAP& mapTemplateReg);
	virtual int convertTemplateRegtoConfig(const PHY_TEMPLATE_REG_MAP& mapTemplateReg, PHY_TEMPLATE_CONFIG& templateConfig);

	virtual int getTemplateConfig(PHY_TEMPLATE_REG_MAP& mapTemplateReg);
	virtual int setTemplateConfig(const PHY_TEMPLATE_REG_MAP& mapTemplateReg);

	// Driver Current
	virtual void getDriverCurrentScope(uint32_t& min, uint32_t& max);
	virtual int getDriverCurrent(uint32_t& value);

	// Output Amplitude
	virtual void getOutputAmplitudeCoarseTuningScope(uint32_t& min, uint32_t& max);
	virtual void getOutputAmplitudeFineTuningScope(uint32_t& min, uint32_t& max);
	virtual void getOutputAmplitude10MOutputImpedanceScope(uint32_t& min, uint32_t& max);

	virtual int getOutputAmplitudeCoarseTuning(ETHERNET_SPEED speed, uint32_t& value);
	virtual int getOutputAmplitudeFineTuning(ETHERNET_SPEED speed, uint32_t& value);
	virtual int getOutputAmplitude10MOutputImpedance(uint32_t& value);

	// Rise/Fall time step
	virtual int getRiseFallTimeStepEnable(bool& enable);
	virtual int setRiseFallTimeStepEnable(bool enable);

	virtual void getRiseFallTimeStepScope(uint8_t index, uint32_t& min, uint32_t& max);

	virtual int getRiseFallTimeStep(uint8_t index, uint32_t& value);
	virtual int setRiseFallTimeStep(uint8_t index, uint32_t value);

	// Low Pass Filter
	virtual void getLowPassFilterScope(uint32_t& min, uint32_t& max);
	virtual int getLowPassFilterScope(uint32_t& value);
	virtual int setLowPassFilterScope(uint32_t value);

	virtual int exportTemplateConfig(const TCHAR* szFilePath);
	virtual int importTemplateConfig(const TCHAR* szFilePath, PHY_TEMPLATE_CONFIG& templateConfig);

	// Loopback
	//Internal Loopback
	virtual int getInternalLoopbackStatus(bool& enable);
	virtual int setInternalLoopbackStatus(bool enable);
	
	// External Loopback
	virtual int getExternalLoopbackStatus(bool& enable);
	virtual int setExternalLoopbackStatus(bool enable);
	
	// Remote Loopback
	virtual int getRemoteLoopbackStatus(bool& enable, ETHERNET_SPEED& speed);
	virtual int setRemoteLoopbackStatus(bool enable, ETHERNET_SPEED speed = ETHERNET_SPEED::SPEED_AUTO);

	//Deplux
	//virtual int setFullDepluxMode(ETHERNET_SPEED speed, bool enable);
	//virtual int getFullDepluxMode(ETHERNET_SPEED speed, bool& enable);

	//virtual int setHalfDepluxMode(ETHERNET_SPEED speed, bool enable);
	//virtual int getHalfDepluxMode(ETHERNET_SPEED speed, bool& enable);

	// Counter
	virtual int getUTPCounterStatus(bool& enable);
	virtual int setUTPCounterStatus(bool enable);

protected:
	FuxiDriverInterface* m_pFuxiDriverInterface;
};

