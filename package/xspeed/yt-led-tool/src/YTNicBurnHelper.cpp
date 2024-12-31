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

// YTNicBurnHelper.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <cstdio>

#include "application/CmdHelper.h"
#include "debug/LogHelper.h"

#include "YTCommandHandler.h"


void displayLogo()
{
    printf("********************************************************************************\n");
    printf("*                                                                              *\n");
#ifdef _WIN32
    printf("*           Motorcomm NIC Burn Helper (Windows) v1.0.0.7 (%s)         *\n", __DATE__);
#else
    printf("*            Motorcomm NIC Burn Helper (Linux) v1.0.0.8 (%s)          *\n", __DATE__);
#endif // _WIN32
    printf("*                                                                              *\n");
    printf("********************************************************************************\n");
    printf("\n");
}


#ifdef _WIN32
int _tmain(int argc, TCHAR* argv[])
#else
int main(int argc, TCHAR* argv[])
#endif // _WIN32
{
    int ret = 0;
    
    displayLogo();

    CmdHelper<TCHAR> cmdHelper;

    if ((ret = cmdHelper.initialize(argc, argv)) != 0)
    {
        printf("Error, initialize command failed=%d", ret);
        return ret;
    }

    YTCommandHandler ytCommandHandler;
    
    if ((ret = ytCommandHandler.initialize(cmdHelper)) != 0)
    {
        //LogHelper::printfError("Error, Initialize command handler failed=%d!\n", ret);
        goto END;
    }

    if ((ret = ytCommandHandler.handleCommand()) != 0)
    {
        //LogHelper::printfError("Error, handle command failed=%d!\n", ret);
        goto END;
    }

END:
    ytCommandHandler.release();

    return ret;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
