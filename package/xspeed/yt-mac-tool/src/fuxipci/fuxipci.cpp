/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci.cpp

Abstract:

    FUXI Linux tool

Environment:

    User mode.

Revision History:
    2022-03-09    xiaogang.fan@motor-comm.com    created

--*/
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>

#include "fuxipci_comm.h"
#include "fuxipci_device.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_err.h"

using namespace std;

#define CMD_PROMPT "motorcomm > "

void * gFuxiMemBase = NULL;
u32 gTarget = 0;
u32 gFuxiPciBus = 0;
u32 gFuxiPciDev = 0;
u32 gFuxiPciFunc = 0;

string gVersion = "Version 1.1";

void print_help()
{
    cout << "Input help to show this help:" << endl;
    cout << "All regs and values input are recognized as hex whether with 0X" << endl;

    /* gmac register related commands */
    cout << "r <reg>                         -- Read gmac registers" << endl;
    cout << "w <reg> <value>                 -- Write gmac registers" << endl;

    /* config register related commands */
    cout << "cr <reg>                        -- Read CFG registers, the reg's unit is 4 bytes" << endl;
    cout << "cw <reg> <value>                -- Write CFG registers, the reg's unit is 4 bytes" << endl;

    /* efuse related commands */
    cout << "mac r	                        -- Read MAC address from eFuse " << endl;
    cout << "mac w <mac_addr>                -- Write MAC address into eFuse" << endl;
    cout << "led r	                        -- Read LED solution from eFuse " << endl;
    cout << "led w <solution_id>             -- Write LED solution into eFuse" << endl;
    cout << "oob r	                        -- Read OOB from eFuse " << endl;
    cout << "oob w                           -- Write OOB into eFuse" << endl;
    cout << "epci info                       -- Read PCIe subsysID and subvendor from eFuse" << endl;
    cout << "epci set <subsys> <subven>      -- Write PCIe subsysID and subvendorID into eFuse" << endl;
    //cout << "el                              -- Efuse Load. Trigger HW autoload which will load regionB only" << endl;
    cout << "ev                              -- Efuse View, read all eFuse contents" << endl;
    cout << "err <reg>                       -- Read eFuse patch for speficied register" << endl;
    cout << "ewr <reg> <value>               -- Write eFuse patch for speficied register" << endl;
    //cout << "ewi <index> <reg> <value>       -- Write Efuse patch, to 0-based entry index" << endl;

    /* lb test related commands */
    //cout << "lbp                             -- Loopback test by phy: 200 packets, 1000/100/10M, ip/udp" << endl;
    cout << "lbc                             -- Loopback test by cable: 200 packets, 1000/100/10M, ip/udp" << endl;
    //cout << "lbp it                          -- Loopback test by phy, test packet include ip/tcp/offload checkum" << endl;
    //cout << "lbp iu                          -- Loopback test by phy, test packet include ip/udp/offload checksum" << endl;
    //cout << "lbp lso                         -- Loopback test by phy, test packet include tcp_large_send v2" << endl;

    /* quit command */
    cout << "quit                            -- Quit app" << endl;
}

