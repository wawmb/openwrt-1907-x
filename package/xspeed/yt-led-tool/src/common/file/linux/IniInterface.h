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

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct _option
{
    char* key;
    char* value;
    struct _option* next;
} Option;

typedef struct _data
{
    char* section;
    Option* option;
    struct _data* next;
} Data;

typedef struct
{
    char comment;
    char separator;
    char* re_string;
    int re_int;
    bool re_bool;
    double re_double;
    Data* data;
} Config;


Config* cnf_read_config(const char* filename, char comment, char separator);

bool cnf_write_file(Config* cnf, const char* filename, const char* header);
bool cnf_add_option(Config* cnf, const char* section, const char* key, const char* value);
bool cnf_get_value(Config* cnf, const char* section, const char* key);
