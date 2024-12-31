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

#include <string.h>
#include <wchar.h>

#ifdef _WIN32

#include <tchar.h>
#include <Windows.h>

#define sleep_s(x)					Sleep((x)*1000)
#define sleep_ms					Sleep
#define CLEAR_SCREEN_CMD			"cls"

	#ifdef _UNICODE

	#define stscanf     swscanf_s

	#else

	#define stscanf     sscanf_s

	#endif // _UNICODE

#else

#include <unistd.h>

#define MAX_PATH					260
#define CLEAR_SCREEN_CMD			"clear"
#define sleep_s						sleep
#define sleep_ms(x)					usleep((x)* 1000)
#define	sprintf_s					sprintf

	#ifdef _UNICODE

	typedef wchar_t TCHAR;
	#define _tcstoul    wcstoul
	#define stscanf     swscanf
	#define _tprintf    wprintf
	#define _tsprintf   wprintf
	#define _stprintf_s wprintf
	#define _tcslen     wcslen
	#define _tstoi      _wtoi
	#define _tcscmp     wcscmp
	#define _tcsicmp    wcscasecmp
	#define _itot       _itoa
	#define _T(x)       L ## x

	#else

	typedef char TCHAR;
	#define _tcstoul    strtoul
	#define stscanf     sscanf
	#define _tprintf    printf
	#define _tsprintf   sprintf
	#define _stprintf_s sprintf
	#define _tcslen     strlen
	#define _tstoi      atoi
	#define _tcscmp     strcmp
	#define _tcsicmp    strcasecmp
	#define _itot       _itow
	#define _T(x)       x

	#endif // _UNICODE

#endif // _WIN32

#ifdef _DEBUG
#define DPRINT(...) printf(__VA_ARGS__)
#else
#define DPRINT(...)
#endif