int main(int argc, char **argv) 
{
    int fd;
    void *map_base; 
    off_t target;
    FILE *cfg_fd;
    char cfg_buf[4][18] = {"0xB1100000","00","00","0"}; //defalt pci mem addr
    char cfg_file_name[] = "./devmem_config.ini";
    char cfg_path[256] = {0};
    char *fname = cfg_file_name;
    char* last_slash;
    size_t result_length;
    int cfg_len;
    int i = 0;

    /* read config file: devmem_config.ini */
    last_slash = strrchr (argv[0], '/'); //get executable full path
    result_length = last_slash - argv[0];
    strncpy (cfg_path, argv[0], result_length);

    if(((strlen(cfg_path) + strlen(cfg_file_name)) < 256)){
        strcat(cfg_path, &cfg_file_name[1]);
        fname = cfg_path;
    }

    cfg_fd = fopen(fname, "r");
    if(!cfg_fd){
        cout << "file ./devmem_config.ini does not exist, use default PCI memory address, " << (char *)&cfg_buf[0] << endl;
        return -1;
    }else{
        while(!feof(cfg_fd)){
            if(i > 4){
                cout <<  "file ./devmem_config.ini has more data, please check it." << endl;
                fclose(cfg_fd);
                return -1;
            }
	        fscanf(cfg_fd, "%s", cfg_buf[i]);
	        i++;
	    }
        fclose(cfg_fd);
    }

    /* save info */
    target = strtoul(cfg_buf[0], 0, 0);
    gFuxiPciBus = strtoul(cfg_buf[1], 0, 0);
    gFuxiPciDev = strtoul(cfg_buf[2], 0, 0);
    gFuxiPciFunc = strtoul(cfg_buf[3], 0, 0);
    
    /* map registers to memory */
    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1) {
        cout << "err: PCI space " << target <<", memory mapped at address " << map_base <<", errno=" << errno << ", " << strerror(errno) << endl;
        close(fd);
        FATAL;
    }

    gFuxiMemBase = map_base;
    gTarget = target;

    /* print help information */
    cout << "                   Motorcomm PCIe Utils " << gVersion << endl;
    print_help();
    cout << endl;
    
    if (argc == 1) {
        while (true) 
        {
            static string args;
            static unsigned int nRepeat = 0, nFile = 0;
            static ifstream fin;

            if(!nRepeat) {
                cout << CMD_PROMPT;

                if(fin.is_open()){
                    nFile = 0;
                    if(!fin.eof()){
                        do{
                            getline(fin, args);
                        }while((!fin.eof()) && args.empty());
                        cout << args << endl;
                        nFile = 1;
                    }else{
                        fin.close();
                        cout << "done." << endl;
                        cout << CMD_PROMPT;
                    }
                }

                if(!nFile){
                    getline(cin, args, '\n');
                    //cout << "cmd as chars:" << args.c_str() << endl;
                }

                stringstream ssTemp(args);
                vector<string> args_vec_temp;
                string argTemp;
                while (ssTemp >> argTemp) {
                    args_vec_temp.push_back(argTemp);
                }

                if (args_vec_temp.size() == 0){
                    print_help();
                    continue;
                }else{
                    string cmdTemp = args_vec_temp[0];
                    if (cmdTemp== "rep") // repeat test command.
                    {
                        if (args_vec_temp.size() >= 3)
                        {
                            stringstream sstr;
                            unsigned int pos;

                            sstr << dec << args_vec_temp[1];
                            sstr >> nRepeat;
                            sstr.clear();

                            //cout << "detect repeated cmd: " << cmdTemp << " " << nRepeat << endl;
                            pos = (unsigned int)args.find(args_vec_temp[1]);
                            if(0 < pos)
                            {
                                //cout << "arg1 is from " << pos << " and its len= " <<args_vec_temp[1].length() << endl;
                                args.erase(0, pos + args_vec_temp[1].length());
                                cout << "Cmd \'" << args.c_str() << "\' will be repeated for " << std::dec << nRepeat-- << " times, press any key to stop..." << endl;
                            }else {
                                cout << "argument error." << endl;
                                nRepeat = 0;
                                continue;
                            }
                        }
                    }else if (cmdTemp== "rem"){ //for comments line in a cmd file
                        continue;
                    }else if (cmdTemp== "file"){ // read cmd from a file.
                        if (args_vec_temp.size() >= 2){
                            if(fin.is_open()){
                                cout << "pls wait for completion of previous file cmd." << endl;
                                continue;
                            }

                            fin.open(args_vec_temp[1]);
                            if(!fin.is_open()) {
                                cout << "cannot open file " << args_vec_temp[1] << endl;
                                //cout << args_vec_temp[0] << " " << args_vec_temp[1] << "failed" << endl;
                            }
                        }else{
                            char buffTmp[250];
                            getcwd(buffTmp, 250); 

                            cout << buffTmp << endl;
                        }

                        continue;
                    }
                    //continue;
                }
            } else{
                nRepeat--;
                sleep(1);
                //if(_Exit){
                //    nRepeat = 0;
                //    continue;
                //}
            }

            stringstream ss(args);
            vector<string> args_vec;
            string arg;
            while (ss >> arg) {
                args_vec.push_back(arg);
            }

            if (args_vec.size() == 0)
            {
                continue;
            }
            else
            {
                string cmd = args_vec[0];
                if (cmd == "mac") // MAC address in efuse.
                {
                    if (args_vec.size() == 2)
                    {
                        string subcmd = args_vec[1];

                        if (subcmd == "r")
                        {
                            read_mac_addr_from_efuse(map_base, target);
                        }
                        else
                        {
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else if (args_vec.size() == 3)
                    {
                        string              subcmd = args_vec[1];
                        string              macaddr_str = args_vec[2];
                        uint_least32_t      addr[6];
                        uint8_t             macaddr[6];
                        uint8_t             num = sizeof(macaddr) / sizeof(macaddr[0]);
                        if (subcmd == "w")
                        {
                            if (num != sscanf(macaddr_str.c_str(), "%x:%x:%x:%x:%x:%x", &addr[5], &addr[4], &addr[3], &addr[2], &addr[1], &addr[0]))
                            {
                                cout << "Bad format for mac address." << endl;
                                continue;
                            }
                            for (int i = 0; i < num; i++)
                            {
                                macaddr[i] = addr[i];
                            }
                            write_mac_addr_to_efuse(map_base, target, macaddr);
                        }
                        else
                        {
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else
                    {
                        cout << "please input subcmd" << endl;
                    }
                }
                else if (cmd == "led") // LED in efuse.
                {
                    uint32_t ledId = 0;

                    if (args_vec.size() == 2)
                    {
                        string subcmd = args_vec[1];

                        if (subcmd == "r")
                        {
                            if(true != efuse_read_regionA_regionB(map_base, target, 0x00, &ledId)){
                                printf("Read Led Id error.\n");
                            }
                            else
                            {
                                printf("Read Led Id: %02X\n", ledId);
                            }
                        }
                        else
                        {
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else if (args_vec.size() == 3)
                    {
                        string              subcmd = args_vec[1];
                        string              str_ledId = args_vec[2];
                        uint32_t            oldLedId = 0;
                        bool                ret = false;

                        if (subcmd == "w")
                        {
                            if(!sscanf(str_ledId.c_str(), "%x", &ledId)){
                                cout << "Invalid parameters. ledId is "<< hex << ledId << endl;
                                continue;
                            }

                            if(true != efuse_read_regionA_regionB(map_base, target, 0x00, &oldLedId)){
                                printf("Read Led Id error.\n");
                            }
                            else
                            {
                                if(oldLedId != 0)
                                {
                                    printf("Led Id has written %X, cannot write it again.\n", oldLedId);
                                }
                                else
                                {
                                    ret = efuse_write_led(map_base, target, ledId);
                                    if(true != ret){
                                        printf("Write Led Id error.\n");
                                    }
                                    else
                                    {
                                        printf("Write Led Id successful.\n");
                                    }
                                }
                            }       
                        }
                        else
                        {
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else
                    {
                        cout << "please input subcmd" << endl;
                    }
                }
                else if (cmd == "oob") // LED in efuse.
                {
                    if (args_vec.size() == 2)
                    {
                        string subcmd = args_vec[1];
                        uint32_t reg_val = 0;

                        if (subcmd == "r")
                        {
                            if (!efuse_read_regionA_regionB(map_base, target, 0x07, &reg_val)) {
                                printf("Read OOB error.\n");
                            }
                            else
                            {
                                if (XLGMAC_GET_REG_BITS(reg_val, 2, 1)) {
                                    printf("OOB Ctrl bit already exists.\n");
                                }
                                else
                                {
                                    printf("OOB Ctrl bit does not exist.\n");
                                }
                            }
                        }
                        else if(subcmd == "w")
                        {
                            if (!efuse_read_regionA_regionB(map_base, target, 0x07, &reg_val)) {
                                printf("Read OOB error.\n");
                            }
                            else
                            {
                                if (XLGMAC_GET_REG_BITS(reg_val, 2, 1)) {
                                    printf("OOB Ctrl bit already exists.\n");
                                }
                                else
                                {
                                    if(!efuse_write_oob(map_base, target))
                                    {
                                        printf("Write OOB error.\n");
                                    }
                                    else
                                    {
                                        printf("Write OOB successful.\n");
                                    }
                                }
                            }       
                        }
                        else
                        {
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else
                    {
                        cout << "please input subcmd" << endl;
                    }
                }
                else if (cmd == "lbp") // Loopback test.
                {
                    if (args_vec.size() == 1)
                    {
                        printf("phy loopback test start ...... \n");
                        if(loopback_test(CMD_CHK_PHY_LP) != STATUS_SUCCESSFUL){
                            cout << "phy loopback test fail !" << endl;
                    }
                        else{
                            cout << "phy loopback test pass !" << endl;
                        }
                    }
                    else if (args_vec.size() == 2)
                    {
                        string subcmd = args_vec[1];
                        u32 type = 0;

                        if (subcmd == "it" || subcmd == "iu")
                        {
                            type |= CMD_CHK_XSUM_IPV4;
                            type |= CMD_CHK_XSUM_L4;

                            if(subcmd == "it"){
                                type |= CMD_CHK_TCP;
                                printf("tcp checksum offload test start ...... \n");
                            }else{
                                printf("ucp checksum offload test start ...... \n");
                            }

                            if(loopback_test(type) != STATUS_SUCCESSFUL){
                                cout << "checksum offload test fail !" << endl;
                            }
                            else{
                                cout << "checksum offload test pass !" << endl;
                            }
                        }
                        else if (subcmd == "lso")
                        {
                            type |= CMD_CHK_LSO_V2;
                            if(loopback_test(type) != STATUS_SUCCESSFUL){
                                cout << "large size offload test fail !" << endl;
                            }
                            else{
                                cout << "large size offload test pass !" << endl;
                            }
                        }else{
                            cout << "subcmd is wrong, please input check it." << endl;
                            continue;
                        }

                    }
                    else
                    {
                        cout << "lbp has parameters more than 2, please check it." << endl;
                    }
                }
                else if (cmd == "lbc") // Loopback test.
                {
                    if (args_vec.size() == 1)
                    {
                        printf("cable loopback test start ...... \n");
                        if(loopback_test(CMD_CHK_CABLE_LP) != STATUS_SUCCESSFUL){
                            cout << "cable loopback test fail !" << endl;
                        }
                        else{
                            cout << "cable loopback test pass !" << endl;
                        }
                    }
                    else
                    {
                        cout << "lbc has parameters more than 2, please check it." << endl;
                    }
                }
                else if (cmd == "epci")
                {
                    if (args_vec.size() >= 2)
                    {
                        string subcmd = args_vec[1];

                        if (subcmd == "info")
                        {
                            read_subsys_from_efuse(map_base, target);
                        }
                        else if (subcmd == "set")
                        {
                            //cout << "subcmd is set."<< endl;
                            if(args_vec.size() == 4){
                                string  str_subsys = args_vec[2];
                                string  str_subvendor = args_vec[3];
                                uint32_t    subsys,subvendor;

                                if(!sscanf(str_subsys.c_str(), "%x", &subsys) || !sscanf(str_subvendor.c_str(), "%x", &subvendor)){
                                    cout << "Invalid parameters. subsys is "<< hex << subsys << ", subverdor is " << hex << subvendor << endl;
                                    continue;
                                }

                                write_subsys_info_to_efuse(map_base, target, subsys, subvendor);
                            }else{
                                cout << "pci set command has wong patameters, it has " << args_vec.size() << " parameters." << endl;
                            }
                        }else{
                            cout << "unknown subcmd: " << subcmd << endl;
                        }
                    }
                    else
                    {
                        cout << "please input subcmd" << endl;
                    }
                }
                else if (cmd == "el") // efuse load.
                {
                    efuse_load(map_base, target);
                }
                else if (cmd == "ev") // Efuse View. Read all efuse content in regionC, parse them.
                {
                    uint32_t        reg = 0;
                    uint32_t        value = 0;
                    uint32_t        index = 0;

                    /* print region A and region B */
                    printf("Efuse Region A and Region B:\n");
                    for (index = 0; index < EFUSE_REGION_A_B_LENGTH; index++)
                    {
                        if (!efuse_read_regionA_regionB(map_base, target, index, &value)) 
                        {
                            printf("read efuse region a_b error.\n");
                            break;
                        }
                        else
                        {
                            printf("Byte %2d-0x%2X\t", index, value);
                            if((index + 1) % 5 == 0)
                                printf("\n");
                        }
                    }
                    printf("\n");

                    printf("Efuse Region C:\n");
                    for ( index = 0; index < 39; index++)
                    {
                        read_patch_from_efuse_per_index(map_base, target, index, &reg, &value);
                        printf("efuse: [%02d] reg0x%04x - 0x%08x\n", index, reg, value);
                    }
                }
                else if (cmd == "ewi")  // ewi <index> <reg> <value>. Write efuse patch, to 0-based entry index.
                {
                    if (args_vec.size() >= 4)
                    {
                        stringstream    sstr;
                        uint32_t        index;
                        uint32_t        reg;
                        uint32_t        value;
                        uint32_t    cur_reg = 0;
                        uint32_t    cur_val = 0;

                        sstr << hex << args_vec[1];
                        sstr >> index;
                        sstr.clear();
                        sstr << hex << args_vec[2];
                        sstr >> reg;
                        sstr.clear();
                        sstr << hex << args_vec[3];
                        sstr >> value;

                        if (index > 38) {
                            printf("index 0x%x out of range, must be less than 0x27.\n", index);
                            continue;
                        }
                        read_patch_from_efuse_per_index(map_base, target, index, &cur_reg, &cur_val);

                        if(cur_reg == 0 && cur_val == 0){
                            write_patch_to_efuse_per_index(map_base, target, index, reg, value);
                            if(!(reg >> 16))
                                printf("Reg %x entry current value %x.\n", reg, value);
                        }else{
                            printf("Index %d has saved reg %x value %x, cannot rewirte.\n", index, cur_reg, cur_val);
                        }
                    }
                    else
                    {
                        cout << "ewi <index> <reg> <value>" << endl;
                    }
                }
                else if (cmd == "err") // err <reg>. Read efuse patch for specified register.
                {
                    if (args_vec.size() >= 2)
                    {
                        stringstream    sstr;
                        uint32_t        reg;

                        sstr << hex << args_vec[1];
                        sstr >> reg;
                        read_patch_from_efuse(map_base, target, reg);
                    }
                    else
                    {
                        cout << "err <reg>" << endl;
                    }            
                }
                else if (cmd == "ewr")  // ewr <reg> <value>. Write efuse patch for specified register.
                {
                    if (args_vec.size() >= 3)
                    {
                        stringstream    sstr;
                        uint32_t        reg;
                        uint32_t        value;

                        sstr << hex << args_vec[1];
                        sstr >> reg;
                        sstr.clear();
                        sstr << hex << args_vec[2];
                        sstr >> value;

                        write_patch_to_efuse(map_base, target, reg, value);
                    }
                    else
                    {
                        cout << "ewr <reg> <value>" << endl;
                    }
                }
                else if (cmd == "r") // read GMAC registers.
                {
                    if (args_vec.size() >= 2)
                    {
                        stringstream sstr;
                        unsigned int reg;
                        unsigned int value;
                        
                        sstr << hex << args_vec[1];
                        sstr >> reg;

                        read_gmac_register(map_base, target, reg);
                    }
                    else
                    {
                        cout <<"r <reg>\n"<< endl;
                    }
                }
                else if (cmd == "w")  // write GMAC registers.
                {
                    if (args_vec.size() >= 3)
                    {
                        stringstream sstr;
                        unsigned int reg;
                        unsigned int value;

                        sstr << hex << args_vec[1];
                        sstr >> reg;

                        sstr.clear();
                        sstr << hex << args_vec[2];
                        sstr >> value;
                        
                        write_gmac_register(map_base, target, reg, value);
                    }
                    else
                    {
                        cout << "w <reg> <value>\n" << endl;
                    }
                }
                else if (cmd == "cr") // read CFG registers.
                {
                    if (args_vec.size() >= 2)
                    {
                        stringstream sstr;
                        unsigned int reg;
                        unsigned int value;

                        sstr << hex << args_vec[1];
                        sstr >> reg;

                        read_gmac_config_register(map_base, target, reg);
                    }
                    else
                    {
                        cout <<"cr <reg>\n" <<endl;
                    }
                }
                else if (cmd == "cw")  // write CFG registers.
                {
                    if (args_vec.size() >= 3)
                    {
                        stringstream sstr;
                        unsigned int reg;
                        unsigned int value;

                        sstr << hex << args_vec[1];
                        sstr >> reg;

                        sstr.clear();
                        sstr << hex << args_vec[2];
                        sstr >> value;

                        write_gmac_config_register(map_base, target, reg, value);
                    }
                    else
                    {
                        cout << "cw <reg> <value>" << endl;
                    }
                }
                else if (cmd == "quit")
                {
                    return 0;
                }
                else if (cmd == "q")
                {
                    return 0;
                }
                else
                {
                    print_help();
                }
            }
            /* here can repeat commands if needed. yzhang comments*/
            /* tbd */
        }
    }else{
        cout << "The PCI tool doesnot need any paramters, please check it." << endl;
        if(munmap(map_base, MAP_SIZE) == -1) FATAL;
        close(fd);
        return -1;
    }

    if(munmap(map_base, MAP_SIZE) == -1) FATAL;
    close(fd);
    return 0;
}

