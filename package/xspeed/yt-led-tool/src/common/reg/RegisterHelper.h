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

#include <typeinfo>
#include <stdint.h>

#if  __WORDSIZE ==  64
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif


#define BITS_PER_LONG_LONG 64

#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & \
	 (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

__inline uint32_t GET_REG_BITS(uint32_t var, uint32_t pos, uint32_t len)
{
    uint32_t _pos = (pos);
    uint32_t _len = (len);
    uint32_t v = ((var)&GENMASK(_pos + _len - 1, _pos)) >> (_pos);

    return v;
}

__inline uint32_t SET_REG_BITS(uint32_t var, uint32_t pos, uint32_t len, uint32_t val)
{
    uint32_t _var = (var);
    uint32_t _pos = (pos);
    uint32_t _len = (len);
    uint32_t _val = (val);
    _val = (_val << _pos) & GENMASK(_pos + _len - 1, _pos);
    _var = (_var & ~GENMASK(_pos + _len - 1, _pos)) | _val;

    return _var;
}

__inline uint64_t GET_REG_BITS(uint64_t var, uint64_t pos, uint64_t len)
{
    uint64_t _pos = (pos);
    uint64_t _len = (len);
    uint64_t v = ((var)&GENMASK_ULL(_pos + _len - 1, _pos)) >> (_pos);

    return v;
}

__inline uint64_t SET_REG_BITS(uint64_t var, uint64_t pos, uint64_t len, uint64_t val)
{
    uint64_t _var = (var);
    uint64_t _pos = (pos);
    uint64_t _len = (len);
    uint64_t _val = (val);
    _val = (_val << _pos) & GENMASK_ULL(_pos + _len - 1, _pos);
    _var = (_var & ~GENMASK_ULL(_pos + _len - 1, _pos)) | _val;

    return _var;
}