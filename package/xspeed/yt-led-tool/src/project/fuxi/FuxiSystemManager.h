#pragma once

#include <stdint.h>

#include "fuxi/FuxiDefine.h"
#include "fuxi/FuxiDeviceHelper.h"


class FuxiSystemManager
{
public:
	FuxiSystemManager();
	virtual ~FuxiSystemManager();

	virtual int initialize(uint8_t devIndex, FuxiDeviceHelper* fuxiDeviceHelper) = 0;
	virtual void release() = 0;

	virtual int setVirtualMacAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) = 0;
	virtual int getVirtualMacAddress(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) = 0;

	virtual int removeVirtualMacAddress() = 0;

protected:
	FuxiDeviceHelper* m_fuxiDeviceHelper;
};

