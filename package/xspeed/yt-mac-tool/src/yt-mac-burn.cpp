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

// for dir operations
#include <dirent.h>

/* below is for pci device finding */
#include <pci/pci.h>

#include "fuxipci_comm.h"
#include "fuxipci_device.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_err.h"

using namespace std;

#define CMD_PROMPT "motorcomm > "

#if !defined(_countof)
#define _countof sizeof
#endif

typedef struct yt_pci_dev_s
{
    char df_path[50]; // debug fs path
    u16 domain;
    u16 ven_id;
    u16 dev_id;
    u16 sub_ven_id;
    u16 sub_dev_id;
    u32 mem;
    u8 bus, dev, func;
    struct pci_dev *pcidev;
} yt_pci_dev_t;

extern bool write_mac_subsys_to_efuse(void *map_base, u32 target, u8 *mac_addr, u32 *subsys, u32 *revid);

void *gFuxiMemBase = NULL;
u32 gTarget = 0;
u32 gFuxiPciDomain;
u32 gFuxiPciBus = 0;
u32 gFuxiPciDev = 0;
u32 gFuxiPciFunc = 0;
u32 gFuxiLpPacketsNum = 200;

string gVersion = "Version 1.1";

/* PCI devide releated */
#define YT6801_MAX_NUM_SUPPORTED 10
yt_pci_dev_t yt6801_devices[YT6801_MAX_NUM_SUPPORTED] = {0};
int num_of_yt6801_dev = 0;
int cur_yt6801_dev = 0;
extern pci_dev_id dev_list[]; // in file: fuxipci_lpbk.cpp

extern "C" int findout_all_yt6801_devices(int verbose);
extern "C" char *get_netdevice_name_by_bdf(int domain, int bus, int dev, int func, char *bdf, char *name_net_dev);

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
    // cout << "el                              -- Efuse Load. Trigger HW autoload which will load regionB only" << endl;
    cout << "ev                              -- Efuse View, read all eFuse contents" << endl;
    cout << "err <reg>                       -- Read eFuse patch for speficied register" << endl;
    cout << "ewr <reg> <value>               -- Write eFuse patch for speficied register" << endl;
    // cout << "ewi <index> <reg> <value>       -- Write Efuse patch, to 0-based entry index" << endl;

    /* lb test related commands */
    // cout << "lbp                             -- Loopback test by phy: 200 packets, 1000/100/10M, ip/udp" << endl;
    cout << "lbc                             -- Loopback test by cable: 200 packets, 1000/100/10M, ip/udp" << endl;
    // cout << "lbp it                          -- Loopback test by phy, test packet include ip/tcp/offload checkum" << endl;
    // cout << "lbp iu                          -- Loopback test by phy, test packet include ip/udp/offload checksum" << endl;
    // cout << "lbp lso                         -- Loopback test by phy, test packet include tcp_large_send v2" << endl;

    /* quit command */
    cout << "quit                            -- Quit app" << endl;
}

