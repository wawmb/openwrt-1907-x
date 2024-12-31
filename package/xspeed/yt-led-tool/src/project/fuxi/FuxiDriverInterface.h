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
#include <string>

#include "fuxi/FuxiDefine.h"
#include "fuxi/FuxiDeviceHelper.h"


//#pragma pack(1)

typedef struct _MP_COUNT_STRUC {
    // Packet counts
    uint64_t                 SwInUcastPkts;
    uint64_t                 SwInUcastOctets;
    uint64_t                 SwInMulticastPkts;
    uint64_t                 SwInMulticastOctets;
    uint64_t                 SwInBroadcastPkts;
    uint64_t                 SwInBroadcastOctets;
    uint64_t                 SwOutUcastPkts;
    uint64_t                 SwOutUcastOctets;
    uint64_t                 SwOutMulticastPkts;
    uint64_t                 SwOutMulticastOctets;
    uint64_t                 SwOutBroadcastPkts;
    uint64_t                 SwOutBroadcastOctets;
    int32_t                  SwWaitSend;
    uint64_t                 HwGoodTransmits;
    uint64_t                 HwGoodRxUcastPkts;
    uint64_t                 HwGoodRxMulticastPkts;
    uint64_t                 HwGoodRxBroadcastPkts;

    // Count of transmit errors
    uint32_t                   HwTxAbortExcessCollisions;
    uint32_t                   HwTxLateCollisions;
    uint32_t                   HwTxDmaUnderrun;
    uint32_t                   HwTxLostCRS;
    uint32_t                   HwTxOKButDeferred;
    uint32_t                   HwOneRetry;
    uint32_t                   HwMoreThanOneRetry;
    uint32_t                   HwTotalRetries;

    // Count of receive errors
    uint32_t                   HwRcvCrcErrors;
    uint32_t                   HwRcvAlignmentErrors;
    uint32_t                   HwRcvResourceErrors;
    uint32_t                   HwRcvFifoOverrunErrors;
    uint32_t                   HwRcvCdtFrames;
    uint32_t                   HwRcvRuntErrors;

    //for miscellaneous interrupt counter
    uint32_t					HwEthernetPhyLinkUp;
    uint32_t					HwEthernetPhyLinkChange;
    uint32_t					HwMacMiscIntHandleCnt;
    uint32_t                   	HwMacMsiRxIntHandleCnt;
    uint32_t                   	HwMacMsiTxIntHandleCnt;
    uint32_t                  	HwMacRxOwnCompltCnt;
    uint32_t                   	HwMacTxWriteTailCnt;
    uint32_t					HwMacRxBufUnavailable;
    uint32_t					HwMacPcsPhyLinkAn;
    uint32_t					HwMacD0RxWakePkt;
    uint32_t					HwMacMMCRxCntNotClear;
    uint32_t					HwMacMMCTxCntNotClear;
    uint32_t					HwMacMMCRxCksumCntNClear;
    uint32_t					HwMacRxWatchDogTimeout;
    uint32_t					HwMacTxLateExCollision;
    uint32_t					HwMacTxLossCarrier;
    uint32_t					HwMacTxJabberTimeout;

    //for ephy statistics
    uint64_t                    UtpCheckerIBNormalPacket;
    uint64_t                    UtpCheckerIBJumboPacket;
    uint64_t                    UtpCheckerIBShortPacket;
    uint32_t                    UtpCheckerIBNormalErrorPacket;
    uint32_t                    UtpCheckerIBJumboErrorPacket;
    uint32_t                    UtpCheckerIBShortErrorPacket;
    uint32_t                    UtpCheckerIBNoSFD;

    uint64_t                    UtpCheckerOBNormalPacket;
    uint64_t                    UtpCheckerOBJumboPacket;
    uint64_t                    UtpChecker0BShortPacket;
    uint32_t                    UtpCheckerOBNormalErrorPacket;
    uint32_t                    UtpCheckerOBJumboErrorPacket;
    uint32_t                    UtpCheckerOBShortErrorPacket;
    uint32_t                    UtpCheckerOBNoSFD;
} MP_COUNT_STRUC;

struct led_setting
{
	uint32_t s0_led_setting[5];
	uint32_t s3_led_setting[5];
	uint32_t s5_led_setting[5];
	uint32_t disable_led_setting[5];
};
//#pragma pack()


class FuxiDriverInterface
{
public:
	FuxiDriverInterface() = default;
	virtual ~FuxiDriverInterface() = default;

	virtual int open(bool retry = false) = 0;
	virtual int open(uint8_t index) = 0;
	virtual int open(uint64_t memBaseAddress) = 0;
	virtual bool isOpen() = 0;
	virtual void close() = 0;

    virtual int getHandle(void* handle) = 0;

    virtual int readMemReg(uint32_t addr, uint32_t& value) = 0;
    virtual int writeMemReg(uint32_t addr, uint32_t value) = 0;

	virtual int readGMACReg(uint32_t addr, uint32_t& value) = 0;
	virtual int writeGMACReg(uint32_t addr, uint32_t value) = 0;

	virtual int readGMACConfigReg(uint32_t addr, uint32_t& value) = 0;
	virtual int writeGMACConfigReg(uint32_t addr, uint32_t value) = 0;

	virtual int readPHYReg(uint32_t addr, uint32_t& value) = 0;
	virtual int writePHYReg(uint32_t addr, uint32_t value) = 0;

	virtual int readEfuseReg(uint32_t addr, uint32_t& value) = 0;

	virtual int readEfusePatch(uint8_t index, uint32_t& addr, uint32_t& value) = 0;
	virtual int writeEfusePatch(uint8_t index, uint32_t addr, uint32_t value) = 0;

	virtual int readMACAddressfromEFuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) = 0;
	virtual int writeMACAddresstoEfuse(uint8_t(&macAddr)[MAC_ADDR_BYTE_COUNT]) = 0;

	virtual int readSubSystemIDfromEFuse(uint32_t& subDeviceID, uint32_t& subVendorID, uint32_t& revisionID) = 0;
	virtual int writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID, uint32_t revisionID) = 0;
	virtual int writeSubSystemIDtoEFuse(uint32_t subDeviceID, uint32_t subVendorID) = 0;

	virtual int writeLEDStatustoEFuse(uint8_t status) = 0;

	virtual int enableWOLtoEfuse() = 0;

	virtual int setPHYPackageCounterEnable(bool enable) = 0;

	virtual int getAllPackageCounter(MP_COUNT_STRUC& packageCounter) = 0;

	virtual int initLoopback() = 0;
	virtual int cleanLoopback() = 0;

	virtual int updateLEDConfigtoEfuse(const led_setting& ledSetting) = 0;
    virtual int simulateLEDSetting(const led_setting& ledSetting) = 0;

    virtual int getDeviceLocation(std::wstring& wcsDevLoaction) = 0;

    virtual int setDriverLoopbackMode(uint32_t mode) = 0;

    virtual int getPCIMemInfo(uint32_t& address, uint32_t& range) = 0;
    virtual int getPCIPortInfo(uint32_t& address, uint32_t& range) = 0;
    virtual int getPCIInterruptLevel(uint32_t& level) = 0;

    virtual int getGSOSize(uint32_t& size) = 0;
    virtual int setGSOSize(uint32_t size) = 0;

    virtual int getTXRXInterruptModeration(uint32_t& txIntModeration, uint32_t& rxIntModeration) = 0;
    virtual int setTXInterruptModeration(uint32_t txIntModeration) = 0;
    virtual int setRXInterruptModeration(uint32_t rxIntModeration) = 0;
};

