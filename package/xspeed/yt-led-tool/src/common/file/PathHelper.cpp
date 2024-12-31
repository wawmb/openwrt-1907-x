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

#include "PathHelper.h"

#include <assert.h>

#ifdef _WIN32

#include <Shlwapi.h>
#include <Shlobj.h>
#pragma comment (lib, "Shlwapi.lib")
#else
#include <unistd.h>
#include <fstream>
#endif


namespace PathHelper {

	bool IsPathExistsA(const char* szFilePath)
	{
#ifdef _WIN32
		return PathFileExistsA(szFilePath);
#else
		return access(szFilePath, F_OK) == 0;
#endif
	}

	bool IsPathExistsW(const wchar_t* szFilePath)
	{
#ifdef _WIN32
		return PathFileExistsW(szFilePath);
#else
		// TO DO
		assert(0);
		return false;
#endif
	}

	std::string GetExeDirA()
	{
#ifdef _WIN32
		return ExtractFileDir(GetExePathA());
#else
		std::string strPath = get_current_dir_name();
		strPath.append("/");
		return strPath;
#endif
	}

	std::wstring GetExeDirW()
	{
#ifdef _WIN32
		return ExtractFileDir(GetExePathW());
#else
		// TO DO
		assert(0);
		return L"";
#endif
	}

	int copyFileA(const char* szSrcPath, const char* szDestPath)
	{
#ifdef _WIN32
		if (::CopyFileA(szSrcPath, szDestPath, FALSE))
			return 0;
		else
			return -1;

#else
		std::ifstream srcFstream(szSrcPath, std::ios::binary);
		if (!srcFstream.is_open())
			return -1;

		std::ofstream  dstFstream(szDestPath, std::ios::binary);
		if (!dstFstream.is_open())
		{
			srcFstream.close();
			return -2;
		}

		dstFstream << srcFstream.rdbuf();
		srcFstream.close();
		dstFstream.close();

		return 0;
#endif // _WIN32
	}

	int copyFileW(const wchar_t* szSrcPath, const wchar_t* szDestPath)
	{
#ifdef _WIN32
		if (::CopyFileW(szSrcPath, szDestPath, FALSE))
			return 0;
		else
			return -1;

#else
		assert(0);
		return 1;
#endif // _WIN32
	}

#ifdef _WIN32
	std::string GetSystemDirA()
	{
		std::string dir = "";
		char buf[MAX_PATH] = { 0 };
		if (SHGetSpecialFolderPathA(NULL, buf, CSIDL_SYSTEM, TRUE))
			dir = buf;

		return dir;
	}

	std::wstring GetSystemDirW()
	{
		std::wstring dir = L"";
		wchar_t buf[MAX_PATH] = { 0 };
		if (SHGetSpecialFolderPathW(NULL, buf, CSIDL_SYSTEM, TRUE))
			dir = buf;

		return dir;
	}

	int GetWindowsLetterA(char& letter)
	{
		int ret = 1;

		std::string strTempSystemDir = GetSystemDirA();

		char tempLetter = 'C';

		while (tempLetter != 'Z')
		{
			strTempSystemDir.replace(0, 1, 1, tempLetter);

			if (PathFileExistsA(strTempSystemDir.c_str()))
			{
				std::string strWinPEPath = strTempSystemDir + "\\wpeutil.dll";

				if (!PathFileExistsA(strWinPEPath.c_str()))
				{
					letter = tempLetter;
					ret = 0;
					break;
				}

			}

			tempLetter++;
		}

		return ret;
	}

	int GetWindowsLetterW(wchar_t& letter)
	{
		int ret = 1;

		std::wstring wcsTempSystemDir = GetSystemDirW();

		wchar_t tempLetter = L'C';

		while (tempLetter != L'Z')
		{
			wcsTempSystemDir.replace(0, 1, 1, tempLetter);

			if (PathFileExistsW(wcsTempSystemDir.c_str()))
			{
				std::wstring wcsWinPEPath = wcsTempSystemDir + L"\\wpeutil.dll";

				if (!PathFileExistsW(wcsWinPEPath.c_str()))
				{
					letter = tempLetter;
					ret = 0;
					break;
				}

			}

			tempLetter++;
		}

		return ret;
	}

	std::string GetProgramDataDirA()
	{
		std::string dir = "";
		char buf[MAX_PATH] = { 0 };
		if (SHGetSpecialFolderPathA(NULL, buf, CSIDL_COMMON_APPDATA, TRUE))
			dir = buf;

		return dir;
	}

	std::wstring GetProgramDataDirW()
	{
		std::wstring dir = L"";
		wchar_t buf[MAX_PATH] = { 0 };
		if (SHGetSpecialFolderPathW(NULL, buf, CSIDL_COMMON_APPDATA, TRUE))
			dir = buf;

		return dir;
	}

	std::string GetWindowsProgramDataDirA()
	{
		std::string strWindowsProgramDataDir = "";

		char windowsLetter = 0;
		if (GetWindowsLetterA(windowsLetter) == 0)
		{
			std::string strProgramDataDir = GetProgramDataDirA();

			strProgramDataDir.replace(0, 1, 1, windowsLetter);

			strWindowsProgramDataDir = strProgramDataDir;
		}

		return strWindowsProgramDataDir;
	}

	std::wstring GetWindowsProgramDataDirW()
	{
		std::wstring wcsWindowsProgramDataDir = L"";

		wchar_t windowsLetter = 0;
		if (GetWindowsLetterW(windowsLetter) == 0)
		{
			std::wstring wcsProgramDataDir = GetProgramDataDirW();

			wcsProgramDataDir.replace(0, 1, 1, windowsLetter);

			wcsWindowsProgramDataDir = wcsProgramDataDir;
		}

		return wcsWindowsProgramDataDir;
	}

	std::string GetExePathA()
	{
		std::string strPath = "";
		char buf[MAX_PATH] = { 0 };
		if (GetModuleFileNameA(NULL, buf, MAX_PATH))
			strPath = buf;

		return strPath;
	}

	std::wstring GetExePathW()
	{
		std::wstring strPath = L"";
		wchar_t buf[MAX_PATH] = { 0 };
		if (GetModuleFileNameW(NULL, buf, MAX_PATH))
			strPath = buf;

		return strPath;
	}

	std::string GetExeNameA()
	{
		std::string strExeName = "";

		std::string strPath = GetExePathA();

		strExeName = ExtractFileName(strPath);

		return strExeName;
	}

	std::wstring GetExeNameW()
	{
		std::wstring strExeName = L"";

		std::wstring strPath = GetExePathW();

		strExeName = ExtractFileName(strPath);

		return strExeName;
	}



#endif
}