int main(int argc, char **argv)
{
    int fd, ret = 0;
    void *map_base;
    off_t target;
    FILE *cfg_fd;
    char cfg_buf[4][18] = {"0xB1100000", "00", "00", "0"}; // defalt pci mem addr
    char cfg_file_name[] = "./devmem_config.ini";
    char cfg_path[256] = {0};
    char *fname = cfg_file_name;
    char *last_slash;
    size_t result_length;
    int cfg_len;
    int i = 0, verbose = 0;
    char name_net_dev[50] = {0};
    char bdf[15] = {0};
    char *netname = NULL; // indicate if net device exists

    /* read config file: devmem_config.ini */
    last_slash = strrchr(argv[0], '/'); // get executable full path
    result_length = last_slash - argv[0];
    strncpy(cfg_path, argv[0], result_length);

    if (((strlen(cfg_path) + strlen(cfg_file_name)) < 256))
    {
        strcat(cfg_path, &cfg_file_name[1]);
        fname = cfg_path;
    }

    if ((argc == 2) && (!strcmp(argv[1], "-v")))
    {
        verbose = 1;
    }

#if 1
    if (!findout_all_yt6801_devices(verbose))
    {
        printf("No YT6801 devices found, pls check privilege or if device is ready?\n");
        return -1;
    }
    else if (verbose)
        return num_of_yt6801_dev;

    if (num_of_yt6801_dev > 1)
    {
        cfg_fd = fopen(fname, "r");
        if (!cfg_fd)
        {
            // cout << "file ./devmem_config.ini does not exist, use default PCI memory address, " << (char *)&cfg_buf[0] << endl;

            cur_yt6801_dev = 0;
            switch (num_of_yt6801_dev)
            {
            case 1:
                break;
            default:
                cout << "There are " << num_of_yt6801_dev << " YT6801 NICs found, but file " << cfg_file_name << " does not exist." << endl;
                printf("By default, the first NIC will be used in below process. BDF=%02x:%02x.%02x.\n", yt6801_devices[cur_yt6801_dev].bus, yt6801_devices[cur_yt6801_dev].dev, yt6801_devices[cur_yt6801_dev].func);
                cout << "To change the NIC worked on, pls specify the NO.(note: number only) as list below in the file: " << cfg_file_name << endl;
                cout << "The command is like this: echo 1 > " << cfg_file_name << " [Enter].\n"
                     << endl;
                for (i = 0; i < num_of_yt6801_dev; i++)
                {
                    netname = get_netdevice_name_by_bdf(yt6801_devices[i].domain, yt6801_devices[i].bus, yt6801_devices[i].dev, yt6801_devices[i].func, bdf, name_net_dev);
                    printf("NO.[%d]\tBDF=%04x:%02x:%02x.%02x, %s.\n", i, yt6801_devices[i].domain, yt6801_devices[i].bus, yt6801_devices[i].dev, yt6801_devices[i].func, netname ? netname : "No net-device");
                }
                break;
            }
        }
        else
        {
            i = 0;
            while (!feof(cfg_fd))
            {
                if (0 && i >= 1)
                {
                    cout << "The first NO. of YT6801 in file " << cfg_file_name << " will be used, " << cfg_buf[0] << "." << endl;
                    // fclose(cfg_fd);
                    break;
                }
                fscanf(cfg_fd, "%s", cfg_buf[0]);
                i++;
            }
            fclose(cfg_fd);
            cur_yt6801_dev = atoi(cfg_buf[0]);
            if (cur_yt6801_dev >= num_of_yt6801_dev)
            {
                printf("bad NO. of Yt6801 NIC configured, %d, default NIC is used.\n", cur_yt6801_dev);
                cur_yt6801_dev = 0;
            }
        }
    }
#else // below is no longer needed
    /* save info */
    target = strtoul(cfg_buf[0], 0, 0);
    gFuxiPciBus = strtoul(cfg_buf[1], 0, 0);
    gFuxiPciDev = strtoul(cfg_buf[2], 0, 0);
    gFuxiPciFunc = strtoul(cfg_buf[3], 0, 0);
#endif

    strcpy(gFuxiDfPath, yt6801_devices[cur_yt6801_dev].df_path);
    target = yt6801_devices[cur_yt6801_dev].mem;
    gFuxiPciDomain = yt6801_devices[cur_yt6801_dev].domain;
    gFuxiPciBus = yt6801_devices[cur_yt6801_dev].bus;
    gFuxiPciDev = yt6801_devices[cur_yt6801_dev].dev;
    gFuxiPciFunc = yt6801_devices[cur_yt6801_dev].func;

    netname = get_netdevice_name_by_bdf(gFuxiPciDomain, gFuxiPciBus, gFuxiPciDev, gFuxiPciFunc, bdf, name_net_dev);
    if (0 && netname)
        printf("pci %s map to net device:%s\n", bdf, name_net_dev);

    /* map registers to memory */
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        FATAL;

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if (map_base == (void *)-1)
    {
        cout << "err: PCI space " << target << ", memory mapped at address " << map_base << ", errno=" << errno << ", " << strerror(errno) << endl;
        close(fd);
        FATAL;
    }

    gFuxiMemBase = map_base;
    gTarget = target;

    if (argc == 1)
    {
        // block for MAC address read
        {
            uint8_t macaddr[6] = {0};
            u32 reg = 0, value = 0;
            u32 machr = 0, maclr = 0;
            int valid_mac = 0;

            for (u8 index = 0;; index++)
            {
                if (!read_patch_from_efuse_per_index(map_base, target, index, &reg, &value))
                {
                    break; // reach the last item.
                }
                if (0x00 == reg)
                {
                    break; // reach the blank.
                }
                if (MACA0LR_FROM_EFUSE == reg)
                {
                    valid_mac = 1;
                    maclr = value;
                }
                if (MACA0HR_FROM_EFUSE == reg)
                {
                    valid_mac = 1;
                    machr = value;
                }
            }

            if (valid_mac)
            {
#if __BYTE_ORDER == __BIG_ENDIAN
                macaddr[1] = (machr >> 8) & 0xFF;
                macaddr[0] = (machr) & 0xFF;
                macaddr[5] = (maclr >> 24) & 0xFF;
                macaddr[4] = (maclr >> 16) & 0xFF;
                macaddr[3] = (maclr >> 8) & 0xFF;
                macaddr[2] = (maclr) & 0xFF;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
                macaddr[0] = (machr >> 8) & 0xFF;
                macaddr[1] = (machr) & 0xFF;
                macaddr[2] = (maclr >> 24) & 0xFF;
                macaddr[3] = (maclr >> 16) & 0xFF;
                macaddr[4] = (maclr >> 8) & 0xFF;
                macaddr[5] = (maclr) & 0xFF;
#else
                error "please contact provider for Endian issue fix."
#endif
            }

            cout << "Present Mac Address: ";
            {
                for (i = 0; i < _countof(macaddr); i++)
                {
                    printf("%02x", macaddr[i]);
                    if (i < _countof(macaddr) - 1)
                    {
                        cout << ":";
                    }
                }
                cout << endl;
            }

            if (!valid_mac)
            {
                cout << "Default MAC address will be loaded: 00:55:7B:B5:7D:F7" << endl;
            }
        }
    }
    else if (argc >= 2)
    {

        // block for mac address write and verification
        string macaddr_str = argv[1];
        uint8_t macaddr[6];
        uint_least32_t addr[6];
        uint8_t verify = 0, verify_ok = 0;
        uint8_t dbg_err = 0;

        if ((argc == 2) && (!strcmp(argv[1], "-h")))
        {
            printf("%s \t\t\t- Display current MAC address.\n", argv[0]);
            printf("%s <MAC address> \t- Burn and verify MAC address.\n", argv[0]);
            printf("%s svid [svid] \t- Burn subsystem Vendor ID and subsystem(Device) ID, eg:0x28066866.\n", argv[0]);
            printf("%s led [led_id] \t- Burn LED mode, can be 0,1,2 presently.\n", argv[0]);
            printf("%s wol [1] \t\t- Burn WOL OOB, must set to 1.\n", argv[0]);
            printf("%s lbc [number] \t- Do cable loopback test with specified number of round. Default 1.\n", argv[0]);
            printf("%s dlostfix [val] \t- Display or Burn Clock Gate config.\n", argv[0]);
            printf("%s net \t\t- Display PCIe BDF and Net devices name.\n", argv[0]);
            printf("%s -v \t\t- Display all YT6801 present.\n", argv[0]);
            printf("%s -h \t\t- Display this list.\n", argv[0]);

            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "net")))
        {
            char tmp_netdev_name[50] = {0};
            char dbgfs_netops[256] = {0};
            FILE *tmp_fd;

            sprintf(dbgfs_netops, "%snetdev_ops", gFuxiDfPath);
            tmp_fd = fopen(dbgfs_netops, "r");
            if (tmp_fd)
            {
                while (!feof(tmp_fd))
                {
                    fscanf(tmp_fd, "%s", (char *)tmp_netdev_name);
                }
                fclose(tmp_fd);
            }

            printf("pci %s map to net device:%s, df=%s-%s\n", bdf, netname ? name_net_dev : "No net device", gFuxiDfPath, 0 == tmp_netdev_name[0] ? "not exist." : tmp_netdev_name);

            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "lbc")))
        {
            int num_of_test = 1;

            if (argc >= 3)
            {
                num_of_test = atoi(argv[2]);
                if (!num_of_test)
                    num_of_test = 1;
            }
            gFuxiLpPacketsNum = num_of_test;

            // printf("currently, cable loopback test can only applied on the first NO. of NIC.\n");
            printf("please make sure loopback cable is plugged in right NIC.\nCable loopback test start...\n");
#if 0
			for(i = 0; i < num_of_test; i++)
			{	
				printf("cable loop test round %d of %d...\n", i + 1, num_of_test);
				if(loopback_test(CMD_CHK_CABLE_LP) != STATUS_SUCCESSFUL){
					printf("cable loop test round %d of %d...failed.\n", i + 1, num_of_test);

					ret = -1;
					break;
				}
				else{
					printf("cable loop test round %d of %d...passed\n", i + 1, num_of_test);
				}
			}
#else
            if (loopback_test(CMD_CHK_CABLE_LP) != STATUS_SUCCESSFUL)
            {
                printf("cable loop test failed.\n");
                ret = -1;
            }
            else
            {
                printf("cable loop test round %d...passed\n", num_of_test);
            }

#endif

            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "led")))
        {
            uint32_t value;

            ret = 0;
            if (true == efuse_read_regionA_regionB(map_base, target, 0x00, &value))
            {
                if (argc == 2)
                {
                    printf("present LED Mode:\t%u\n", value & 0x1f);
                }
                else if (argc >= 3)
                {
                    int nNumLed = atoi(argv[2]);

                    if ((0 <= nNumLed) && (32 > nNumLed)) /* 0-31 */
                    {
                        if ((nNumLed != (value & 0x1f)) && (0 == (value & 0x1f)))
                        {
                            /* changable only when 0 to 1 or 2 */
                            if (true == efuse_write_led(map_base, target, (u32)nNumLed & 0x1f))
                            {
                                printf("write led value %d\n", nNumLed);
                            }
                            else
                            {
                                printf("Write Led Id %u error.\n", (u32)nNumLed & 0x1f);
                                ret = 1;
                            }
                        }
                        else if (nNumLed != (value & 0x1f))
                        {
                            printf("LED mode cannot be changed twice, current %d, requested %d.\n", value & 0x1f, nNumLed);
                            ret = -2;
                        }
                    }
                    else
                    {
                        printf("Invalid LED mode input, only 0-31 are accepted.\n");
                        ret = -1;
                    }
                }
            }
            else
            {
                printf("ioctl error, check privilege or if device is ready?\n");
                ret = 1;
            }

            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "wol")))
        {
            uint32_t value;

            if (true == efuse_read_regionA_regionB(map_base, target, 0x7, &value))
            {
                if (argc == 2)
                {
                    printf("present WOL Status:\t%s\n", value & 0x4 ? "Enabled(1)" : "Disabled(0)");
                }
                else if (argc >= 2)
                {
                    int nNumWol = atoi(argv[2]);

                    // printf("WOL - current %d, requested %d.\n", value & 0x4 ? 1 : 0, nNumWol);

                    if ((1 == nNumWol) && (0 == (value & 0x4)))
                    {
                        if (true == efuse_write_oob(map_base, target))
                        {
                            printf("write wol to %d.\n", nNumWol);
                        }
                        else
                        {
                            printf("ioctl error, check privilege or if device is ready?\n");
                            ret = 1;
                        }
                    }
                    else if ((0 == nNumWol) && (4 == (value & 0x4)))
                    {
                        printf("WOL cannot be changed twice, current %d, requested %d.\n", value & 0x4 ? 1 : 0, nNumWol);
                        ret = -2;
                    }
                }
            }
            else
            {
                printf("ioctl error, check privilege or if device is ready?\n");
                ret = 1;
            }
            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "dlostfix")))
        {
            u32 offset = 0, value, tmpValue = 0;
            bool succeed = false;
            u8 index = 0;
            uint32_t reg = 0x1188;

            for (index = 0;; index++)
            {
                if (true != read_patch_from_efuse_per_index(map_base, target, index, &offset, &value))
                {
                    succeed = false;
                    break; // reach the last item.
                }

                if (0x00 == offset)
                {
                    break; // reach the blank.
                }
                if (reg == offset)
                {
                    succeed = true;
                    tmpValue = value;
                }
            }

            if (1)
            {
                if (argc == 2)
                {
                    if (succeed)
                    {
                        printf("present DevLostFixBit Status:\t0x%08x\n", tmpValue);
                    }
                    else
                    {
                        printf("present DevLostFixBit is not configured.\n");
                    }
                }
                else if (argc >= 2)
                {
#define TMP_POWER_LOST_BITS nNum // 0x10111	//

                    // int nNum = atoi(argv[2]);
                    u32 nNum = strtoul(argv[2], NULL, 16);
                    // int nNum = 1;

                    value = tmpValue;
                    printf("%s - dev lost fix current %x, requested %x.\n", argv[0], value, TMP_POWER_LOST_BITS);

                    // if(1 == nNum) //for debug
                    if ((TMP_POWER_LOST_BITS & 0x10) && (TMP_POWER_LOST_BITS != value))
                    {
                        reg = 0x1188;
                        value = TMP_POWER_LOST_BITS;

                        write_patch_to_efuse(map_base, target, reg, value);
                        if (1)
                        {
                            printf("write DevLostFixBit to %x.\n", nNum);

                            // verify...
                            value = 0;
                            succeed = 0;
                            for (index = 0;; index++)
                            {
                                if (true != read_patch_from_efuse_per_index(map_base, target, index, &offset, &value))
                                {
                                    succeed = false;
                                    break; // reach the last item.
                                }

                                if (0x00 == offset)
                                {
                                    break; // reach the blank.
                                }
                                if (reg == offset)
                                {
                                    succeed = true;
                                    tmpValue = value; // tmpValue contains the last value of dlostfix.
                                }
                            }

                            value = tmpValue;
                            if (succeed)
                            {
                                // if((value & TMP_POWER_LOST_BITS) && (0 == (value & TMP_POWER_LOST_BITS2)))
                                if (value == TMP_POWER_LOST_BITS)
                                {
                                    printf("verified DevLostFixBit, 0x%x, pass.\n", value);

                                    /* check current reg value and patched-reg value */
                                    value = readl((unsigned char *)map_base + ((target + reg) & MAP_MASK));
                                    if (TMP_POWER_LOST_BITS != value)
                                    {
                                        printf("Please restart to take the value effection, current 0x%x.\n", value);
                                    }
                                }
                                else
                                {
                                    printf("DevLostFixBit verified, 0x%x, Failed\n", value);
                                    ret = 1;
                                }
                            }
                            else
                            {
                                printf("error, read patch failed, verification stopped.\n");
                                ret = 1;
                            }
                        }
                        else
                        {
                            printf("ioctl error, check privilege or driver loaded?\n");
                            ret = 1;
                        }
                    }
                    else if (0 == (0x10 & nNum))
                    {
                        printf("DevLostFixBit(b[4]) cannot be changed to 0, current 0x%08x, requested 0x%08x.\n", value, nNum);

                        ret = -2;
                    }
                    else
                    {
                        printf("Duplication operation for DevLostFixBit, current 0x%08x, requested 0x%08x.\n", value, nNum);
                    }
                }
            }

            goto sole_exit;
        }
        else if ((argc >= 2) && (!strcmp(argv[1], "svid")))
        {
            u32 value;
            u32 subsys, subvendor;
            u32 offset = 0, tmpValue = 0;
            bool succeed = false;
            u8 index = 0;

            for (index = 0;; index++)
            {
                if (true != read_patch_from_efuse_per_index(map_base, target, index, &offset, &value))
                {
                    succeed = false;
                    break; // reach the last item.
                }

                if (0x00 == offset)
                {
                    break; // reach the blank.
                }
                if (EFUSE_SUBSYS_REGISTER == offset)
                {
                    succeed = true;
                    tmpValue = value;
                }
            }
            /* about subsystem ID-4B: offset 0x2c,
             * means 2B subvendor ID(offset 0x2C)
             * and 2B subsystem(also sub-device id), offset 0x2E
             */
            if (1)
            {
#if __BYTE_ORDER == __BIG_ENDIAN
                subvendor = (tmpValue & 0xffff0000) >> 16;
                subsys = tmpValue & 0xffff;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
                subsys = (tmpValue & 0xffff0000) >> 16;
                subvendor = tmpValue & 0xffff;
#else
                error "please contact provider for Endian issue fix."
#endif

                // printf("Done read from patch,,tmpVal=0x%08x,val=0x%08x\n", tmpValue, value);

                if (argc == 2)
                {
                    if (succeed)
                    {
                        printf("present Susystem Vendor ID:0x%04x, Subsystem (device) ID:0x%04x\n", subvendor, subsys);
                    }
                    else
                    {
                        printf("No configured Subsystem IDs, default is used.\npresent Subsystem Vendor ID:0x%04x, Subsystem (device) ID:0x%04x\n",
                               yt6801_devices[cur_yt6801_dev].sub_ven_id, yt6801_devices[cur_yt6801_dev].sub_dev_id);
                    }
                }
                else if (argc >= 2)
                {
                    string str_svid = argv[2];
                    unsigned long nNumId;

                    if ((str_svid.find("0x") != string::npos) && (str_svid.size() > 2))
                    {
                        nNumId = strtoul(argv[2], NULL, 16);

                        // printf("SVID - current 0x%x, requested 0x%x.\n", value, nNumId);

                        nNumId &= 0xffffffff;
                        if (!succeed || (tmpValue != nNumId))
                        {
                            subvendor = (nNumId & 0xffff0000) >> 16;
                            subsys = nNumId & 0xffff;

                            if (0 == nNumId)
                            {
                                printf("WARNING: about to set subsystem Vendor ID and subsystem (device) ID to 0, please enter 'y' to confirm, other to abort...\n");
                                char stopp = getchar();

                                // printf ("char input: %c %d\n", stopp, (int)stopp);
                                if ('y' != stopp)
                                {
                                    ret = -1;
                                    goto sole_exit;
                                }
                            }
#if __BYTE_ORDER == __BIG_ENDIAN
                            value = (subvendor & 0xffff) << 16;
                            value = value | (subsys & 0xffff);
#elif __BYTE_ORDER == __LITTLE_ENDIAN
                            value = (subsys & 0xffff) << 16;
                            value = value | (subvendor & 0xffff);
#else
                            error "please contact provider for Endian issue fix."
#endif
                            printf("about to write patch,,arg=0x%08x,val=0x%08x\n", (unsigned int)nNumId, value);
                            if (true == write_mac_subsys_to_efuse(map_base, target, NULL, (u32 *)&value, NULL))
                            {
                                printf("write subsystem Vendor ID and subsystem (device) ID ok, 0x%04x%04x.\n", subvendor, subsys /*nNumId*/);
                                printf("WARNING: Please restart NIC to take effect with new subsystem Vendor ID and subsystem (device) ID.\n");
                            }
                            else
                            {
                                printf("1) check privilege or driver loaded? 2)may efuse is used up.\n");
                                ret = 1;
                            }
                        }
                        else
                        {
                            printf("Same subsystem Vendor ID and subsystem (device) ID already.\n");
                        }
                    }
                    else
                    {
                        printf("Invalid subsystem Vendor ID and subsystem (device) ID.\n");
                        ret = -1;
                    }
                }
            }
            goto sole_exit;
        }
        else if ((argc == 2) && (!strcmp(argv[1], "ev")))
        {
            uint32_t reg;
            uint32_t value;
            int avail = 0;

            for (i = 0; i < 39; i++)
            {
                if (read_patch_from_efuse_per_index(map_base, target, i, &reg, &value))
                {
                    if (reg || value)
                    {
                        printf("efuse: [%02d] reg 0x%04x - 0x%08x\n", i, reg, value);
                    }
                    else
                        avail++;
                }
                else
                {
                    printf("ioctl error, check privilege or if device is ready?\n");
                }
            }
            printf("Available: %d.\n", avail);

            goto sole_exit;
        }
        else if ((argc == 3) && (!strcmp(argv[1], "-t")))
        {
            // macaddr_str = argv[2];
            macaddr_str.assign(argv[2]);
            dbg_err = 1;
        }
        else if ((argc == 3) && (!strcmp(argv[2], "-t")))
        {
            dbg_err = 1;
        }

        if (_countof(macaddr) != sscanf(macaddr_str.c_str(), "%x:%x:%x:%x:%x:%x", &addr[5], &addr[4], &addr[3], &addr[2], &addr[1], &addr[0]))
        {
            printf("Bad format for mac address, %s\n", argv[1]);

            ret = 2;
            goto sole_exit;
        }

        for (i = 0; i < _countof(macaddr); i++)
        {
            macaddr[i] = addr[i];
        }

