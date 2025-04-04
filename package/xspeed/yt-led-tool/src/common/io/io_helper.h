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

#ifndef __IO_HELPER_H__
#define __IO_HELPER_H__

#include <stdint.h>
#include <sys/types.h>



// Memory Map
int MemRead(uint64_t uPhysAddr, void *pBuf, size_t nSize);

int MemRead8(uint64_t uPhysAddr, uint8_t *pValue);
int MemRead16(uint64_t uPhysAddr, uint16_t *pValue);
int MemRead32(uint64_t uPhysAddr, uint32_t *pValue);

int MemWrite(uint64_t uPhysAddr, void *pBuf, size_t nSize);

int MemWrite8(uint64_t uPhysAddr, uint8_t uValue);
int MemWrite16(uint64_t uPhysAddr, uint16_t uValue);
int MemWrite32(uint64_t uPhysAddr, uint32_t uValue);

#endif	//#ifndef __IO_HELPER_H__
