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

#include "LogHelper.h"

#include <cstdarg>
#include <string>


LogHelper::LogHelper()
{
}

LogHelper::~LogHelper()
{
}

#ifdef _WIN32
void LogHelper::OutputFormatDebugString(const char* format, ...)
{
	va_list _ArgList;
	va_start(_ArgList, format);

	char szContent[1024] = { 0 };
	vsprintf_s(szContent, format, _ArgList);

	OutputDebugStringA(szContent);

	va_end(_ArgList);
}

void LogHelper::OutputFormatDebugString(const wchar_t* format, ...)
{
	va_list _ArgList;
	va_start(_ArgList, format);

	wchar_t szContent[1024] = { 0 };
	vswprintf_s(szContent, format, _ArgList);

	OutputDebugStringW(szContent);

	va_end(_ArgList);
}

void LogHelper::printfColorText(WORD wAttributes, const char* format, va_list argList)
{
	CONSOLE_SCREEN_BUFFER_INFO CsInfo;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsole, &CsInfo);
	SetConsoleTextAttribute(hConsole, wAttributes);

	//va_list _ArgList;
	//va_start(_ArgList, format);
	_vfprintf_l(stdout, format, NULL, argList);
	//va_end(_ArgList);

	SetConsoleTextAttribute(hConsole, CsInfo.wAttributes);
}

void LogHelper::printfColorText(WORD wAttributes, const wchar_t* format, va_list argList)
{
	CONSOLE_SCREEN_BUFFER_INFO CsInfo;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(hConsole, &CsInfo);
	SetConsoleTextAttribute(hConsole, wAttributes);

	//va_list _ArgList;
	//va_start(_ArgList, format);
	_vfwprintf_l(stdout, format, NULL, argList);
	//va_end(_ArgList);

	SetConsoleTextAttribute(hConsole, CsInfo.wAttributes);
}
#endif // _WIN32

void LogHelper::printfError(const char* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_RED, format, argList);
#else

	printf("\033[31m");
	printf("\033[49m");

	vfprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}

void LogHelper::printfError(const wchar_t* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_RED, format, argList);
#else

	printf("\033[31m");
	printf("\033[49m");

	vfwprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}

void LogHelper::printfSuccess(const char* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_GREEN, format, argList);
#else

	printf("\033[32m");
	printf("\033[49m");

	vfprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}

void LogHelper::printfSuccess(const wchar_t* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_GREEN, format, argList);
#else

	printf("\033[32m");
	printf("\033[49m");

	vfwprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}

void LogHelper::printfWarning(const char* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_RED | FOREGROUND_GREEN, format, argList);
#else

	printf("\033[33m");
	printf("\033[49m");

	vfprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}

void LogHelper::printfWarning(const wchar_t* format, ...)
{
	va_list argList;
	va_start(argList, format);

#ifdef _WIN32
	printfColorText(FOREGROUND_RED | FOREGROUND_GREEN, format, argList);
#else

	printf("\033[33m");
	printf("\033[49m");

	vfwprintf(stdout, format, argList);

	printf("\033[39m");
	printf("\033[49m");
#endif // _WIN32

	va_end(argList);
}