#if __BYTE_ORDER == __BIG_ENDIAN
        // if(0)
        {
#define SWAP_BYTE(var, a, b) \
    do                       \
    {                        \
        u8 tmpByte = var[a]; \
        var[a] = var[b];     \
        var[b] = tmpByte;    \
    } while (0)

            SWAP_BYTE(macaddr, 4, 5);
            SWAP_BYTE(macaddr, 0, 3);
            SWAP_BYTE(macaddr, 1, 2);
        }
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#else
        error "please contact provider for Endian issue fix."
#endif

        write_mac_addr_to_efuse(map_base, target, (u8 *)macaddr);
        if (1)
        {
            cout << "Write Mac Address : ";
            for (int i = 0; i < _countof(macaddr); i++)
            {
                printf("%02x", macaddr[i]);
                if (i < _countof(macaddr) - 1)
                {
                    cout << ":";
                }
            }
            cout << endl;
            verify = 1;
        }

        if (verify)
        {
            uint8_t macaddr_verify[6];
            u32 reg = 0, value = 0;
            u32 machr = 0, maclr = 0;
            int valid_mac = 0;

            cout << "Verify Mac Address: ";
            for (i = 0;; i++)
            {
                if (!read_patch_from_efuse_per_index(map_base, target, i, &reg, &value))
                {
                    break; // reach the last item.
                }
                if (0x00 == reg)
                {
                    break; // reach the blank.
                }
                if (MACA0LR_FROM_EFUSE == reg)
                {
                    valid_mac = 1;
                    maclr = value;
                }
                if (MACA0HR_FROM_EFUSE == reg)
                {
                    valid_mac = 1;
                    machr = value;
                }
            }

            if (valid_mac)
            {

#if __BYTE_ORDER == __BIG_ENDIAN
                macaddr_verify[4] = (machr >> 8) & 0xFF;
                macaddr_verify[5] = (machr) & 0xFF;
                macaddr_verify[0] = (maclr >> 24) & 0xFF;
                macaddr_verify[1] = (maclr >> 16) & 0xFF;
                macaddr_verify[2] = (maclr >> 8) & 0xFF;
                macaddr_verify[3] = (maclr) & 0xFF;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
                macaddr_verify[5] = (machr >> 8) & 0xFF;
                macaddr_verify[4] = (machr) & 0xFF;
                macaddr_verify[3] = (maclr >> 24) & 0xFF;
                macaddr_verify[2] = (maclr >> 16) & 0xFF;
                macaddr_verify[1] = (maclr >> 8) & 0xFF;
                macaddr_verify[0] = (maclr) & 0xFF;
#else
                error "please contact provider for Endian issue fix."
#endif

                verify_ok = 1;
                for (int i = 0; i < _countof(macaddr_verify); i++)
                {
                    if (dbg_err && (i == 3))
                        macaddr_verify[i] = ~macaddr[i]; // inject an err for test only

                    if (macaddr[i] != macaddr_verify[i])
                    {
                        verify_ok = 0;
                    }
                    printf("%02x", macaddr_verify[i]);
                    if (i < _countof(macaddr_verify) - 1)
                    {
                        cout << ":";
                    }
                }
                cout << endl;
            }
            else
            {
                verify_ok = 0;
            }
        }

        if (verify_ok)
        {
            printf("Successfully.\n");
        }
        else
        {
            printf("Failed.\nPress Enter to exit.");
            ret = 4;
            // getchar();
        }
    }

