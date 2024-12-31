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
#include <vector>
#include <algorithm>
#include <iterator>
#include <regex>

#ifdef _WIN32
#include <Shlwapi.h>
#endif // _WIN32


namespace PathHelper {

#define PATH_SLASH					'/'
#define PATH_BACK_SLASH				'\\'


template <class char_t>
std::basic_string<char_t>
ExtractFileName(const std::basic_string<char_t>& filePath)
{
	typename std::basic_string<char_t>::size_type pos = filePath.find_last_of(PATH_BACK_SLASH);

	if (pos != std::basic_string<char_t>::npos && pos + 1 < filePath.size())
		return filePath.substr(pos + 1, filePath.size() - pos - 1);
	else
	{
		pos = filePath.find_last_of(PATH_SLASH);

		if (pos != std::basic_string<char_t>::npos && pos + 1 < filePath.size())
			return filePath.substr(pos + 1, filePath.size() - pos - 1);
		else
			return filePath;
	}
}

template <class char_t>
std::basic_string<char_t>
ExtractFileDir(const std::basic_string<char_t>& filePath)
{
	typename std::basic_string<char_t>::size_type pos = filePath.find_last_of(PATH_BACK_SLASH);

	if (pos != std::basic_string<char_t>::npos)
		return filePath.substr(0, pos + 1);
	else
	{
		pos = filePath.find_last_of(PATH_SLASH);

		if (pos != std::basic_string<char_t>::npos)
			return filePath.substr(0, pos + 1);
		else
			return filePath;
	}
}

template <class char_t>
std::basic_string<char_t>
IncludeSlashForPath(const std::basic_string<char_t>& tcsFilePath)
{
	if (!tcsFilePath.empty())
	{
		if (tcsFilePath[tcsFilePath.length() - 1] != PATH_SLASH)
		{
			std::basic_string<char_t> tcsTemp = tcsFilePath;
			tcsTemp += PATH_SLASH;
			return tcsTemp;
		}
	}

	return tcsFilePath;
}

template <class char_t>
std::basic_string<char_t>
IncludeBackSlashForPath(const std::basic_string<char_t>& tcsFilePath)
{
	if (!tcsFilePath.empty())
	{
		if (tcsFilePath[tcsFilePath.length() - 1] != PATH_BACK_SLASH)
		{
			std::basic_string<char_t> tcsTemp = tcsFilePath;
			tcsTemp += PATH_BACK_SLASH;
			return tcsTemp;
		}
	}

	return tcsFilePath;
}

template <class char_t>
std::basic_string<char_t>
ExcludeBackSlashForPath(const std::basic_string<char_t>& tcsFilePath)
{
	if (!tcsFilePath.empty())
	{
		if (tcsFilePath[tcsFilePath.length() - 1] == PATH_BACK_SLASH)
		{
			std::basic_string<char_t> tcsTemp = tcsFilePath.substr(0, tcsFilePath.length() - 1);
			return tcsTemp;
		}
	}

	return tcsFilePath;
}

template<typename E,
typename TR = std::char_traits<E>,
typename AL = std::allocator<E>,
typename _str_type = std::basic_string<E, TR, AL>>
std::vector<_str_type> Split(const std::basic_string<E, TR, AL>& in, const std::basic_string<E, TR, AL>& delim) 
{
	_str_type strTemp = delim;

	typename _str_type::size_type pos = strTemp.find(PATH_SLASH);

	while (pos != _str_type::npos)
	{	
		strTemp.replace(pos, 1, 2, PATH_SLASH);
		pos = strTemp.find(PATH_SLASH, pos + 2);
	} 

	std::basic_regex<E> re{ strTemp };

	std::vector<_str_type> vecRet = std::vector<_str_type>{ std::regex_token_iterator<typename _str_type::const_iterator>(in.begin(), in.end(), re, -1), std::regex_token_iterator<typename _str_type::const_iterator>() };

	if (!vecRet.empty())
	{
		typename std::vector<_str_type>::iterator iter = vecRet.begin();

		if ((*iter).empty())
			vecRet.erase(iter);
	}
	

	return vecRet;
}


template <class char_t>
std::vector<std::basic_string<char_t>>
GetDirectoryList(const std::basic_string<char_t>& tcDirPath)
{
	std::vector<std::basic_string<char_t>> vecDirectorys;

	if (tcDirPath.empty())
		return vecDirectorys;

	char_t szTag[2] = { PATH_SLASH , '\0' };

	typename std::basic_string<char_t>::size_type pos = tcDirPath.find(szTag);

	if (pos == std::basic_string<char_t>::npos)
	{
		szTag[0] = PATH_BACK_SLASH;
		pos = tcDirPath.find(szTag);
	}
		
	std::basic_string<char_t> tcsDelim = szTag;

	vecDirectorys = Split(tcDirPath, tcsDelim);

	return vecDirectorys;
}

bool IsPathExistsA(const char* szFilePath);
bool IsPathExistsW(const wchar_t* szFilePath);

std::string GetExeDirA();
std::wstring GetExeDirW();

int copyFileA(const char* szSrcPath, const char* szDestPath);
int copyFileW(const wchar_t* szSrcPath, const wchar_t* szDestPath);

#ifdef UNICODE

#define IsPathExists				IsPathExistsW
#define GetExeDir					GetExeDirW
#define copyFile					copyFileW

#else

#define IsPathExists				IsPathExistsA
#define GetExeDir					GetExeDirA
#define copyFile					copyFileA

#endif

#ifdef _WIN32

std::string GetSystemDirA();
std::wstring GetSystemDirW();

int GetWindowsLetterA(char& letter);
int GetWindowsLetterW(wchar_t& letter);

std::string GetProgramDataDirA();
std::wstring GetProgramDataDirW();

std::string GetWindowsProgramDataDirA();
std::wstring GetWindowsProgramDataDirW();

std::string GetExePathA();
std::wstring GetExePathW();



std::string GetExeNameA();
std::wstring GetExeNameW();

#ifdef UNICODE
	#define GetSystemDir  GetSystemDirW
	#define GetWindowsLetter  GetWindowsLetterW
	#define GetProgramDataDir  GetProgramDataDirW
	#define GetWindowsProgramDataDir  GetWindowsProgramDataDirW
	#define GetExePath  GetExePathW
	
	#define GetExeName  GetExeNameW
#else
	#define GetSystemDir  GetSystemDirA
	#define GetWindowsLetter  GetWindowsLetterA
	#define GetProgramDataDir  GetProgramDataDirA
	#define GetWindowsProgramDataDir  GetWindowsProgramDataDirA
	#define GetExePath  GetExePathA
	
	#define GetExeName  GetExeNameA
#endif // !UNICODE

#endif

}