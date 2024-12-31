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

#include "file/IniHelper.h"

#include <string>

extern "C" {
#include "IniInterface.h"
}


class IniHelper_Linux : public IniHelper<char>
{
public:
	IniHelper_Linux();
	virtual ~IniHelper_Linux();

	virtual int open(const char* szFilePath, bool bCreateIfnotExists = false) override;
	virtual bool isOpen() override;
	virtual void close();

	virtual int getValue(const char* szSection, const char* szKey, std::basic_string<char>& value) override;
	virtual int getValue(const char* szSection, const char* szKey, int& value) override;

	virtual int setValue(const char* szSection, const char* szKey, const char* szValue) override;

	virtual int save() override;

protected:
	Config* m_pIniConfig;
};

