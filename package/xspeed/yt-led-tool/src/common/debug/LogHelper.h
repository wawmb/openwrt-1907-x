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

#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

class LogHelper
{
protected:
	LogHelper();
	virtual ~LogHelper();

public:
#ifdef _WIN32
	static void OutputFormatDebugString(const char* format, ...);
	static void OutputFormatDebugString(const wchar_t* format, ...);


	static void printfColorText(WORD wAttributes, const char* format, va_list argList);
	static void printfColorText(WORD wAttributes, const wchar_t* format, va_list argList);
#endif // _WIN32

	static void printfError(const char* format, ...);
	static void printfError(const wchar_t* format, ...);

	static void printfSuccess(const char* format, ...);
	static void printfSuccess(const wchar_t* format, ...);

	static void printfWarning(const char* format, ...);
	static void printfWarning(const wchar_t* format, ...);

	
};