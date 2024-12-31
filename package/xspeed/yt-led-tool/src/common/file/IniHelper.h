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

#include <string>


template <typename char_t>
class IniHelper
{
public:
	IniHelper() = default;
	virtual ~IniHelper() = default;

	virtual int open(const char_t* szFilePath, bool bCreateIfnotExists = false) = 0;
	virtual bool isOpen() = 0;
	virtual void close() = 0;

	virtual std::basic_string<char_t> getFilePath() { return m_tcsFilePath; }

	virtual int getValue(const char_t* szSection, const char_t* szKey, std::basic_string<char_t>& value) = 0;
	virtual int getValue(const char_t* szSection, const char_t* szKey, int& value) = 0;

	virtual int setValue(const char_t* szSection, const char_t* szKey, const char_t* szValue) = 0;

	virtual int save() = 0;

protected:
	std::basic_string<char_t> m_tcsFilePath;
};