sole_exit:
    if (munmap(map_base, MAP_SIZE) == -1)
        FATAL;
    close(fd);

    return ret;

    /* print help information */
    cout << "                   Motorcomm PCIe Utils " << gVersion << endl;
    print_help();
    cout << endl;

    if (argc == 1)
    {
        while (true)
        {
            static string args;
            static unsigned int nRepeat = 0, nFile = 0;
            static ifstream fin;

            if (!nRepeat)
            {
                cout << CMD_PROMPT;

                if (fin.is_open())
                {
                    nFile = 0;
                    if (!fin.eof())
                    {
                        do
                        {
                            getline(fin, args);
                        } while ((!fin.eof()) && args.empty());
                        cout << args << endl;
                        nFile = 1;
                    }
                    else
                    {
                        fin.close();
                        cout << "done." << endl;
                        cout << CMD_PROMPT;
                    }
                }

                if (!nFile)
                {
                    getline(cin, args, '\n');
                    // cout << "cmd as chars:" << args.c_str() << endl;
                }

                stringstream ssTemp(args);
                vector<string> args_vec_temp;
                string argTemp;
                while (ssTemp >> argTemp)
                {
                    args_vec_temp.push_back(argTemp);
                }

                if (args_vec_temp.size() == 0)
                {
                    print_help();
                    continue;
                }
                else
                {
                    string cmdTemp = args_vec_temp[0];
                    if (cmdTemp == "rep") // repeat test command.
                    {
                        if (args_vec_temp.size() >= 3)
                        {
                            stringstream sstr;
                            unsigned int pos;

                            sstr << dec << args_vec_temp[1];
                            sstr >> nRepeat;
                            sstr.clear();

                            // cout << "detect repeated cmd: " << cmdTemp << " " << nRepeat << endl;
                            pos = (unsigned int)args.find(args_vec_temp[1]);
                            if (0 < pos)
                            {
                                // cout << "arg1 is from " << pos << " and its len= " <<args_vec_temp[1].length() << endl;
                                args.erase(0, pos + args_vec_temp[1].length());
                                cout << "Cmd \'" << args.c_str() << "\' will be repeated for " << std::dec << nRepeat-- << " times, press any key to stop..." << endl;
                            }
                            else
                            {
                                cout << "argument error." << endl;
                                nRepeat = 0;
                                continue;
                            }
                        }
                    }
                    else if (cmdTemp == "rem")
                    { // for comments line in a cmd file
                        continue;
                    }
                    else if (cmdTemp == "file")
                    { // read cmd from a file.
                        if (args_vec_temp.size() >= 2)
                        {
                            if (fin.is_open())
                            {
                                cout << "pls wait for completion of previous file cmd." << endl;
                                continue;
                            }

                            fin.open(args_vec_temp[1]);
                            if (!fin.is_open())
                            {
                                cout << "cannot open file " << args_vec_temp[1] << endl;
                                // cout << args_vec_temp[0] << " " << args_vec_temp[1] << "failed" << endl;
                            }
                        }
                        else
                        {
                            char buffTmp[250];
                            getcwd(buffTmp, 250);

                            cout << buffTmp << endl;
                        }

                        continue;
                    }
                    // continue;
                }
            }
            else
            {
                nRepeat--;
                sleep(1);
                // if(_Exit){
                //     nRepeat = 0;
                //     continue;
                // }
            }

            stringstream ss(args);
            vector<string> args_vec;
            string arg;
            while (ss >> arg)
            {
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
                        string subcmd = args_vec[1];
                        string macaddr_str = args_vec[2];
                        uint_least32_t addr[6];
                        uint8_t macaddr[6];
                        uint8_t num = sizeof(macaddr) / sizeof(macaddr[0]);
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
                            if (true != efuse_read_regionA_regionB(map_base, target, 0x00, &ledId))
                            {
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
                        string subcmd = args_vec[1];
                        string str_ledId = args_vec[2];
                        uint32_t oldLedId = 0;
                        bool ret = false;

                        if (subcmd == "w")
                        {
                            if (!sscanf(str_ledId.c_str(), "%x", &ledId))
                            {
                                cout << "Invalid parameters. ledId is " << hex << ledId << endl;
                                continue;
                            }

                            if (true != efuse_read_regionA_regionB(map_base, target, 0x00, &oldLedId))
                            {
                                printf("Read Led Id error.\n");
                            }
                            else
                            {
                                if (oldLedId != 0)
                                {
                                    printf("Led Id has written %X, cannot write it again.\n", oldLedId);
                                }
                                else
                                {
                                    ret = efuse_write_led(map_base, target, ledId);
                                    if (true != ret)
                                    {
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
                            if (!efuse_read_regionA_regionB(map_base, target, 0x07, &reg_val))
                            {
                                printf("Read OOB error.\n");
                            }
                            else
                            {
                                if (XLGMAC_GET_REG_BITS(reg_val, 2, 1))
                                {
                                    printf("OOB Ctrl bit already exists.\n");
                                }
                                else
                                {
                                    printf("OOB Ctrl bit does not exist.\n");
                                }
                            }
                        }
                        else if (subcmd == "w")
                        {
                            if (!efuse_read_regionA_regionB(map_base, target, 0x07, &reg_val))
                            {
                                printf("Read OOB error.\n");
                            }
                            else
                            {
                                if (XLGMAC_GET_REG_BITS(reg_val, 2, 1))
                                {
                                    printf("OOB Ctrl bit already exists.\n");
                                }
                                else
                                {
                                    if (!efuse_write_oob(map_base, target))
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
                        if (loopback_test(CMD_CHK_PHY_LP) != STATUS_SUCCESSFUL)
                        {
                            cout << "phy loopback test fail !" << endl;
                        }
                        else
                        {
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

                            if (subcmd == "it")
                            {
                                type |= CMD_CHK_TCP;
                                printf("tcp checksum offload test start ...... \n");
                            }
                            else
                            {
                                printf("ucp checksum offload test start ...... \n");
                            }

                            if (loopback_test(type) != STATUS_SUCCESSFUL)
                            {
                                cout << "checksum offload test fail !" << endl;
                            }
                            else
                            {
                                cout << "checksum offload test pass !" << endl;
                            }
                        }
                        else if (subcmd == "lso")
                        {
                            type |= CMD_CHK_LSO_V2;
                            if (loopback_test(type) != STATUS_SUCCESSFUL)
                            {
                                cout << "large size offload test fail !" << endl;
                            }
                            else
                            {
                                cout << "large size offload test pass !" << endl;
                            }
                        }
                        else
                        {
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
                        if (loopback_test(CMD_CHK_CABLE_LP) != STATUS_SUCCESSFUL)
                        {
                            cout << "cable loopback test fail !" << endl;
                        }
                        else
                        {
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
                            // cout << "subcmd is set."<< endl;
                            if (args_vec.size() == 4)
                            {
                                string str_subsys = args_vec[2];
                                string str_subvendor = args_vec[3];
                                uint32_t subsys, subvendor;

                                if (!sscanf(str_subsys.c_str(), "%x", &subsys) || !sscanf(str_subvendor.c_str(), "%x", &subvendor))
                                {
                                    cout << "Invalid parameters. subsys is " << hex << subsys << ", subverdor is " << hex << subvendor << endl;
                                    continue;
                                }

                                write_subsys_info_to_efuse(map_base, target, subsys, subvendor);
                            }
                            else
                            {
                                cout << "pci set command has wong patameters, it has " << args_vec.size() << " parameters." << endl;
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
                else if (cmd == "el") // efuse load.
                {
                    efuse_load(map_base, target);
                }
                else if (cmd == "ev") // Efuse View. Read all efuse content in regionC, parse them.
                {
                    uint32_t reg = 0;
                    uint32_t value = 0;
                    uint32_t index = 0;

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
                            if ((index + 1) % 5 == 0)
                                printf("\n");
                        }
                    }
                    printf("\n");

                    printf("Efuse Region C:\n");
                    for (index = 0; index < 39; index++)
                    {
                        read_patch_from_efuse_per_index(map_base, target, index, &reg, &value);
                        printf("efuse: [%02d] reg0x%04x - 0x%08x\n", index, reg, value);
                    }
                }
                else if (cmd == "ewi") // ewi <index> <reg> <value>. Write efuse patch, to 0-based entry index.
                {
                    if (args_vec.size() >= 4)
                    {
                        stringstream sstr;
                        uint32_t index;
                        uint32_t reg;
                        uint32_t value;
                        uint32_t cur_reg = 0;
                        uint32_t cur_val = 0;

                        sstr << hex << args_vec[1];
                        sstr >> index;
                        sstr.clear();
                        sstr << hex << args_vec[2];
                        sstr >> reg;
                        sstr.clear();
                        sstr << hex << args_vec[3];
                        sstr >> value;

                        if (index > 38)
                        {
                            printf("index 0x%x out of range, must be less than 0x27.\n", index);
                            continue;
                        }
                        read_patch_from_efuse_per_index(map_base, target, index, &cur_reg, &cur_val);

                        if (cur_reg == 0 && cur_val == 0)
                        {
                            write_patch_to_efuse_per_index(map_base, target, index, reg, value);
                            if (!(reg >> 16))
                                printf("Reg %x entry current value %x.\n", reg, value);
                        }
                        else
                        {
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
                        stringstream sstr;
                        uint32_t reg;

                        sstr << hex << args_vec[1];
                        sstr >> reg;
                        read_patch_from_efuse(map_base, target, reg);
                    }
                    else
                    {
                        cout << "err <reg>" << endl;
                    }
                }
                else if (cmd == "ewr") // ewr <reg> <value>. Write efuse patch for specified register.
                {
                    if (args_vec.size() >= 3)
                    {
                        stringstream sstr;
                        uint32_t reg;
                        uint32_t value;

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
                        cout << "r <reg>\n"
                             << endl;
                    }
                }
                else if (cmd == "w") // write GMAC registers.
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
                        cout << "w <reg> <value>\n"
                             << endl;
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
                        cout << "cr <reg>\n"
                             << endl;
                    }
                }
                else if (cmd == "cw") // write CFG registers.
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
    }
    else
    {
        cout << "The PCI tool doesnot need any paramters, please check it." << endl;
        if (munmap(map_base, MAP_SIZE) == -1)
            FATAL;
        close(fd);
        return -1;
    }

    if (munmap(map_base, MAP_SIZE) == -1)
        FATAL;
    close(fd);
    return 0;
}
