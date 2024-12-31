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

#include <vector>
#include <map>
#include <string>
#include <algorithm>


template <typename char_t>
class CmdHelper
{
	typedef std::vector<std::basic_string<char_t>> MODE_VEC;

	typedef std::pair<std::basic_string<char_t>, std::basic_string<char_t>> CMD_PAIR;
	typedef std::map<std::basic_string<char_t>, std::basic_string<char_t>> CMD_MAP;
public:
	CmdHelper();
	virtual ~CmdHelper();

	virtual int initialize(int argc, char_t* argv[]);

	virtual int parameterCount();
	virtual bool hasParameter();

	virtual void setModeTag(char_t tag);
	virtual void setCmdTag(char_t tag);
	virtual void setCaseSensitive(bool sensitive);

	virtual bool hasMode(const char_t* szMode);
	virtual bool hasCmd(const char_t* szCmd);
	virtual int getCmdValue(const char_t* szCmd, std::basic_string<char_t>& tcsValue);

protected:
	virtual bool isModeKey(const char_t* szMode);
	virtual bool isCmdKey(const char_t* szCmd);

	virtual typename CmdHelper<char_t>::MODE_VEC::iterator getMode(const char_t* szCmd);
	virtual typename CmdHelper<char_t>::CMD_MAP::iterator getCmd(const char_t* szCmd);

protected:
	int m_argc;
	char_t** m_argv;

	MODE_VEC m_modeVec;
	CMD_MAP m_cmdMap;

	char_t m_modeTag;
	char_t m_cmdTag;

	bool m_caseSensitive;
};

template <typename char_t>
CmdHelper<char_t>::CmdHelper()
	: m_argc(1)
	, m_argv(nullptr)
	, m_modeTag('/')
	, m_cmdTag('-')
	, m_caseSensitive(false)
{
}

template <typename char_t>
CmdHelper<char_t>::~CmdHelper()
{
	m_argv = nullptr;
}

template <typename char_t>
int CmdHelper<char_t>::initialize(int argc, char_t* argv[])
{
	int ret = 0;

	m_argc = argc;
	m_argv = argv;

	m_modeVec.clear();
	m_cmdMap.clear();

	for (int i = 1; i < argc; i++)
	{
		if (isModeKey(argv[i]))
		{
			m_modeVec.push_back(argv[i]);
		}
		else if (isCmdKey(argv[i]))
		{
			if ((i + 1 < argc) && !isCmdKey(argv[i + 1]))
			{
				m_cmdMap.insert(CMD_PAIR(argv[i], argv[i + 1]));
				i++;
			}
			else
			{
				std::basic_string<char_t> strEmpty;
				m_cmdMap.insert(CMD_PAIR(argv[i], strEmpty));
			}
				
		}
	}

	return ret;
}

template <typename char_t>
int CmdHelper<char_t>::parameterCount()
{
	return m_argc;
}

template <typename char_t>
bool CmdHelper<char_t>::hasParameter()
{
	return parameterCount() > 1;
}

template <typename char_t>
void CmdHelper<char_t>::setModeTag(char_t tag)
{
	m_modeTag = tag;
}

template <typename char_t>
void CmdHelper<char_t>::setCmdTag(char_t tag)
{
	m_cmdTag = tag;
}

template <typename char_t>
void CmdHelper<char_t>::setCaseSensitive(bool sensitive)
{
	m_caseSensitive = sensitive;
}

template <typename char_t>
bool CmdHelper<char_t>::hasMode(const char_t* szMode)
{
	typename MODE_VEC::iterator iter = getMode(szMode);

	return iter != m_modeVec.end();
}

template <typename char_t>
bool CmdHelper<char_t>::hasCmd(const char_t* szCmd)
{
	typename CMD_MAP::iterator iter = getCmd(szCmd);

	return iter != m_cmdMap.end();
}

template <typename char_t>
int CmdHelper<char_t>::getCmdValue(const char_t* szCmd, std::basic_string<char_t>& tcsValue)
{
	typename CMD_MAP::iterator iter = getCmd(szCmd);

	if (iter == m_cmdMap.end())
		return -1;

	tcsValue = iter->second;

	return 0;
}

template <typename char_t>
bool CmdHelper<char_t>::isModeKey(const char_t* szMode)
{
	std::basic_string<char_t> tcsMode = szMode;

	if (tcsMode.empty())
		return false;
	else if (tcsMode.size() == 1)
		return false;
	else
		return tcsMode[0] == m_modeTag;
}

template <typename char_t>
bool CmdHelper<char_t>::isCmdKey(const char_t* szCmd)
{
	std::basic_string<char_t> tcsCmd = szCmd;

	if (tcsCmd.empty())
		return false;
	else if (tcsCmd.size() == 1)
		return false;
	else
		return tcsCmd[0] == m_cmdTag;
}

template <typename char_t>
typename CmdHelper<char_t>::MODE_VEC::iterator CmdHelper<char_t>::getMode(const char_t* szMode)
{
	typename CmdHelper<char_t>::MODE_VEC::iterator iter = m_modeVec.begin();

	for (; iter != m_modeVec.end(); iter++)
	{
		std::basic_string<char_t> tcsMode(*iter);
		std::basic_string<char_t> tcsTargetMode(szMode);

		if (!m_caseSensitive)
		{
			transform(tcsMode.begin(), tcsMode.end(), tcsMode.begin(), ::tolower);
			transform(tcsTargetMode.begin(), tcsTargetMode.end(), tcsTargetMode.begin(), ::tolower);
		}

		if (tcsMode.compare(tcsTargetMode) == 0)
			break;
	}

	return iter;
}

template <typename char_t>
typename CmdHelper<char_t>::CMD_MAP::iterator CmdHelper<char_t>::getCmd(const char_t* szCmd)
{
	typename CmdHelper<char_t>::CMD_MAP::iterator iter = m_cmdMap.begin();

	for (; iter != m_cmdMap.end(); iter++)
	{
		std::basic_string<char_t> tcsCmd(iter->first);
		std::basic_string<char_t> tcsTargetCmd(szCmd);

		if (!m_caseSensitive)
		{
			transform(tcsCmd.begin(), tcsCmd.end(), tcsCmd.begin(), ::tolower);
			transform(tcsTargetCmd.begin(), tcsTargetCmd.end(), tcsTargetCmd.begin(), ::tolower);
		}

		if (tcsCmd.compare(tcsTargetCmd) == 0)
			break;
	}

	return iter;
}
