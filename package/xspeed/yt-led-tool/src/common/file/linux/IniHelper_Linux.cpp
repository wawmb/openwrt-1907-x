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

#include "IniHelper_Linux.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>


IniHelper_Linux::IniHelper_Linux()
	: IniHelper<char>()
	, m_pIniConfig(nullptr)
{
}

IniHelper_Linux::~IniHelper_Linux()
{
	close();
}

int IniHelper_Linux::open(const char* szFilePath, bool bCreateIfnotExists)
{
	int ret = 0;

	if (access(szFilePath, F_OK) != 0)
	{
		if (!bCreateIfnotExists)
			return -1;

		int fd = ::creat(szFilePath, 0777);
		if (fd == -1)
			return -2;
		else
			::close(fd);
	}

	m_pIniConfig = cnf_read_config(szFilePath, ';', '=');
	if (m_pIniConfig == nullptr)
		return -3;

	m_tcsFilePath = szFilePath;

	return ret;
}

bool IniHelper_Linux::isOpen()
{
	return m_pIniConfig != nullptr;
}

void IniHelper_Linux::close()
{
	if (m_pIniConfig)
	{
		free(m_pIniConfig);
		m_pIniConfig = nullptr;
	}
}

int IniHelper_Linux::getValue(const char* szSection, const char* szKey, std::basic_string<char>& value)
{
	value = "";

	if (cnf_get_value(m_pIniConfig, szSection, szKey))
	{
		value = m_pIniConfig->re_string;
		return 0;
	}
		
	return -1;
}

int IniHelper_Linux::getValue(const char* szSection, const char* szKey, int& value)
{
	value = 0;

	if (cnf_get_value(m_pIniConfig, szSection, szKey))
	{
		value = m_pIniConfig->re_int;
		return 0;
	}

	return -1;
}

int IniHelper_Linux::setValue(const char* szSection, const char* szKey, const char* szValue)
{
	if (!cnf_add_option(m_pIniConfig, szSection, szKey, szValue))
		return -1;

	return 0;
}

int IniHelper_Linux::save()
{
	if (!cnf_write_file(m_pIniConfig, m_tcsFilePath.c_str(), ""))
		return -1;

	return 0;
}