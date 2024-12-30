/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_lpbk.cpp

Abstract:

    FUXI Linux loopback test function

Environment:

    User mode.

Revision History:
    2022-03-14    xiaogang.fan@motor-comm.com    created

--*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <net/if.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include<arpa/inet.h>

#include "fuxipci_comm.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_linux_driver.h"
#include "fuxipci_err.h"
#include "fuxipci_net.h"
#include "fuxipci_lso.h"
#include "fuxipci_rss.h"
#include "fuxipci_xsum.h"


using namespace std;

extern ADAPTER ADPT; 
extern u8 rss_key[];
// global variables defined here, all start with g_ 
// fields are to be filled by APP or DRIVER
ATHR_DOSAPP_CHAR    gAthrDosappChar;
// driver structure returned by driver
PVOID               gPDriver;

// Save some adapter related information that required in scope of APP
// The NIC position info and memory used by DRIVER are saved in this structure.
// this structure can be extended when neccessary
// the Bus/Dev/Func of NIC to be used, if not specified by user, will be the 1st found one
APP_ADAPTER         gAppAdapter;

PATHR_DOS_PACKET    gpRecvdPkt;     // linked to the end of previous packets
u16                 gRecvdPktNum;   // accumulated each time IndicatePackets is invoked
u16                 gRecvdPktAttr;  // overwritten by the new incoming packets   
u32                 gBufSize;

pci_dev_id dev_list[] = {
	{ 0x1f0a, 0x6801 },
};

int athr_getch()
{
    struct termios old_termios, new_termios;
    int  error;
    char c;


    fflush(stdout);
    tcgetattr(0, &old_termios);
    new_termios = old_termios;

    /* raw mode, line settings */
    new_termios.c_lflag &= ~ICANON;
#ifdef TERMIOSECHO
    /* enable echoing the char as it is typed */
    new_termios.c_lflag |=  ECHO;
#else
    /* disable echoing the char as it is typed */
    new_termios.c_lflag &= ~ECHO;
#endif

#ifdef TERMIOSFLUSH
    /* use this to flush the input buffer before blocking for new input */
    #define OPTIONAL_ACTIONS TCSAFLUSH
#else
    /*
     * use this to return a char from the current input buffer, or block
     * if no input is waiting
     */
    #define OPTIONAL_ACTIONS TCSANOW
#endif

    /* minimum chars to wait for */
    new_termios.c_cc[VMIN] = 1;
    /* minimum wait time, 1 * 0.10s */
    new_termios.c_cc[VTIME] = 1;

    error = tcsetattr(0, OPTIONAL_ACTIONS, &new_termios);
    if (0 == error) {
        /* get char from stdin */
        error  = read(0, &c, 1);
    }

    /* restore old settings */
    error += tcsetattr(0, OPTIONAL_ACTIONS, &old_termios);
    return (error == 1 ? (int)c : -1);
}

/*
 * kbhit() -- a keyboard lookahead monitor
 *
 * returns the number of characters available to read
 */
int athr_kbhit()
{
    struct timeval tv;
    struct termios old_termios, new_termios;
    int            error;
    int            count = 0;

    tcgetattr(0, &old_termios);
    new_termios              = old_termios;

    /* raw mode */
    new_termios.c_lflag &= ~ICANON;

    /* disable echoing the char as it is typed */
    new_termios.c_lflag &= ~ECHO;

    /* minimum chars to wait for */
    new_termios.c_cc[VMIN] = 1;

    /* minimum wait time, 1 * 0.10s */
    new_termios.c_cc[VTIME] = 1;
    error = tcsetattr(0, TCSANOW, &new_termios);

    tv.tv_sec  = 0;
    tv.tv_usec = 100;
    /* insert a minimal delay */
    select(1, NULL, NULL, NULL, &tv);
    error += ioctl(0, FIONREAD, &count );
    error += tcsetattr(0, TCSANOW, &old_termios);
    return( error == 0 ? count : -1 );
}

// allocate enough memory for app to use
int allocAppMem(IN PAPP_ADAPTER pAppAdapter)
{
    PVOID   virAddr = NULL;
    u32     memSize = 0;

    // for Tx Assume : 128K packet, max count equals 100
    memSize = MAX_PACKET_COUNT * 64 * 1024 * 2;

    virAddr = malloc(memSize);   //DBG only
    if (virAddr == NULL)
    {
        printf("alloc dma-memory(%uKB) failed\n", memSize/1024);
        return STATUS_FAILED;
    }
    pAppAdapter->pDmaVirAddr = (u8*)virAddr;
    pAppAdapter->pAlignedDmaVirAddr = (u8*)virAddr;

    // for Rx, max to RxRingSize
    memSize += sizeof(ATHR_DOS_PACKET) * 4096;
    gpRecvdPkt = (PATHR_DOS_PACKET)malloc(memSize);
    gBufSize = memSize;

    // >>> For DBG only
#if 0
    printf("DmaVirMem = %p\n", pAppAdapter->pDmaVirAddr);
    printf("DmaPhyMem = %08x\n", pAppAdapter->DmaPhyAddr);
    printf("AlignedDmaVirMem = %p\n", pAppAdapter->pAlignedDmaVirAddr);
    printf("AlignedDmaPhyMem = %08x\n", pAppAdapter->AlignedDmaPhyAddr);
    printf("gpRecvdPkt=%p, gBufSize=%d\n", gpRecvdPkt, gBufSize);
#endif
    return STATUS_SUCCESSFUL;
}

// free the memory allocated by allocAppMem
int freeAppMem(IN PAPP_ADAPTER pAppAdapter)
{
    if (pAppAdapter->pDmaVirAddr != NULL) 
    {
        free(pAppAdapter->pDmaVirAddr);
        pAppAdapter->pDmaVirAddr = NULL;
    }
    pAppAdapter->pAlignedDmaVirAddr = NULL;

    if (gpRecvdPkt)
    {
        free(gpRecvdPkt);
        gpRecvdPkt = NULL;
    }
    return STATUS_SUCCESSFUL;
}

bool GetKey(char* buf, const char* name, int type, u32* pv)
{
    char *p;
    char buffer[200];
    int l;

    p = strstr(buf, name);
    if (NULL == p) {
        return false;
    }
        
    // get whole line
    for (l=0; l < sizeof(buffer); l++)
    {
        if (*p != 0xA)
            buffer[l] = *p;
        else
        {
            buffer[l] = 0;
            break;
        }
        p++;
    }
    if (l == sizeof(buffer))
        buffer[l-1] = 0;
    
    p = strstr(buffer, "=");
    p += strlen("=");
    if (type == 0) // decimel interger
    {
        char* p2;
        *pv = strtoul(p, &p2, 10);
    }
    else
    {
        char* p2;
        *pv = strtoul(p, &p2, 16);
    }

    return true;
}


void GetParamFromFile(u8* pAdapter, char* paramFile, 
                      struct param_entry* params, int paramCount)
{
    char DefaultName[] = "p.ini";
    FILE* f;
    char* buf = NULL;
    int filelength, i, cnt;

    if (!paramFile)
    {
        paramFile = DefaultName;
    }

    f = fopen(paramFile, "rt");
    if (NULL == f) 
        return;
        
    fseek(f, 0, SEEK_END);
    filelength = ftell(f);

    buf = (char*)malloc(filelength+1);
    if (NULL == buf)
        return;

    memset(buf, 0, filelength+1);
    fseek(f, 0, SEEK_SET);
    cnt = fread(buf, filelength, 1, f);
    fclose(f);

    //DBG_PRINT(("Get parameters from file %s\n"), paramFile);
    for (i=0; i<paramCount; i++)
    {
        u32 v;
        u8* P = (u8*)pAdapter + params[i].offset;
        if (GetKey(buf, params[i].name, params[i].format, &v))
        {
            if ((int)v < params[i].min || (int)v > params[i].max)
                v = params[i].def;

            switch (params[i].size)
            {
            case 1: // byte
                *(u8*) P = (u8)v;  break;
            case 2: // word
                *(u16*)P = (u16)v; break;
            case 4: // word
                *(u32*)P = (u32)v; break;
            default:
                printf("dskfslkdflksfdlksdfslkdflksdlkfdslkfslkdflkslkfdlsf!!!!!\n");
                break;          
            }
            #if DEBUG
            if (params[i].format)
            {
                DBG_PRINT(("%s=0x%lX\n", params[i].name, v));
            }
            else
            {
                DBG_PRINT(("%s=%ld\n", params[i].name, v));
            }
            #endif
        }
    }

    free(buf);
}


struct param_entry drvParams[] ={
//{"TpdRingSize",     0,  MP_OFFSET(ADAPTER, TpdRingSize),   MP_SIZE(ADAPTER, TpdRingSize),   256,    6,      2048},
//{"RxRingSize",      0,  MP_OFFSET(ADAPTER, RxRingSize),    MP_SIZE(ADAPTER, RxRingSize),    512,    8,      4095},
//{"RxBufBlockSize",  0,  MP_OFFSET(ADAPTER, RxBufBlockSize),MP_SIZE(ADAPTER, RxBufBlockSize),1536,   128,    9216},
{"MaxFrameSize",    0,  MP_OFFSET(ADAPTER, MaxFrameSize),  MP_SIZE(ADAPTER, MaxFrameSize),  1514,   100,    9014},
{"CrcAddedBySW",    0,  MP_OFFSET(ADAPTER, CrcAddedBySW),  MP_SIZE(ADAPTER, CrcAddedBySW),  0,      0,      1},
{"VlanStrip",       0,  MP_OFFSET(ADAPTER, VlanStrip),     MP_SIZE(ADAPTER, VlanStrip),     1,      0,      1},
{"Padding",         0,  MP_OFFSET(ADAPTER, Padding),       MP_SIZE(ADAPTER, Padding),       1,      0,      1},
{"enTxFC",          0,  MP_OFFSET(ADAPTER, enTxFC),        MP_SIZE(ADAPTER, enTxFC),        1,      0,      1},
{"enRxFC",          0,  MP_OFFSET(ADAPTER, enRxFC),        MP_SIZE(ADAPTER, enRxFC),        1,      0,      1},
{"enAllMulti",      0,  MP_OFFSET(ADAPTER, enAllMulti),    MP_SIZE(ADAPTER, enAllMulti),    0,      0,      1},
{"enDbgMode",       0,  MP_OFFSET(ADAPTER, enDbgMode),     MP_SIZE(ADAPTER, enDbgMode),     0,      0,      1},
//{"enRCInt",         0,  MP_OFFSET(ADAPTER, enRCInt),       MP_SIZE(ADAPTER, enRCInt),       0,      0,      1},
//{"rxchkCRC",        0,  MP_OFFSET(ADAPTER, rxchkCRC),      MP_SIZE(ADAPTER, rxchkCRC),      1,      0,      1},
{"rxchkRSS",        0,  MP_OFFSET(ADAPTER, rxchkRSS),      MP_SIZE(ADAPTER, rxchkRSS),      0,      0,      1},
//{"rxchkIPPayXsum",  0,  MP_OFFSET(ADAPTER, rxchkIPPayXsum),MP_SIZE(ADAPTER, rxchkIPPayXsum),0,      0,      1},
//{"enCutThru",       0,  MP_OFFSET(ADAPTER, enCutThru),     MP_SIZE(ADAPTER, enCutThru),     0,      0,      1},
//{"NipToQx",         0,  MP_OFFSET(ADAPTER, NipToQx),       MP_SIZE(ADAPTER, NipToQx),       0,      0,      1},
};

void AdapterDefault(PADAPTER pAdapter, char* paramFile)
{
    //pAdapter->TpdRingSize    = TX_RING_SZ_DEF;
    //pAdapter->RxRingSize     = 4080;         //RX_RING_SZ_DEF; RFD&RRD Ring Size
    //pAdapter->RxBufBlockSize = RX_BUFBLOCK_SZ_DEF;   // RxBufBlockSize * MAX_PKT_BUF > MaxFrameSize
    //pAdapter->TxBufSize      = TX_BUF_SZ_DEF;
    
    //pAdapter->AutoNeg        = true;
    //pAdapter->NipToQx        = false; //TRUE;

    pAdapter->MaxFrameSize   = MTU_DEF;
    //pAdapter->MaxIntPerSec   = MAX_INTNUM;
    
    pAdapter->CrcAddedBySW   = false;
    pAdapter->VlanStrip      = true;
    pAdapter->Padding        = true;
    pAdapter->enBC           = true;
    pAdapter->enTxFC         = true;
    pAdapter->enRxFC         = true;
    pAdapter->enAllMulti     = true;//FALSE;

    //pAdapter->enRCInt        = false;
    pAdapter->enDbgMode      = false;//TRUE;

    // rx verify
    //pAdapter->rxchkCRC       = true; //FALSE;
    pAdapter->rxchkRSS       = false;//TRUE;

    pAdapter->lldpChkAddr     = 0;

    /* get settings from external file "p.ini" */
    int drvParamsCount = sizeof(drvParams)/sizeof(drvParams[0]);
    GetParamFromFile((u8*)pAdapter, paramFile, drvParams, drvParamsCount);

}
void* RegisterNIC(PATHR_DOSAPP_CHAR Char)
{
    PADAPTER pAdapter = &ADPT;

    strcpy((char *)Char->NicName, "fuxi");
    Char->DrvVer     = 0x0100000B;

    Char->Open       = Open_Linux;
    Char->Close      = Close_Linux;
    Char->Load       = Load_Linux;
    Char->Unload     = Unload_Linux;
    Char->Reset      = Reset_Linux;
    Char->Stats      = GetStats_Linux;
    Char->Send       = Send_Linux;
    Char->Receive    = Receive_Linux;
    Char->ReturnPkts = ReturnPackets_Linux;
    Char->IoCtrl     = IoCtrl_Linux;
    Char->EnInt      = EnableInterrupt_Linux;

    memset(pAdapter, 0, sizeof(pAdapter));

    AdapterDefault(pAdapter, (char*)Char->InitFileName);

    return pAdapter;
}


int get_interface_names_by_proc(const char *dev_file, char (*if_name)[IFNAMSIZ])
{
    int   net_inf_count = 0;
    int   alloc_size = 4096;
    char  *buf = NULL;
    char  *p, *head, *tail;
    int   fd, read_count;

    fd = open(dev_file, O_RDONLY);
    if(fd < 0) {
        return -11;
    }

    while(1) {
        buf = (char*)realloc(buf, alloc_size);
        if(buf == NULL) {
            close(fd);
            return -12;
        }
        lseek(fd, 0, SEEK_SET);
        read_count = read(fd, buf, alloc_size);
        if(read_count <= 0) {
            free(buf);
            close(fd);
            return -13;
        }
        if(read_count < alloc_size)
            break;
        alloc_size <<= 1;
    }
    buf[read_count] = 0;  // to terminate the string

    head = buf;
    while(head < buf + read_count) {
        p = head;
        while((*p!='\n' && *p!='\r') && (p < buf + read_count))
            p++;
	while((*p=='\n' || *p=='\r') && (p < buf + read_count))
            *p++ = 0;
        tail = p;

        p = strchr(head, ':');
        if(p) {
            // a line containing a interface device:
            *p = 0;
            while(isspace(*head) && (head < buf + read_count))
                head++;
            strncpy(if_name[net_inf_count], head, IFNAMSIZ);
            net_inf_count++;
	    if (net_inf_count == MAX_DEVICE_COUNT)
	    	break;
        }
        head = tail;
    }
    free(buf);
    close(fd);
    return net_inf_count;
}


/* Input:
        bListAdapter == TRUE, list all supported adapters according to the supporting list
        bListAdapter == FALSE, find the 1st supported adapter and use it as default one.
     Return:
        0 - Found device
        Non-Zero - No NIC is found
*/
u32  find_list_adapters(IN bool bListAdapter, OUT PAPP_ADAPTER pAppAdapter)
{
    int ret = STATUS_SUCCESSFUL;
    int i, j;
    u32 ven_id;
    u32 dev_id;
    u32 did_vid;
    RW_REG rwReg;
    char dev_name[64] = "netdev_ops";
    int founds = 0;
    int ndevs = sizeof(dev_list)/sizeof(dev_list[0]);

    char if_name[MAX_DEVICE_COUNT][IFNAMSIZ];
    int  if_count = 0;
    
    /* Create debugfs node */
    if (STATUS_SUCCESSFUL != gAthrDosappChar.Open(dev_name)) {
        ret = STATUS_FAILED;
        return ret;
    }

    /* close the debugfs */
    gAthrDosappChar.Close();

    // if we are searching the 1st NIC adapter, not listing adapters
    if (bListAdapter == false && pAppAdapter->SpecifiedByUser == false)
    {
        //printf("Found device: %x\t\t\n", dev_list[0].dev_id);
        pAppAdapter->DevId = dev_list[0].dev_id;
        strcpy(pAppAdapter->DevName, dev_name);
    }
    else if (bListAdapter == false && pAppAdapter->SpecifiedByUser == true)
    {
        printf("Found device: %x\t\t", dev_list[0].dev_id);
        pAppAdapter->DevId = dev_list[0].dev_id;
        strcpy(pAppAdapter->DevName, dev_name);
    }
    else
    {
        printf("%s", "Index\tName");
        printf("%64s\n", "DevID VenID");
        printf("%d", founds);
        printf("\t%s", "Ethernet Adapter");
        printf(" %-40s", dev_name);
        printf("%04X  %04X\n",  
            dev_list[i].dev_id,
            dev_list[i].ven_id);
    }
    return  ret; 
}

// determine which adapter to test 
// either specified by user or by the 1st supported adapter during scanning
int determineAdapter(IN PAPP_ADAPTER pAppAdapter)
{
    int ret;
    u16 devId;

    // if the user doesn't specify a NIC, find the 1st supported adapter
    ret = find_list_adapters(false, &gAppAdapter);
    if (ret != STATUS_SUCCESSFUL)
    {
        return ret;
    }

    devId = pAppAdapter->DevId;
    if (devId & 1)
    {
        pAppAdapter->NicType |= NICTYPE_GIGA;
    }
    else
    {
        pAppAdapter->NicType |= NICTYPE_FAST;
    }
    return STATUS_SUCCESSFUL;
}

// handle the commands parsed from input
// Input: pAppAdapter->BusNum/DevNum/FunNum - from which NIC to get MAC address
// Output: pAppAdapter->MacAddr[6] 
int getMacAddr(IN OUT PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    char    paraStr[] = "MacAddr";

    nicPos = pAppAdapter->DevId;

    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_NORMAL);
    if (gPDriver == NULL) {
        return ERR_FAILED_LOAD_DRIVER;
    }

    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        paraStr, sizeof(paraStr), pAppAdapter->MacAddr, 6);
    if (ret != 0) {
        return ret; 
    }

    if(!(pAppAdapter->MacAddr[0] | pAppAdapter->MacAddr[1] | pAppAdapter->MacAddr[2]
        | pAppAdapter->MacAddr[3] | pAppAdapter->MacAddr[4] | pAppAdapter->MacAddr[5])){
        pAppAdapter->MacAddr[0] = 0x00;
        pAppAdapter->MacAddr[1] = 0x55;
        pAppAdapter->MacAddr[2] = 0x7b;
        pAppAdapter->MacAddr[3] = 0xb5;
        pAppAdapter->MacAddr[4] = 0x7d;
        pAppAdapter->MacAddr[5] = 0xf7;
    }

#if 0
    memcpy(pAppAdapter->MacAddrDst, pAppAdapter->MacAddr, 6);
#else
    pAppAdapter->MacAddrDst[0] = 0xff;
    pAppAdapter->MacAddrDst[1] = 0xff;
    pAppAdapter->MacAddrDst[2] = 0xff;
    pAppAdapter->MacAddrDst[3] = 0xff;
    pAppAdapter->MacAddrDst[4] = 0xff;
    pAppAdapter->MacAddrDst[5] = 0xff;
#endif
#if 0
    printf("MAC-address: %02X-%02X-%02X-%02X-%02X-%02X\n",
            pAppAdapter->MacAddr[0], pAppAdapter->MacAddr[1], pAppAdapter->MacAddr[2], 
            pAppAdapter->MacAddr[3], pAppAdapter->MacAddr[4], pAppAdapter->MacAddr[5]);
#endif
    return gAthrDosappChar.Unload(gPDriver);
}

/*
   Get some global hw parameters that affect the HW globally, not packet by packet
*/
int getGlobalHwParas(IN OUT PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    char    vlanStripStr[] = "VlanStrip";
    char    mtuStr[] = "MTU";
    char    lldpChkAddrStr[] = "LldpChkAddr";
    char    RxRingSizeStr[] = "RxRingSize";
    char    RxBufBlockSizeStr[] = "RxBufBlockSize";

    nicPos = pAppAdapter->DevId;

    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_NORMAL);
    if (gPDriver == NULL) {
        return ERR_FAILED_LOAD_DRIVER;
    }

    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        vlanStripStr, sizeof(vlanStripStr), &pAppAdapter->bVlanStrip, 1);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        mtuStr, sizeof(mtuStr), &pAppAdapter->MTU, 2);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        lldpChkAddrStr, sizeof(lldpChkAddrStr), &pAppAdapter->bLldpChkAddr, 1);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        RxRingSizeStr, sizeof(RxRingSizeStr), &pAppAdapter->RxRingSize, 2);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }

    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        RxBufBlockSizeStr, sizeof(RxBufBlockSizeStr), &pAppAdapter->RxBufBlockSize, 2);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    
    return gAthrDosappChar.Unload(gPDriver);
}

struct param_entry appParams[] ={
{"OneByOne",        0,  MP_OFFSET(APP_ADAPTER, bOneByOne),     MP_SIZE(APP_ADAPTER, bOneByOne),     0,      0,      1},
{"LinkCapability",  1,  MP_OFFSET(APP_ADAPTER, LinkCapability),MP_SIZE(APP_ADAPTER, LinkCapability),8,      1,      0xFF},  
{"RSSType",         1,  MP_OFFSET(APP_ADAPTER, RssType),       MP_SIZE(APP_ADAPTER, RssType),       0xF,    0,      0xF},
{"BaseCpuNum",      0,  MP_OFFSET(APP_ADAPTER, BaseCpuNum),    MP_SIZE(APP_ADAPTER, BaseCpuNum),    0,      0,      3},
{"txFrameType",     1,  MP_OFFSET(APP_ADAPTER, enPacketType),  MP_SIZE(APP_ADAPTER, enPacketType),  0xFF,   0,      0xFF},
};

// allocate enough memory for app to use
int appHwPara(IN PAPP_ADAPTER pAppAdapter)
{
    int ret;
    int appParamCount;
    
    ret = gAthrDosappChar.Open(gAppAdapter.DevName);
    if (ret != STATUS_SUCCESSFUL)
    {
        printf("\nFailed to open device...\n");
        return ERR_FAILED_LOAD_DRIVER;
    }

    if ((ret = getMacAddr(&gAppAdapter)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get MAC address.\n");
        return ret;
    }

    if ((ret = getGlobalHwParas(&gAppAdapter)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get some HW parameters.\n");
        return ret;
    }
    gAthrDosappChar.Close();

    // Default App parameters
    pAppAdapter->LinkCapability = LC_TYPE_10F|LC_TYPE_100F;
    if (pAppAdapter->DevId & 1)
    {
        pAppAdapter->LinkCapability |= LC_TYPE_1000F;
    }
    pAppAdapter->RssType = RSS_TYPE_IPV4 | RSS_TYPE_IPV4_TCP |
                           RSS_TYPE_IPV6 | RSS_TYPE_IPV6_TCP;
    pAppAdapter->BaseCpuNum = 0;
    pAppAdapter->enPacketType = 0xFF;

    if ((pAppAdapter->NicType & NICTYPE_L1F) &&
        (pAppAdapter->RevId & 0xF8) == 0x10)    // L1f-B0
    {
        pAppAdapter->bIpXsumBug = false;
    }
    else
    {
        pAppAdapter->bIpXsumBug = true;
    }

    if (!pAppAdapter->LoopCount)
    {
        pAppAdapter->LoopCount = gFuxiLpPacketsNum;
    }

    //appParamCount = sizeof(appParams)/sizeof(appParams[0]);
    //GetParamFromFile((u8*)pAppAdapter, pAppAdapter->pInitFileName, appParams, appParamCount);

    return STATUS_SUCCESSFUL;
}

void genPacketType(PGENERATE_PKT_TYPE pPktType, u16 enPacketType)
{
    // if any input is a random, generate it
    if (pPktType->vlan_type == vlan_type_random)
    {
        pPktType->vlan_type = (enum VLAN_TYPE)(RAND() % vlan_type_random);
    }

    if (pPktType->frame_type == frame_type_random)
    {
        pPktType->frame_type = (enum FRAME_TYPE)(RAND() % frame_type_random);

        //printf("ft %d=>", pPktType->frame_type);
        if ((pPktType->frame_type == frame_type_ethernetII) &&
            (enPacketType & TX_PKT_TYPE_II) == 0)
        {
            pPktType->frame_type = frame_type_802_3_snap;
        }
        if ((pPktType->frame_type == frame_type_802_3_snap) &&
            (enPacketType & TX_PKT_TYPE_SNAP) == 0)
        {
            pPktType->frame_type = frame_type_802_3;
        }
        if ((pPktType->frame_type == frame_type_802_3) &&
            (enPacketType & TX_PKT_TYPE_802_3) == 0)
        {
            if (enPacketType & frame_type_ethernetII)
            {
                pPktType->frame_type = frame_type_ethernetII;
            }
            else
            {
                pPktType->frame_type = frame_type_802_3_snap;
            }
        }
        //printf("%d\t", pPktType->frame_type);
    }

    if (pPktType->frame_type == frame_type_IIorSnap)
    {
        pPktType->frame_type = (enum FRAME_TYPE)(RAND() % 2);

        if ((pPktType->frame_type == frame_type_ethernetII) &&
            (enPacketType & TX_PKT_TYPE_II) == 0)
        {
            pPktType->frame_type = frame_type_802_3_snap;
        }
        if ((pPktType->frame_type == frame_type_802_3_snap) &&
            (enPacketType & TX_PKT_TYPE_SNAP) == 0)
        {
            pPktType->frame_type = frame_type_ethernetII;
        }
    }

    if (pPktType->l3_type == l3_type_random)
    {
        pPktType->l3_type = (enum L3_TYPE)(RAND() % l3_type_random);

        if ((pPktType->l3_type == l3_type_ipv4) &&
            (enPacketType & TX_PKT_TYPE_IPV4) == 0)
        {
            pPktType->l3_type = l3_type_ipv6;
        }
        if ((pPktType->l3_type == l3_type_ipv6) &&
            (enPacketType & TX_PKT_TYPE_IPV6) == 0)
        {
            pPktType->l3_type = l3_type_ipv4;
        }
    }

    if (pPktType->l4_type == l4_type_random)
    {
        pPktType->l4_type = (enum L4_TYPE)(RAND() % l4_type_random);

        if ((pPktType->l4_type == l4_type_tcp) &&
            (enPacketType & TX_PKT_TYPE_TCP) == 0)
        {
            pPktType->l4_type = l4_type_udp;
        }
        if ((pPktType->l4_type == l4_type_udp) &&
            (enPacketType & TX_PKT_TYPE_UDP) == 0)
        {
            pPktType->l4_type = l4_type_tcp;
        }
    }
}

void genIpAddr(PAPP_ADAPTER pAppAdapter, PGENERATE_PKT_TYPE pPktType)
{
    // generate IP address
    if (pPktType->l3_type == l3_type_ipv4)
    {
        pAppAdapter->IpAddr[0] = 192;
        pAppAdapter->IpAddr[1] = 168;
        //pAppAdapter->IpAddr[2] = (u8)RAND();
        //pAppAdapter->IpAddr[3] = (u8)RAND();
        pAppAdapter->IpAddr[2] = 0;
        pAppAdapter->IpAddr[3] = 202;

        pAppAdapter->IpAddrDst[0] = 192;
        pAppAdapter->IpAddrDst[1] = 168;
        //pAppAdapter->IpAddrDst[2] = (u8)RAND();
        //pAppAdapter->IpAddrDst[3] = (u8)RAND();
        pAppAdapter->IpAddrDst[2] = 0;
        pAppAdapter->IpAddrDst[3] = 201;
    }
    else if (pPktType->l3_type == l3_type_ipv6)
    {
        pAppAdapter->IpAddr[12] = 192;
        pAppAdapter->IpAddr[13] = 168;
        pAppAdapter->IpAddr[14] = (u8)RAND();
        pAppAdapter->IpAddr[15] = (u8)RAND();

        pAppAdapter->IpAddrDst[12] = 192;
        pAppAdapter->IpAddrDst[13] = 168;
        pAppAdapter->IpAddrDst[14] = (u8)RAND();
        pAppAdapter->IpAddrDst[15] = (u8)RAND();
    }
}

u16 genPktLen(u16 minPktLen, u16 maxPktLen)
{
    u16 randRange, randPktLen;

    randRange = maxPktLen - minPktLen;
    randPktLen = (u16) RAND();
    randPktLen %= (randRange + 1);  // [0, randRange]
    randPktLen = randPktLen + minPktLen;

    return randPktLen;
}

void fillPacket(u8 *buf, u32 len, bool bChanged)
{
    u32 i;

#ifdef FILL_DWORD
    static u32 pktFill = 0x01;    //For DBG only

    u32 *pu32 = NULL;
    // the remainder less than 4 bytes will not be handled
    if (len >= 4)  
    {
        for (i = 0; i <= len - 4; i += 4)
        {
            pu32 = (u32 *) (buf+i);
            *pu32 = htonl(pktFill);
            if (bChanged) pktFill++;
        }
        for (; i<len; i++)
        {
            *(buf+i) = 0x11;
        }
    }
    if (!bChanged)
    {
        pktFill++;
    }

#else
    static u8 pktFill = 0x01;

    for (i = 0; i < len; i++)
    {
        //*(p+i) = (u8)i;
        *(buf+i) = (u8)(pktFill + i);
    }
    pktFill++;
#endif
}

// generate a random packet according to the input packet type requirement
// by default, the xsum for ipv4/tcp/udp will be calculated
// if input denotes a incorrect xsum, then ~xsum
int generateRandomPacket(IN PAPP_ADAPTER pAppAdapter,
                               IN PGENERATE_PKT_TYPE pPacketType,
                               IN u8* pPktBuf,
                               OUT PATHR_DOS_PACKET *ppDosPacket)
{
    uint16_t maxPktLen, minPktLen, randPktLen;
    uint16_t ipPlLen = 0;     // Ip payload length, ip header not included
    uint16_t l4PlLen = 0;     // tcp/udp payload length, tcp/udp header not included
    uint8_t  protocol = 0;    // Protocol filed of Ipv4 / ipv6          
    uint16_t i;
    uint16_t mss;
    uint32_t hashVal = 0;
    bool tso = false;
    uint8_t* pFrameLen = NULL;
    uint8_t* pIpHdr, * pL4Hdr = NULL;
    uint8_t* p = pPktBuf;
    PATHR_DOS_PACKET pPacket;
    uint16_t tcpopt_len, ipopt_len, ipext_len, ipext_len_adjusted;
    bool ip_frag, doRss;
    uint16_t MTU;

    randPktLen = 0;
    ip_frag = pPacketType->flag & PKT_FLAG_FRAGMENT ? true : false;
    doRss = pPacketType->flag & PKT_FLAG_CALC_RSS ? true : false;

    // allocate a ATHR_DOS_PACKET to be returned to the caller
    pPacket = (PATHR_DOS_PACKET)malloc(sizeof(ATHR_DOS_PACKET));
    memset(pPacket, 0, sizeof(ATHR_DOS_PACKET));
    *ppDosPacket = pPacket;

    genPacketType(pPacketType, pAppAdapter->enPacketType);
    if (doRss)
    {
        genIpAddr(pAppAdapter, pPacketType);
    }

    // set the TYPEs of generated packet according to the input flag
    if (pPacketType->flag & PKT_FLAG_TO_CXSUM)
        pPacket->Type |= PKT_TYPE_TO_CXSUM;

    if (pPacketType->flag & PKT_FLAG_TO_IPXSUM)
        pPacket->Type |= PKT_TYPE_TO_IPXSUM;

    if (pPacketType->flag & PKT_FLAG_TO_L4XSUM)
        pPacket->Type |= PKT_TYPE_TO_L4XSUM;

    if (pPacketType->flag & PKT_FLAG_TO_TSOV1)
    {
        pPacket->Type |= PKT_TYPE_TO_TSOV1;
        tso = true;
    }

    if (pPacketType->flag & PKT_FLAG_TO_TSOV2)
    {
        pPacket->Type |= PKT_TYPE_TO_TSOV2;
        tso = true;
    }

    if (pPacketType->flag & PKT_FLAG_IPXSUM_ERR)
        pPacket->Type |= PKT_TYPE_IPXSUM_ERR;

    if (pPacketType->flag & PKT_FLAG_L4XSUM_ERR)
        pPacket->Type |= PKT_TYPE_L4XSUM_ERR;

    // generate random option length
    if (pPacketType->flag & PKT_FLAG_PTP_OVER_UDP)
    {
        ipopt_len = 0;
    }
    else
    {
        if (pAppAdapter->bIpXsumBug == true)
        {
            ipopt_len = 0;		// for HW bug
        }
        else
        {
            ipopt_len = ((uint16_t)RAND()) % (40 + 1);  //ip v4 option
            ipopt_len &= 0xFFFC;
        }
    }

    if (tso) {
        tcpopt_len = 0x4;
    }
    else
    {
        tcpopt_len = 0;
    }

    // generate a random length for ipv6 extension header, if ip_frag is set
    // the minimum length of if fragmentation header is 16bytes
    while (1)
    {
        ipext_len = ((uint16_t)RAND()) % 121;    // ip v6 extension header
        ipext_len &= 0xFFF8;
        if (ip_frag) // if ip frag, at least 16 bytes
        {
            if (ipext_len >= 0x10)
                break;
        }
        else    // if no ip frag, any random length would be ok
        {
            break;
        }
    }

    //-----------------------------------------------------------------------------
    MTU = pAppAdapter->MTU;
    if (pAppAdapter->bXsumBug)      // patch bug: let maxPktLen < 2K
    {
        while (MTU > 2000)
        {
            MTU -= 100;
        }
    }

    maxPktLen = 1514;   // mac header 14 + max data 1500, do not count 4bytes FCS in
    minPktLen = 14;     // at minimum mac header 14 bytes

    if (pPacketType->flag & PKT_FLAG_TO_CXSUM)
    {
        minPktLen += 2; // if do custom checksum, need at least two more bytes
    }

    if (pPacketType->frame_type == frame_type_ethernetII)
    {
        maxPktLen = MTU;
    }
    // if doing large send and the generated packet is EII type, generate some large packets
    // for 802.3 SNAP packet, it still cannot be exceed 1514 bytes per packet.
    if (tso && (pPacketType->frame_type == frame_type_ethernetII))
    {
        //maxPktLen = 65000;  // 64K
        maxPktLen = 7900;  // 8K
    }
    // step0: generate the mac address field
    // set the source MAC address and destination MAC address field
#if 0
    memcpy(p, pAppAdapter->MacAddrDst, sizeof(pAppAdapter->MacAddrDst));
    p += 6;
    memcpy(p, pAppAdapter->MacAddr, sizeof(pAppAdapter->MacAddr));
    p += 6;
#else
    for(int j = 0; j < 6; j++)
    {
        p[j] = pAppAdapter->MacAddrDst[5 - j];
    }
    p += 6;

    for(int j = 0; j < 6; j++)
    {
        p[j] = pAppAdapter->MacAddr[5 - j];
    }
    p += 6;
#endif
    // generate the contents of packet, there is no random value by now
    // step1: vlan if neccessary
    if (pPacketType->vlan_type == vlan_type_insert)
    {
        pPacket->Type |= PKT_TYPE_TO_VLANINST;
        pPacket->VlanID = (uint16_t)RAND();
    }
    if (pPacketType->vlan_type == vlan_type_tagged)
    {
        pPacket->Type |= PKT_TYPE_VLANTAGGED;
        pPacket->VlanID = (uint16_t)RAND();
        *(uint16_t*)p = htons(0x8100);
        p += 2;
        *(uint16_t*)p = htons(pPacket->VlanID);
        p += 2;
        minPktLen += 4;
    }

    // step2:  construct the packet according to the frame type
    if (pPacketType->frame_type == frame_type_802_3)
    {
        uint16_t u16Len;
        pPacket->Type |= PKT_TYPE_802_3;

        randPktLen = genPktLen(minPktLen, maxPktLen);

        // fill packet length field
        // if the packet is vlan tagged already, extra 4 bytes should be subtracted
        u16Len = randPktLen - 14;       // subtract the MAC header 
        if (pPacketType->vlan_type == vlan_type_tagged)
        {
            u16Len -= 4;
        }
        *(uint16_t*)p = htons(u16Len);
        p += 2;

        fillPacket(p, randPktLen - minPktLen, false);
    }
    else // Ethernet II & SNAP
    {
        if (pPacketType->frame_type == frame_type_ethernetII)
        {
            pPacket->Type |= PKT_TYPE_EII;
        }

        if (pPacketType->frame_type == frame_type_802_3_snap)
        {
            pPacket->Type |= PKT_TYPE_SNAP;

            pFrameLen = p;      // save the location of Length field, to be adjusted at last
            p += 2;
            minPktLen += 8;
            *p++ = 0xAA; *p++ = 0xAA; *p++ = 0x03; // snap flag
            *p++ = 0x00; *p++ = 0x13; *p++ = 0x74; // attansic's OUI
        }

        // the Ethernet II and SNAP are basically same
        if (pPacketType->l3_type == l3_type_none)
        {
            uint16_t proto;
            proto = (uint16_t)RAND();
            // get rid of some special protocal type
            if (proto < 0x0600)
                proto += 0x0600;
            if (proto == 0x8100)    // vlan
                proto = 0x8104;
            if (proto == 0x0800 || proto == 0x86DD)
                proto = 0x8105;
            if (proto == 0x8808)    // control frame
                proto = 0x8807;
            if (proto == 0x88F7)    // PTP
                proto = 0x88F6;
            if (proto == 0x88CC)    // LLDP
                proto = 0x88CD;

            if (pPacketType->flag & (PKT_FLAG_PTP_OVER_MAC | PKT_FLAG_LLDP))
                proto = pAppAdapter->MacProtocol;

            *(uint16_t*)p = htons(proto);
            p += 2;

            randPktLen = genPktLen(minPktLen, maxPktLen);

            fillPacket(p, randPktLen - minPktLen, false);
        }
        else    // ipv4 or ipv6 in layer 3
        {
            uint8_t* srcIP, * destIP;
            srcIP = pAppAdapter->IpAddr;
            destIP = pAppAdapter->IpAddrDst;

            // determine the value of protocol field used in ip header
            if (pPacketType->l4_type == l4_type_tcp)
            {
                pPacket->Type |= PKT_TYPE_TCP;
                protocol = 6;
            }
            else if (pPacketType->l4_type == l4_type_udp)
            {
                pPacket->Type |= PKT_TYPE_UDP;
                protocol = 17;
            }
            else if (pPacketType->l4_type == l4_type_none)
            {
                // should  0x3D - Any host internal protocol be used here
                if (pPacketType->l3_type == l3_type_ipv4)
                {
                    protocol = 0x88;    // = UDP lite, copyied from old project ?????
                }
                else if (pPacketType->l3_type == l3_type_ipv6)
                {
                    protocol = EXT_NONE;    // ipv6 no next header for ipv6
                }
            }


            // pre-calcuate the tcp/udp header length in level 4, only exists when no fragmentation
            ipPlLen = 0;
            if (ip_frag == false)   // if no fragment, then tcp/udp header exists
            {
                if (pPacketType->l4_type == l4_type_tcp)
                {
                    ipPlLen = sizeof(TCPHeader) + tcpopt_len;
                    //minPktLen = minPktLen + sizeof(TCPHeader) + tcpopt_len;
                }
                if (pPacketType->l4_type == l4_type_udp)
                {
                    ipPlLen = sizeof(UDPHeader);
                    //minPktLen = minPktLen + sizeof(UDPHeader);
                }
                minPktLen += ipPlLen;
            }


            // build ipv4 header
            if (pPacketType->l3_type == l3_type_ipv4)
            {
                IPHeader* iph;
                pPacket->Type |= PKT_TYPE_IPV4;

                hashVal = calHashIpv4(srcIP, destIP);

                minPktLen += sizeof(IPHeader) + ipopt_len;
                if (tso) 
                    randPktLen = genPktLen(1600, maxPktLen);
                else
                    randPktLen = genPktLen(minPktLen, maxPktLen);

                if (randPktLen < 60)
                    randPktLen = 60;

                // When doing TSO, 0 payload length of TCP packet is not allowed
                if (tso)
                {
                    if (randPktLen - minPktLen == 0)
                        randPktLen++;
                }

                l4PlLen = randPktLen - minPktLen;
                ipPlLen += l4PlLen;

                *(uint16_t*)p = htons(0x0800);
                p += 2;
                pIpHdr = p; // points to Ip header
                
                p += build_ipv4_header(
                    ipPlLen + sizeof(IPHeader) + ipopt_len, // total_Length
                    (uint8_t)ipopt_len,
                    protocol,
                    srcIP, destIP,
                    p,
                    ip_frag,
                    1);
                pL4Hdr = p; // points to tcp/udp header

                iph = (IPHeader*)pIpHdr;
                if (pPacketType->flag & PKT_FLAG_PTP_OVER_UDP)
                {
                    iph->iph_ttl = 1;
                }
                // calculate ipv4 header xsum
                iph->iph_xsum = 0;
                iph->iph_xsum = ComputeChecksum(pIpHdr, (iph->iph_verlen & 0xf) * 4, 0);

                // store the correct IPv4 xsum to UpLevelReserved[0], and clear the xsum field of ip hdr
                // UINT16 -> UINT32 -> PVOID, to avoid compile warning with a direct cast from UINT16 to PVOID
                pPacket->UpLevelReserved[0] = (void*)((unsigned long)(iph->iph_xsum));
                //iph->iph_xsum = 0;
                if (pPacketType->flag & PKT_FLAG_TO_IPXSUM)
                    iph->iph_xsum = 0;
                else if (pPacketType->flag & PKT_FLAG_IPXSUM_ERR)
                    iph->iph_xsum ^= -1;     // bitwise NOT iph->iph_xsum;
                //print("pPcacketType->flag is %d, iph->iph_xsum is %x \n", pPacketType->flag, iph->iph_xsum, color_yellow);
            }

            // build ipv6 header            
            if (pPacketType->l3_type == l3_type_ipv6)
            {
                pPacket->Type |= PKT_TYPE_IPV6;

                hashVal = calHashIpv6(srcIP, destIP);

                minPktLen = minPktLen + sizeof(IPV6Header) + ipext_len;
                randPktLen = genPktLen(minPktLen, maxPktLen);

                // When doing TSO, 0 payload length of TCP packet is not allowed
                if (tso)
                {
                    if (randPktLen - minPktLen == 0)
                        randPktLen++;
                }

                l4PlLen = randPktLen - minPktLen;
                ipPlLen += l4PlLen;

                *(uint16_t*)p = htons(0x86DD);
                p += 2;
                pIpHdr = p; // points to Ip header

                // build ipv6 header
                ipext_len_adjusted = (uint16_t)build_ipv6_header(
                    protocol,
                    srcIP,
                    destIP,
                    p,
                    ip_frag,
                    ipext_len);

                minPktLen -= (ipext_len - ipext_len_adjusted);
                randPktLen -= (ipext_len - ipext_len_adjusted);

                // extension headers also count
                ((IPV6Header*)p)->paylen = htons(ipPlLen + ipext_len_adjusted);
                p += (sizeof(IPV6Header) + ipext_len_adjusted);
                pL4Hdr = p; // points to tcp/udp header
            }

            if (ip_frag == false)   // if no fragment, then tcp/udp header exists
            {
                uint16_t  src_port, des_port;
                //src_port = (uint16_t)RAND();
                //des_port = (uint16_t)RAND();
                src_port = 138;
                des_port = 138;
                if (des_port == 319 || des_port == 320) // PTP
                {
                    des_port = 321;
                }
                if (pPacketType->flag & PKT_FLAG_PTP_OVER_UDP)
                {
                    des_port = pAppAdapter->Port;
                }

                if (pPacketType->l4_type == l4_type_tcp)
                {
                    // mss is only meaningful when it's a tcp packet
                    uint16_t maxMssLen = 0;
                    uint16_t minMssLen = 0;
                    if (pPacketType->frame_type == frame_type_802_3_snap)
                    {
                        maxMssLen = MAX_MSS_SNAP;
                        minMssLen = MIN_MSS_SNAP;
                    }
                    if (pPacketType->frame_type == frame_type_ethernetII)
                    {
                        maxMssLen = MTU - 250;
                        minMssLen = MIN_MSS_EII;
                    }
                    mss = ((uint16_t)RAND()) % (maxMssLen - minMssLen + 1);
                    mss += minMssLen;
                    //mss = 1000; //DBG only
                    if (mss > randPktLen - minPktLen)     // dbg
                        mss = randPktLen - minPktLen;
                    pPacket->MSS = mss;
                    if (pPacketType->l3_type == l3_type_ipv4) {
                        hashVal = calHashIpv4Tcp(srcIP, destIP, src_port, des_port);
                    }
                    else if (pPacketType->l3_type == l3_type_ipv6) {
                        hashVal = calHashIpv6Tcp(srcIP, destIP, src_port, des_port);
                    }

                    uint8_t tcpFlag;
                    if (tso) {
                        tcpFlag = TCP_CTRL_SYN;
                    }
                    else {
                        //tcpFlag = TCP_FLAGS[RAND() % (sizeof(TCP_FLAGS))];
                        tcpFlag = TCP_CTRL_FIN;
                    }
                    //printf("FXG: tcpopt_len is %d\n", tcpopt_len);
                    build_tcp_header(
                        (uint8_t)tcpopt_len,
                        src_port,
                        des_port,
                        (uint32_t)RAND(), // seqNum
                        (uint32_t)RAND(), // ackNum
                        tcpFlag,
                        l4PlLen,
                        0x28,
                        srcIP,
                        destIP,
                        ipPlLen,
                        mss,
                        tso,
                        p);

                    if (pPacketType->l3_type == l3_type_ipv6) { // ipv6
                        TCPHeader* h = (TCPHeader*)p;
                        if (tso)
                            h->tcp_xsum = ~ComputePsduXsum2(srcIP, destIP, 6, 0);
                        else
                            h->tcp_xsum = ~ComputePsduXsum2(srcIP, destIP, 6, ipPlLen);
                    }

                    p += sizeof(TCPHeader) + tcpopt_len;
                }

                if (pPacketType->l4_type == l4_type_udp)
                {
                    build_udp_header(
                        src_port,
                        des_port,
                        ipPlLen,
                        srcIP,
                        destIP,
                        p);

                    if (pPacketType->l3_type == l3_type_ipv6) { // ipv6
                        UDPHeader* h = (UDPHeader*)p;
                        h->udp_xsum = ~ComputePsduXsum2(srcIP, destIP, 17, ipPlLen);
                    }

                    p += sizeof(UDPHeader);
                }
            }


            // now fill random data the the rest part in a packet  [...] means optional. 
            // if it's a frag packet, tcp/udp head will be omitted
            //     ipv4 hdr [options] | random data ......
            //     ipv6 hdr [ext hdr] | random data ......
            // else not a frag packet
            //     ipv4 hdr [options] | tcp hdr [options] | random data ......
            //     ipv6 hdr [ext hdr] | tcp hdr [options] | random data ......
            //     ipv4 hdr [options] | udp hdr | random data ......
            //     ipv6 hdr [ext hdr] | udp hdr | random data ......
            // minPktLen = ipv6 hdr [ext hdr]  OR ipv4 hdr [options]  + tcp hdr [options]  OR udp hdr
            // randPktLen = minPktLen + random data
            fillPacket(p, randPktLen - minPktLen, true);
            //printf("FXG: randPktLen is %d, fillLength is %d\n", randPktLen, randPktLen - minPktLen);
            if (ip_frag == false)   // calculate the xsum of tcp/udp header after the random data was filled
            {
                uint16_t l4Xsum = 0;
                l4Xsum = ComputeChecksum(pL4Hdr, ipPlLen, 0);
#if 0
                if(!l4Xsum)
                {
                    printf("FXG: Xsum cal error, ipPlLen is %d\n", ipPlLen);
                    if(ipPlLen)
                    {
                        printf("Head is :\n");
                        for(int i = 0; i < ipPlLen; i++)
                        {
                            printf("%2x ", pL4Hdr[i]);
                        }
                    }
                printf("\n");
                }
#endif
                // store the correct TCP/UDP xsum to UpLevelReserved[1]
                // UINT16 -> UINT32 -> PVOID, to avoid compile warning with a direct cast from UINT16 to PVOID
                pPacket->UpLevelReserved[1] = (void*)((unsigned long)l4Xsum);

                if (pPacketType->l4_type == l4_type_tcp)
                {
                    TCPHeader* ph = (TCPHeader*)pL4Hdr;
                    // if l4xsum not calculated by NIC, nor filled with a INCORRECT xsum
                    // should fill a correct l4 xsum here
                    if ((pPacketType->flag &
                        (PKT_FLAG_TO_L4XSUM | PKT_FLAG_L4XSUM_ERR | PKT_FLAG_TO_TSOV1 | PKT_FLAG_TO_TSOV2)) == 0)
                    {
                        ph->tcp_xsum = l4Xsum;
                    }
                    // if to fill an INCORRECT xsum intended
                    else if (pPacketType->flag & PKT_FLAG_L4XSUM_ERR)
                    {
                        ph->tcp_xsum = ~l4Xsum;
                    }
                    // or leave the pseudo xsum unmodified
                }

                if (pPacketType->l4_type == l4_type_udp)
                {
                    if (pL4Hdr != NULL) 
                    {
                        UDPHeader* ph = (UDPHeader*)pL4Hdr;
                        // if l4xsum not calculated by NIC, nor filled with a INCORRECT xsum
                        // should fill a correct l4 xsum here
                        if ((pPacketType->flag &
                            (PKT_FLAG_TO_L4XSUM | PKT_FLAG_L4XSUM_ERR)) == 0)
                        {
                            if(l4Xsum){
                                ph->udp_xsum = l4Xsum;
                            }
                            else
                            {
                                //printf("FXG: ~l4Xsum is %x, ph->udp_xsum is %X.\n", ~l4Xsum, ph->udp_xsum);
                                ph->udp_xsum = ~l4Xsum;
                            }
                        }
                        // if to fill an INCORRECT xsum intended
                        else if (pPacketType->flag & PKT_FLAG_L4XSUM_ERR)
                        {
                            ph->udp_xsum = ~l4Xsum;
                        }
                        // or leave the pseudo xsum unmodified
                    }
                }
            }

            if (doRss == true)
            {
                pPacket->Hash = hashVal;
                pPacket->CpuNum = rssGetCpuNum(hashVal, pAppAdapter->BaseCpuNum);
            }
        }

        // if 802.3 SNAP, adjust the length field, total length of a packet minus the length of 802.3 MAC header
        if (pPacketType->frame_type == frame_type_802_3_snap)
        {
            // if the packet is vlan tagged already, extra 4 bytes should be subtracted
            uint16_t u16Len = randPktLen - 14;       // subtract the MAC header 
            if (pPacketType->vlan_type == vlan_type_tagged) {
                u16Len -= 4;                // vlan tag
            }

            if (NULL != pFrameLen)
            {
                *(uint16_t*)pFrameLen = htons(u16Len);
            }
        }

    }

    // if this packet is to verify Custom checksum function, generata random starting offset and
    // the offset to store the xsum value. Both offset must be aligned at half-word.
    // Custom xsum covers area [starting offset, end of packet]
    // do not place them at the 1st 14byte, which is MAC header, since this will never happen
    // in real environment
    if ((pPacketType->flag & PKT_FLAG_TO_CXSUM) &&
        (tso == false) /*&& (ip_frag == FALSE)*/)  // Does ip_frag relate to csum??
    {
        uint16_t cxTmp = 0;
        uint16_t cxOff[2];

        uint16_t csXsumSkipLen, csXsumBaseLen, csXsumMaxOff;
        if ((pPacketType->frame_type == frame_type_ethernetII) ||
            (pPacketType->frame_type == frame_type_802_3))
        {
            csXsumSkipLen = 15;
            csXsumBaseLen = 14;
        }
        else // frame type SNAP
        {
            csXsumSkipLen = 15 + 8; // 8 Bytes of SNAP header
            csXsumBaseLen = 14 + 8;
        }
        csXsumMaxOff = (randPktLen > 512) ? 512 : randPktLen;

        // since csum pos/start are both half-word aligned, so 15 bytes are also skipped
        // as it has same effect as 14bytes.
        if (randPktLen <= csXsumSkipLen)
        {
            pPacket->Type &= ~PKT_TYPE_TO_CXSUM;
            pPacket->CsumPos = 0xFFFF;     // -1 means no Custom XSUM
            pPacket->CsumStart = 0xFFFF;   // -1 means no Custom XSUM
        }
        else
        {
            for (i = 0; i < 2; i++)
            {
                cxTmp = ((uint16_t)RAND()) % (csXsumMaxOff - csXsumBaseLen);
                cxTmp &= 0xFFFE;          // 2bytes alignment, max 512 bytes
                cxTmp += csXsumBaseLen;   // skip the MAC/SNAP header

                // if the packet length is odd, and pos/start is at the last byte, move backward 2 bytes
                if (randPktLen - cxTmp == 1)
                {
                    cxTmp -= 2;
                }
                cxOff[i] = cxTmp;
            }

            pPacket->CsumStart = cxOff[0];
            pPacket->CsumPos = cxOff[1];

            p = pPktBuf;
            cxTmp = pPacket->CsumPos;
            *((uint16_t*)(p + cxTmp)) = 0;  // clear the cxsum value to zero
            // since customer xsum function and ip/tcp-udp xsum function are mutually exclusive.
            // so store the xsum value to UpLevelReserved[0] which also used for stroing ip xsum.
            pPacket->UpLevelReserved[0] = (void*)(unsigned long)ComputeChecksum(
                p + pPacket->CsumStart, randPktLen - pPacket->CsumStart, 0);
        }
    }

    // step3: split the packet into several piece, and fill them into ATHR_DOS_PACKET
    pPacket->Length = randPktLen;
    p = pPktBuf;

    uint16_t  splitNum, splitLen, splitLenSum;
    uint32_t  splitSum;
    uint16_t  split[MAX_PKT_BUF]; //at most split into 4 pieces

    splitNum = (uint16_t)RAND();
    splitNum %= MAX_PKT_BUF;
    splitNum += 1;

    splitSum = 0;
    for (i = 0; i < splitNum; i++)
    {
        split[i] = (uint16_t)RAND() % 2000;
        split[i]++;     // to avoid zero, [1, 2000]
        splitSum += split[i];
    }

    splitLenSum = 0;
    for (i = 0; i < splitNum; i++)
    {
        splitLen = (uint16_t)(((randPktLen * split[i] * 1.0) / splitSum) + 1.0);

        // !!! This can be arbitrary address, no alignment requirements !!!
        // align splitLen to 16bytes, if 0, then set to 16;
        //splitLen += 15;
        //splitLen &= 0xFFF0;
        //if (splitLen == 0)
        //    splitLen = 16;

        pPacket->Buf[i].Addr = p + splitLenSum;
        pPacket->Buf[i].Length = splitLen;

        splitLenSum += splitLen;
        if (splitLenSum >= randPktLen)
        {
            splitLen -= (splitLenSum - randPktLen);
            splitLenSum -= (splitLenSum - randPktLen);
            pPacket->Buf[i].Length = splitLen;
            break;  // exit the loop if already reach the end
        }
    }

    for (i = 0; i < splitNum; i++)
    {
        pPacket->SGList[i].Addr = 0;
        pPacket->SGList[i].Length = pPacket->Buf[i].Length;
    }

    return STATUS_SUCCESSFUL;
}

// link the generated packets together and return the head of packet list
PATHR_DOS_PACKET linkPackets(IN PATHR_DOS_PACKET pOldPacket, IN PATHR_DOS_PACKET pNewPacket)
{
    PATHR_DOS_PACKET pPacket = pOldPacket;

    if (pPacket == NULL)
        return pNewPacket;
        
    // the end of new added packet points to NUll and it will be inserted at the end
    while (pPacket->Next != NULL)
    {
        pPacket = pPacket->Next;
    }
    pPacket->Next = pNewPacket;
    
    return pOldPacket;
}

int clearStat()
{
    int     ret;
    char    str[] = "ClearStat";

    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        str, sizeof(str), NULL, 0);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    return STATUS_SUCCESSFUL;
}

int getIpHdrOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset)
{
    u8* pRxBuf = NULL;
    u16 v16 = 0;
    u16 offset = 0;

    // if the input packet is not an ip packet
    if ((pRx->Type & (PKT_TYPE_IPV4|PKT_TYPE_IPV6)) == 0)
    {
        printf("Not a IPv4 or IPv6 packet.\n");
        *pBufIndex = 0;
        *pBufOffset = 0;
        return STATUS_FAILED;
    }

    // definitely, ip header will located in the 1st buffer, so don't have to consider crossing buffer
    // at least, the buffer size will be 256 bytes.  
    pRxBuf = (u8 *) pRx->Buf[0].Addr;

    offset = 12;    // skip MAC addr
    v16 = ntohs(*((u16 *)(pRxBuf + offset)));
    if (v16 == 0x8100) // if VLAN tag, skip it
    {
        offset += 4;
    }
    
    v16 = ntohs(*((u16 *)(pRxBuf + offset)));
    if ((v16 == 0x0800) || (v16 == 0x86DD))  // if ip datagram v4 / v6
    {
        offset += 2;
    }
    else if (v16 < 0x0800)
    {
        offset += 2;
        v16 = ntohs(*((u16 *)(pRxBuf + offset)));
        if (v16 == 0xAAAA)
        {
            offset += 2;
            v16 = ntohs(*((u16 *)(pRxBuf + offset)));
            if (v16 == 0x0300)
            {
                offset += 2;
                v16 = ntohs(*((u16 *)(pRxBuf + offset)));
                if (v16 == 0x1374)  // Attansic OUI
                {
                    offset += 2;    // skip over SNAP header
                    v16 = ntohs(*((u16 *)(pRxBuf + offset)));
                    if ((v16 == 0x0800) || (v16 ==0x86DD)) // if ip datagram v4/v6
                    {
                        offset += 2;
                    }
                    else
                    {
                        return STATUS_FAILED;
                    }
                }
                else
                {
                    return STATUS_FAILED;
                }
            }
            else
            {
                return STATUS_FAILED;
            }
        }
        else
        {
            return STATUS_FAILED;
        }
    }
    else    // neither Ethernet II, nor SNAP.... cannot be recognized
    {
        return STATUS_FAILED;
    }

    *pBufIndex = 0;
    *pBufOffset = offset;
    
    return STATUS_SUCCESSFUL;
}

int getIpXsumOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset)
{
    int ret;
    u16 bufOffset = 0;
    u16 bufIndex = 0;

    if ((pRx->Type & PKT_TYPE_IPV4) == 0) // if input is not a ipv4 packet, return failed
    {
        *pBufIndex = 0;
        *pBufOffset = 0;
        return STATUS_FAILED;
    }

    // ipxsum must also located in the 1st buf. 
    if ((ret = getIpHdrOffset(pRx, &bufIndex, &bufOffset)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get IP header offset.\n");
        return ret; // not ip packet
    }

    bufOffset += 10;  // 10 bytes offset from the IPv4 Header

    *pBufIndex = bufIndex;
    *pBufOffset = bufOffset;

    return STATUS_SUCCESSFUL;
}

int getL4HdrOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, 
                     OUT u16 *pBufOffset, OUT u8 *pL4Protocol)
{
    int ret;
    u8  ipVer;
    u8 *p;
    u8  l4Protocol;
    u16 bufIndex;
    u16 bufOffset;
    u16 stepFromIpHdr = 0;

    if ((ret = getIpHdrOffset(pRx, &bufIndex, &bufOffset)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get IP header offset.\n");
        return ret; // not ip packet
    }
//>>> DBG
#if 0
int i = 0;
u8 *p8;
p8 = (u8 *) (pRx->Buf[0].Addr);
printf("\n>>>>>>\n");
for (i = 0; i < pRx->Buf[0].Length; i++)
{
    printf("%02x ", *p8++);
}
printf("\n<<<<<<\n");
#endif
//<<<
    p = (u8 *) (pRx->Buf[bufIndex].Addr);
    p += bufOffset;
    ipVer = *p;
    ipVer >>= 4;

    if (ipVer == 4) // ipv4
    {
        u16 ipv4HdrLen;
        IPHeader *ipHdr = (IPHeader *)p;
        
        ipv4HdrLen = ipHdr->iph_verlen;        // 1st byte of ip header
        ipv4HdrLen &= 0xF;      // length field
        ipv4HdrLen <<= 2;       // x4 to get length
        stepFromIpHdr = ipv4HdrLen;    // l4 header offset
        // !!! it is impossible to jump to the next buf of RX packet in our implementation
        // so do not have to consider this situation here !!!!
        l4Protocol = ipHdr->iph_protocol;      // tcp or udp
    }
    else if (ipVer == 6)    // ipv6
    {
        // !!! it is impossible to jump to the next buf of RX packet in our implementation
        // so do not have to consider this situation here !!!!
        IPV6Header* ipv6h = (IPV6Header*) p;

        u8 nextHdr = ipv6h->next_header;
        IPV6ExtenHeader* extHdr;
        
        stepFromIpHdr = sizeof(IPV6Header);
        while (nextHdr != EXT_TCP && nextHdr != EXT_UDP)
        {
            extHdr = (IPV6ExtenHeader *) (p + stepFromIpHdr);
            if (nextHdr == EXT_AUTH)
            {
                stepFromIpHdr += (8 + extHdr->hdr_length * 4);
            }
            else if (nextHdr == EXT_FRAGMENT)
            {
                stepFromIpHdr += 8; //fragmentation header fixed at 8 bytes
            }
            else
            {
                stepFromIpHdr += (8 + extHdr->hdr_length * 8);
            }
            nextHdr = extHdr->next_header;
        }
        l4Protocol = nextHdr;       // tcp or udp
    }
    else
    {
        return STATUS_FAILED;
    }

    *pBufIndex = bufIndex;
    *pBufOffset = bufOffset + stepFromIpHdr;
    *pL4Protocol = l4Protocol;
    
    return STATUS_SUCCESSFUL;
}

int getL4XsumOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset)
{
    int ret;
    u8 l4Protocol;
    u16 bufIndex;
    u16 bufOffset;
        
    if ((ret = getL4HdrOffset(pRx, &bufIndex, &bufOffset, &l4Protocol)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get lever4 header offset.\n");
        return ret;
    }

//printf("L4Hdr[bufIndex=%d, bufOffset=%d, l4Protocol=%d] ", bufIndex, bufOffset, l4Protocol);   //DBG only
    // !!! it is impossible to jump to the next buf of RX packet in our implementation
    // so do not have to consider this situation here !!!!
    if (l4Protocol == EXT_TCP)
    {
        bufOffset += 16;    // 16bytes offset from tcp header
    }
    else if (l4Protocol == EXT_UDP)
    {
        bufOffset += 6;    // 6bytes offset from tcp header
    }
    else
    {
        return STATUS_FAILED;
    }

    *pBufIndex = bufIndex;
    *pBufOffset = bufOffset;
    
    return STATUS_SUCCESSFUL;
}

int getL4PayloadOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset)
{
    int ret;
    u16 bufIndex, bufOffset;
    u8 l4Protocol;
    
    if ((ret = getL4HdrOffset(pRx, &bufIndex, &bufOffset, &l4Protocol)) != STATUS_SUCCESSFUL)
    {
        printf("Failed to get lever4 header offset in getL4PayloadOffset().\n");
        return ret;
    }

    // do not consider the situation that stepping over the 1st buf of RX packet
    if (l4Protocol == EXT_TCP)
    {
        u8 dataOff;
        u8 *p = (u8 *) (pRx->Buf[bufIndex].Addr);
        p += bufOffset; // point to TCP header now
        dataOff = *(p + 12);  //data offset and Rsvd 
        dataOff >>= 4;  
        dataOff <<= 2;   // 4bytes to bytes
        bufOffset += dataOff;
    }
    else if (l4Protocol == EXT_UDP)
    {
        bufOffset += sizeof(UDPHeader);
    }

    *pBufIndex = bufIndex;
    *pBufOffset = bufOffset;
    
    return STATUS_SUCCESSFUL;
}
int getCsXsumOffset(IN u32 CsumPos, IN PATHR_DOS_PACKET pRx, 
                         OUT u16 *pBufIndex, OUT u16 *pBufOffset)
{
    u32 bufLenSum;
    int i;

    bufLenSum = 0;
    for (i = 0; i < MAX_PKT_BUF; i++)
    {
        if (CsumPos >= bufLenSum && 
            CsumPos < (bufLenSum + pRx->Buf[i].Length))
        {
            *pBufIndex = i;
            *pBufOffset = (u16)(CsumPos - bufLenSum);
            return STATUS_SUCCESSFUL;
        }
        else
        {
            bufLenSum += pRx->Buf[i].Length;
        }
    }
    
    return STATUS_FAILED;
}
                         

// compare num of input ATHR_DOS_PACKET byte by byte
// 0 -- same packet, 1 -- not same
int cmpPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, IN u16 num)
{
    int ret;
    bool bVlanTagged;
    bool bVlanInsert;
    bool bIpXsum;
    bool bL4Xsum;
    bool bCsXsum;    //custom checksum
    bool bTxIpXsumErr;
    bool bTxL4XsumErr;
    u32 i, j, k, len;
    int lenDiff;    // length difference of Tx packet and Rx packet because of the VLAN

#ifdef LOG_RECV_ERR_PKT //DBG only
PATHR_DOS_PACKET tempTx = pTx;
PATHR_DOS_PACKET tempRx = pRx;
#endif  //LOG_RECV_ERR_PKT

    for (i = 0; i < num; i++)
    {
        if (pTx == NULL || pRx == NULL) // no packet to compare
        {
            printf("NULL packet of TX or RX packet...\n");
            goto ErrRet;
            //return STATUS_FAILED;  
        }

        // switch controll each packet
        bVlanTagged = (pTx->Type & PKT_TYPE_VLANTAGGED) ? true : false;
        bVlanInsert = (pTx->Type & PKT_TYPE_TO_VLANINST) ? true : false;;
        bIpXsum = (pTx->Type & PKT_TYPE_TO_IPXSUM) ? true : false;;
        bL4Xsum = (pTx->Type & PKT_TYPE_TO_L4XSUM) ? true : false;;
        bCsXsum = (pTx->Type & PKT_TYPE_TO_CXSUM) ? true : false;;    //custom checksum

        // These bits in fact reflect the status of incoming packets. But they are used to flag whether the Xsum value
        // filled in TX packet are valid. To do some tests, a wrong xsum value must be filled.
        bTxIpXsumErr = (pTx->Type & PKT_TYPE_IPXSUM_ERR) ? true : false;;
        bTxL4XsumErr = (pTx->Type & PKT_TYPE_L4XSUM_ERR) ? true : false;;

        // global switch affects all incoming/outgoing packets
        // pAppAdapter->bRxChkIpXsum;
        // pAppAdapter->bRxChkTcpXsum;
        // pAppAdapter->bRxChkUdpXsum;
        // pAppAdapter->bVlanStrip;


        if (pRx->Type & PKT_RX_STATUS)
        {
            if (pRx->Type & PKT_TYPE_INCOMPLETE_ERR)
            {
                printf("PKT_TYPE_INCOMPLETE_ERR... length [%d-%d], dump...\n", 
                        pTx->Length, pRx->Length);
#ifdef  SKIP_INCOMPLETE_PKT
                pTx = pTx->Next;
    			pRx = pRx->Next;
    			continue;
#endif  //SKIP_INCOMPLETE_PKT
            }

            if (pRx->Type & PKT_TYPE_IPXSUM_ERR)
            {
                printf("PKT_TYPE_IPXSUM_ERR... \n");
#ifdef  SKIP_IPXSUM_ERR_PKT
                pTx = pTx->Next;
    			pRx = pRx->Next;
    			continue;
#endif  //SKIP_IPXSUM_ERR_PKT
            }

            if (pRx->Type & PKT_TYPE_L4XSUM_ERR)
            {
                printf("PKT_TYPE_L4XSUM_ERR...\n");
#ifdef  SKIP_L4XSUM_ERR_PKT
                pTx = pTx->Next;
    			pRx = pRx->Next;
    			continue;
#endif  //SKIP_L4XSUM_ERR_PKT
            }

            if (pRx->Type & PKT_TYPE_802_3_LEN_ERR)
            {
                printf("PKT_TYPE_802_3_LEN_ERR...\n");
#ifdef  SKIP_802_3_LEN_ERR_PKT
                pTx = pTx->Next;
    			pRx = pRx->Next;
    			continue;
#endif  //SKIP_802_3_LEN_ERR_PKT
            }

            if (pRx->Type & PKT_TYPE_CRC_ERR)
            {
                printf("PKT_TYPE_CRC_ERR...\n");
#ifdef  SKIP_CRC_ERR_PKT
                pTx = pTx->Next;
    			pRx = pRx->Next;
    			continue;
#endif  //SKIP_CRC_ERR_PKT
            }

            if (pRx->Type & PKT_TYPE_RX_ERR)
            {
                printf("PKT_TYPE_RX_ERR...\n");
            }

            goto ErrRet;
        }

        // comparing the length of TX/RX first, if not equeal, 
        // unnecessary to do the following contents comparing 
        // and adjust RX packet length when neccessary.
        if (bVlanTagged && bVlanInsert)
        {
            printf("Do not support Vlan-in-Vlan packet...\n");
            goto ErrRet;
        }

        lenDiff = 0;
        if (bVlanTagged && pAppAdapter->bVlanStrip)
        {
            if (pTx->Length <= 64)  // 14 + 46 + 4
            {
                pRx->Length = pTx->Length - 4;   // HW cannot detect the padding contents
                pRx->Buf[0].Length = pRx->Length;
            }
            lenDiff = 4;
        }
        else if (bVlanInsert && !(pAppAdapter->bVlanStrip))
        {
            if (pTx->Length <= 60)  // 14 + 46
            {
                pRx->Length = pTx->Length + 4;   // HW cannot detect the padding contents
                pRx->Buf[0].Length = pRx->Length;
            }
            lenDiff = -4;
        }
        else if (bVlanTagged && !(pAppAdapter->bVlanStrip))
        {
            if (pTx->Length <= 64)  // 14 + 46 + 4
            {
                pRx->Length = pTx->Length;   // HW cannot detect the padding contents
                pRx->Buf[0].Length = pRx->Length;
            }
        }
        else
        {
            if (pTx->Length <= 60)  // 14 + 46
            {
                pRx->Length = pTx->Length;   // HW cannot detect the padding contents
                pRx->Buf[0].Length = pRx->Length;
            }
        }

        int txPktLen, rxPktLen;
        txPktLen = (int) pTx->Length;
        rxPktLen = (int) pRx->Length;
//DBG >>>
//printf("TxType[0x%08x], Tag[%d] Ins[%d] Strip[%d], txLen=%d, rxLen=%d, diff=%d\n", 
//        pTx->Type, bVlanTagged, bVlanInsert, pAppAdapter->bVlanStrip, txPktLen, rxPktLen, lenDiff);
//<<< DBG
        if (txPktLen - rxPktLen != lenDiff)
        {
            printf("Packet length of TX and RX inequal. txPktLen = %d, rxPktLen = %d\n", txPktLen, rxPktLen);
            goto ErrRet;
        }

        len = 0;
        for (j = 0; j < MAX_PKT_BUF; j++)
        {
            if (pRx->Buf[j].Addr != 0) // if address is valide, then count in Length
                len += pRx->Buf[j].Length;
        }
        if (pRx->Length != len)     // if total length dosen't equal to the sum of buffer length
        {
            printf("Inequal at %d pkt...", i);
            printf("Pkt len != Sum of buf len: %d(%d)\n", pRx->Length, 
                   pRx->Buf[0].Length);
#if 0
            printf("Pkt len != Sum of buf len: %d(%d %d %d %d)\t", pRx->Length, 
                   pRx->Buf[0].Length, pRx->Buf[1].Length,
                   pRx->Buf[2].Length, pRx->Buf[3].Length);
#endif
            goto ErrRet;
            //return STATUS_PACKET_INEQUAL;
        }

        u8 *pTxBuf, *pRxBuf;
        u8 *pRxBufArray[MAX_PKT_BUF];
        u32 rxBufLen[MAX_PKT_BUF];
        u16 rxBufIndex;
        u16 bufIndexIp = 0, bufIndexL4 = 0, bufIndexCs = 0;
        u16 bufOffsetIp = 0, bufOffsetL4 = 0, bufOffsetCs = 0;
        
        for (j = 0; j < MAX_PKT_BUF; j++)
        {
            pRxBufArray[j] = (u8 *) pRx->Buf[j].Addr;
            rxBufLen[j] = pRx->Buf[j].Length;
        }

        // and the Tx packet's buffer will definitely be continuous, but cannot assume this on Rx packet
        pTxBuf = (u8 *) pTx->Buf[0].Addr;

        // Get the offset of checksum filed and skip them when comparing
        if (bIpXsum || bTxIpXsumErr)
        {
            if ((ret = getIpXsumOffset(pRx, &bufIndexIp, &bufOffsetIp)) != STATUS_SUCCESSFUL)
            {
                printf("Failed to get IP xsum offset\n");
                goto ErrRet;
                //return ret;
            }
        }
        if (bL4Xsum || bTxL4XsumErr)
        {
            if ((ret = getL4XsumOffset(pRx, &bufIndexL4, &bufOffsetL4)) != STATUS_SUCCESSFUL)
            {
                printf("Failed to get L4 xsum offset\n");
                return ret;
            }
        }
        if (bCsXsum)
        {
            if ((ret = getCsXsumOffset(pTx->CsumPos, pRx, &bufIndexCs, &bufOffsetCs)) != STATUS_SUCCESSFUL)
            {
                printf("Failed to get custom xsum offset\n");
                return ret;
            }
        }
            
        // since every buffer in RX packet is aligned at 8bytes. So we don't have consider the VLAN/XSUM
        // crossing border problem. Because Xsum field are only 2bytes and VLAN is only 4bytes.
        j = k = 14;
        //j = k = 0;
        rxBufIndex = 0;
        pRxBuf = pRxBufArray[rxBufIndex];
        while (1)
        {
            // In Tx packet is tagged with VLAN and VLAN has been stripped in RX pkt
            // The RX packet has 4bytes less than TX packet in length.
            // Tx - 4 = RX
            if (bVlanTagged && pAppAdapter->bVlanStrip)
            {
                if (j == 12)
                {
                    j += 4;     // skip 
                }
            }

            // In Tx packet is to tag a VLAN by NIC and VLAN will not be stripped in RX pkt
            // The TX packet has 4bytes more than RX packet in length.
            // Tx + 4 = RX
            if (bVlanInsert && !(pAppAdapter->bVlanStrip))
            {
                if (rxBufIndex == 0 && k == 12)
                {
                    k += 4;
                }
            }
            
            // In Tx ip xsum is done by NIC or filled an error xsum value in TX packet intended.
            // Then the ip xsum field should be skipped in comparing
            if (bIpXsum || bTxIpXsumErr)
            {
                if (bufIndexIp == rxBufIndex && bufOffsetIp == j)
                {
                    k += 2;
                    j += 2;
                }
            }
            
            // In Tx L4 xsum is done by NIC or filled an error xsum value in TX packet intended.
            // Then the L4 xsum field should be skipped in comparing
            if (bL4Xsum || bTxL4XsumErr)
            {
                if (bufIndexL4 == rxBufIndex && bufOffsetL4 == j)
                {
                    k += 2;
                    j += 2;
                }
            }

            // In Tx Custom xsum is done by NIC. Rx has no way to verify this value. SW's responsbility.
            // Then the ip custom xsum field should be skipped in comparing
            if (bCsXsum)
            {
                if (bufIndexCs == rxBufIndex && bufOffsetCs == j)
                {
                    k += 2;
                    j += 2;
                }
            }

            // if reaching the end of RX packet or reach the end of one of the recieve buffer,
            // it must be placed here after the increment of j/k.
            if (j >= pTx->Length)
            {
                break;
            }
            if (k >= rxBufLen[rxBufIndex])  // it seem that impossible to >, always ==
            {
                k -= rxBufLen[rxBufIndex];
                rxBufIndex++;
                pRxBuf = pRxBufArray[rxBufIndex];
            }

            if (*(pTxBuf + j) != *(pRxBuf + k))
            {
                //printf("j=%u, k=%u, rxBufIndex=%d\n", j, k, rxBufIndex);
                goto ErrRet;
            }

            else
            {
                j++; 
                k++;
                if (k == rxBufLen[rxBufIndex])
                {
                    rxBufIndex++;
                    k = 0;
                    pRxBuf = pRxBufArray[rxBufIndex];
                }
            }
        }
        
        pTx = pTx->Next;
        pRx = pRx->Next;
    }
        
    return STATUS_PACKET_EQUAL;

ErrRet:
    
#ifdef LOG_RECV_ERR_PKT   //DBG only
logRecvErrPkt(tempTx, tempRx, num, (u16)i);
#endif  //LOG_RECV_ERR_PKT DBG only

    return STATUS_PACKET_INEQUAL | (i << 4);
}

// advance the input packet NUM times, and return the new pointer
// return NULL if reach the end
PATHR_DOS_PACKET advancePackets(IN PATHR_DOS_PACKET pPkt, IN u16 num)
{
    int i = 0;

    if (pPkt == NULL)
    {
        return NULL;
    }
    for (i = 0; i < num; i++)
    {
        pPkt = pPkt->Next;
        if (pPkt == NULL)   // if reach the end, return, no further advancing
        {
            return NULL;
        }
    }
    return pPkt;
}

// Free num packets and advance the packet list from N to N + num
PATHR_DOS_PACKET freePackets(IN PATHR_DOS_PACKET pPkt, IN u32 num)
{
    u32 i = 0;
    PATHR_DOS_PACKET pOldPkt = pPkt;

    for (i = 0; i < num; i++)
    {
        pPkt = advancePackets(pPkt, 1);
        free(pOldPkt);
        if (pPkt == NULL)
        {
            return NULL;
        }
        pOldPkt = pPkt;
    }
   
    return pPkt;
}

int dumpReg()
{
    int     ret;
    char    str[] = "DumpReg";

    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_GET_PARAM, 
        str, sizeof(str), NULL, 0);
    if (ret != STATUS_SUCCESSFUL) {
        return ret; 
    }
    return STATUS_SUCCESSFUL;
}


int testGeneralLpback(PAPP_ADAPTER pAppAdapter)
{
    u32  i, loopCount, pktCount, recvdPktCount, recvCount; 
    int  ret = STATUS_SUCCESSFUL;
    u32  v32;
    u16  num;
    u8*  pBuf = NULL;
    time_t t0, t1;
    double pause;
    PATHR_DOS_PACKET    pPktArry, pPktTmp, pPktSent, pPktRcvd;
    GENERATE_PKT_TYPE   pktType;

#ifdef  LOG_RECV_TIME_OUT
PATHR_DOS_PACKET    pPktPreTimeOut = NULL;
#endif  //LOG_RECV_TIME_OUT

    memset(&pktType, 0, sizeof(pktType));
    pktType.vlan_type = vlan_type_none;
    pktType.l3_type = l3_type_none;
    pktType.l4_type = l4_type_none;
    
    usleep(200 * 1000);
    recvCount = 0;
    for (loopCount = 0; loopCount < pAppAdapter->LoopCount; loopCount++)
    {
        pBuf = pAppAdapter->pAlignedDmaVirAddr;
        pPktArry = pPktTmp = pPktSent = pPktRcvd = NULL;

        if (pAppAdapter->bOneByOne) {
            pktCount = 1;
        } else {
            pktCount = ((u16) RAND()) % MAX_PACKET_COUNT;
            pktCount++;   //[1, MAX_PACKET_COUNT]        
        }

        if (athr_kbhit()) {
            int key = athr_getch();
            if (0x1B == key) {  // K_Escape
                ret = ERR_USER_BREAK;
                break;
            }
        }

        pAppAdapter->LoopIndex = loopCount;     //For DBG only
        // generate packets
        for (i = 0; i < pktCount; i++)
        {
            pAppAdapter->PktIndex = i;   //For DBG only
            //pktType.frame_type = frame_type_random;
            pktType.frame_type = frame_type_ethernetII;
            pktType.l3_type = l3_type_ipv4;
            pktType.l4_type = l4_type_udp;
            genPacketType(&pktType, pAppAdapter->enPacketType);
            genIpAddr(pAppAdapter, &pktType);
            generateRandomPacket(pAppAdapter, &pktType, pBuf, &pPktTmp);
#if 0
            printf("generate Packet length is %d, data is :\n", pPktTmp->Length);
            //for(int i = 0; i < 50; i++){
            //    printf("%X ", pPktTmp->Buf[0].Addr[i]);
            //    if (i != 0 && i % 8 == 0 ){
            //        printf("\n");
            //    }
            //}
#endif

            v32 = pPktTmp->Length;
            v32 += 0xF;
            v32 &= 0xFFFFFFF0;
            pBuf += v32;

            pPktArry = linkPackets(pPktArry, pPktTmp);
        }

        // send packets
        if((loopCount + 1) % 20 == 0){
            DBG_PRINT(("**Loopback[%d]** Sending test packets...\n", 
                loopCount + 1));  // for DBG only
        }

        clearStat();
        pPktSent = pPktArry;
        gAthrDosappChar.Send(gPDriver, pPktArry, (u16)pktCount);
        t0 = time(NULL);

        // recieve packet by polling the value of gRecvdPktNum 
        recvdPktCount = 0;
        num = 0;
        //while (1)
        {
            //num = gRecvdPktNum;
            memset(gpRecvdPkt, 0, gBufSize);
            usleep(50 * 1000);
            num = gAthrDosappChar.Receive(gPDriver, gpRecvdPkt, gBufSize);
            if (num != 0)
            {
                int dbgRet;
                recvdPktCount += num;
                if ((dbgRet = cmpPackets(pAppAdapter, pPktArry, gpRecvdPkt, num)) != STATUS_PACKET_EQUAL)
                {
                    //dbgRet >>= 4;
                    //printf("ERROR: Inequal at pkt[%d] (1 based)...\n", (int)(recvdPktCount-num+1+dbgRet));
                    //printf("cmpPackets return error, error code is [%d] .\n", dbgRet);
#if 0
            printf("Tx Packet length is %d, num is %d, data is :\n", pPktArry->Length, num);
            for(int i = 0; i < pPktArry->Length; i++){
                printf("%X ", pPktArry->Buf[0].Addr[i]);
                if (i != 0 && i % 16 == 0 ){
                    printf("\n");
                }
            }
            printf("generate Packet length is %d, data is :\n", gpRecvdPkt->Length);
            for(int i = 0; i < gpRecvdPkt->Length; i++){
                printf("%X ", gpRecvdPkt->Buf[0].Addr[i]);
                if (i != 0 && i % 16 == 0 ){
                    printf("\n");
                }
            }
#endif

                    ret = STATUS_PACKET_INEQUAL;
                    //break;
                }else{
                    recvCount ++;
                }

#ifdef  LOG_RECV_TIME_OUT
                pPktPreTimeOut = NULL;
                pPktArry = advancePackets(pPktArry, num-1);
                pPktPreTimeOut = pPktArry;
                pPktArry = advancePackets(pPktArry, 1);
#else
                pPktArry = advancePackets(pPktArry, num);   //advancing the sending packet list
#endif  //LOG_RECV_TIME_OUT

                pPktRcvd = gpRecvdPkt;

                if (recvdPktCount != pktCount)  // recieved packets already == xmitted packets
                {
                    printf("FXG: recvdPktCount is %d, pktCount is %d \n", recvdPktCount,pktCount);
                }
            }
            else
            {
                //printf("recieve loopback data fail and has missed No.%d packets.\n", loopCount);
            }

            t1 = time(NULL);
            pause = difftime(t1, t0);
            if (pause > 5) 
            {   // 6sec
                printf("ERROR: timeout(%ds) waitting for recveiving all packets!\n", \
                    (int)(pause));
                ret = ERR_RX_TIMEOUT_LOOPBACK;
                
#ifdef  LOG_RECV_TIME_OUT
                DBG_PRINT(("Dumping the last recieved packet before time-out...\n"));
                dbgPktToFile(pPktPreTimeOut);
                WAIT_KEY();

                DBG_PRINT(("Start dumping the time-out packet...\n"));
                dbgDumpPktInfo(pPktArry);
                dbgPktToFile(pPktArry);
#endif  //LOG_RECV_TIME_OUT

                break;
            }
        }     

        // free the packet list random generated for sending
        freePackets(pPktSent, pktCount);

        if (ret != STATUS_SUCCESSFUL)   // if failed in any loop, do not do loop any more
        {
            //dumpReg();
            //WAIT_KEY();
            //break;
        }
    }
    printf("loopback test send %d packets, recv %d packets.\n", pAppAdapter->LoopCount, recvCount);
    if(recvCount != pAppAdapter->LoopCount)
    {
        ret = STATUS_FAILED;
    }
    else{
        ret = STATUS_SUCCESSFUL;
    }
    return ret;
}

int PhyLoopbackTest(PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    SET_DEV_PARAM setLinkSpeed = {(u8 *)"LinkCap", 0};
    nicPos = pAppAdapter->DevId;

    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_PHY_LOOPBACK);
    if (gPDriver == NULL) {
        ret = ERR_FAILED_LOAD_DRIVER;
        goto exit;
    }
    
    if ((pAppAdapter->LinkCapability & LC_TYPE_1000F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 1000Mb 
    {
        // set to 1000Mb 
	printf("	1000M test ......\n");
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 1000M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    if ((pAppAdapter->LinkCapability & LC_TYPE_100F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 100Mb 
    {
        // set to 100Mb 
	printf("	100M test ...... \n");
        setLinkSpeed.val = LC_TYPE_100F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 100M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    if ((pAppAdapter->LinkCapability & LC_TYPE_10F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 10Mb 
    {
        // set to 10Mb 
	printf("	10M test ......\n");
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 10M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    ret = gAthrDosappChar.Unload(gPDriver);
    if (ret != STATUS_SUCCESSFUL) {
        // failed to unload the NIC driver
        goto exit;
    }
    gPDriver = NULL;

exit:
    return ret;
}

int CableLoopbackTest(PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    SET_DEV_PARAM setLinkSpeed = {(u8 *)"LinkCap", 0};
    nicPos = pAppAdapter->DevId;

    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_CABLE_LOOPBACK);
    if (gPDriver == NULL) {
        ret = ERR_FAILED_LOAD_DRIVER;
        goto exit;
    }
    
    /* set phy to cable loop back mode */
    HwSetPhyReg2CableLoopbackMode((PADAPTER)gPDriver);

    if ((pAppAdapter->LinkCapability & LC_TYPE_1000F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 1000Mb 
    {
        // set to 1000Mb 
	printf("	1000M test ......\n");
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 1000M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    if ((pAppAdapter->LinkCapability & LC_TYPE_100F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 100Mb 
    {
        // set to 100Mb 
	printf("	100M test ...... \n");
        setLinkSpeed.val = LC_TYPE_100F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 100M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    if ((pAppAdapter->LinkCapability & LC_TYPE_10F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 10Mb 
    {
        // set to 10Mb 
	printf("	10M test ......\n");
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 10M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        ret = testGeneralLpback(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto exit;
        }
    }

    ret = gAthrDosappChar.Unload(gPDriver);
    if (ret != STATUS_SUCCESSFUL) {
        // failed to unload the NIC driver
    }
    gPDriver = NULL;

exit:
    
    HwResetPHY2NormalMode((PADAPTER)gPDriver);
    
    if(gPDriver){
        gAthrDosappChar.Unload(gPDriver);
        gPDriver = NULL;
    }
    return ret;
}


int XsumTest(PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL, finalRet = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    u32     cmdType = pAppAdapter->UserCmd;
    SET_DEV_PARAM setLinkSpeed = {(u8 *)"LinkCap", 0};
    nicPos = pAppAdapter->DevId;

    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_PHY_LOOPBACK);
    if (gPDriver == NULL) {
        ret = ERR_FAILED_LOAD_DRIVER;
        goto exit;
    }

    if ((pAppAdapter->LinkCapability & LC_TYPE_1000F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 1000Mb 
    {
        // set to 1000Mb 
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 1000M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        if ((cmdType & (CMD_CHK_XSUM_IPV4|CMD_CHK_XSUM_L4)) == 
            (CMD_CHK_XSUM_IPV4|CMD_CHK_XSUM_L4))
        {
            DBG_PRINT(("test packets type: IPv4 + L4... \n"));
            ret = testXsum(pAppAdapter, cmdType);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("IP and Level4 xsum test failed.\n");
                finalRet = STATUS_FAILED;
            }
            printf("IP and Level4 xsum test done.\n");
        }
#if 0
        usleep(50 * 1000);

        if (cmdType & CMD_CHK_XSUM_IPV4)
        {
            DBG_PRINT(("test packets type: IPv4 only... \n"));
            ret = testXsum(pAppAdapter, cmdType);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("Ip xsum test failed.\n");
                finalRet = STATUS_FAILED;
            }
            printf("Ip xsum test done.\n");
        }

        usleep(50 * 1000);

        if (cmdType & CMD_CHK_XSUM_L4)
        {
            DBG_PRINT(("test packets type: L4 only... \n"));
            ret = testXsum(pAppAdapter, cmdType);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("Level4 xsum test failed.\n");
                finalRet = STATUS_FAILED;
            }
            printf("Level4 xsum test done.\n");
        }

        usleep(50 * 1000);

        if (cmdType & CMD_CHK_CXSUM)
        {
            DBG_PRINT(("test packets type: Custom Xsum... \n"));
            ret = testXsum(pAppAdapter, cmdType);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("Custom xsum test failed.\n");
                finalRet = STATUS_FAILED;
            }
            printf("Custom xsum test done.\n");
        }
#endif
    }

    ret = gAthrDosappChar.Unload(gPDriver);
    if (ret != STATUS_SUCCESSFUL) {
        // failed to unload the NIC driver
        finalRet = ret;
        goto exit;
    }
    gPDriver = NULL;

exit:
    return finalRet;
}

int LsoTest(PAPP_ADAPTER pAppAdapter)
{
    int     ret = STATUS_SUCCESSFUL;
    u16     nicPos = 0;
    u32     cmdType = pAppAdapter->UserCmd;
    SET_DEV_PARAM setLinkSpeed = {(u8 *)"LinkCap", 0};
    nicPos = pAppAdapter->DevId;

    printf("TCP large send offload test .......................\n");
    gPDriver= gAthrDosappChar.Load(nicPos, LOAD_MODE_PHY_LOOPBACK);
    if (gPDriver == NULL) {
        ret = ERR_FAILED_LOAD_DRIVER;
        goto exit;
    }
    
    if ((pAppAdapter->LinkCapability & LC_TYPE_1000F) && (pAppAdapter->DevId & 1)) // bit0 of DevID set, means capable of supporting 1000Mb 
    {
        // set to 1000Mb 
        setLinkSpeed.val = LC_TYPE_1000F;
        ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setLinkSpeed, sizeof(setLinkSpeed), NULL, 0);
        if (ret != STATUS_SUCCESSFUL) {
            printf("Failed to set NIC to 1000M mode.\n");
            goto exit; 
        }

        ret = gAthrDosappChar.Reset(gPDriver, true);
        if (ret != STATUS_SUCCESSFUL) {
            // failed to reset the NIC 
            goto exit;
        }

        if (cmdType & CMD_CHK_LSO_V1)
        {
            DBG_PRINT(("v1... "));
            ret = testLso(pAppAdapter, CMD_CHK_LSO_V1);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("LSO v1 test failed.\n");
                //goto exit;
            }
        }

        if (cmdType & CMD_CHK_LSO_V2)
        {
            //DBG_PRINT(("v2... "));
            ret = testLso(pAppAdapter, CMD_CHK_LSO_V2);
            if (ret != STATUS_SUCCESSFUL) {
                //printf("LSO v2 test failed.\n");
                //goto exit;
            }
        }
    }
   
exit:
    gAthrDosappChar.Unload(gPDriver);
    
    gPDriver = NULL;
    return ret;
}


// handle the commands parsed from input
int handleCommands(IN PAPP_ADAPTER pAppAdapter)
{
    int  ret = STATUS_SUCCESSFUL;
    u32  cmdType = pAppAdapter->UserCmd;
    PVOID pDriverBak = gPDriver;
    u16  nicPos = 0;

    ret = gAthrDosappChar.Open(gAppAdapter.DevName);
    if (ret != STATUS_SUCCESSFUL)
    {
        printf("\nFailed to open device...\n");
        return ERR_FAILED_LOAD_DRIVER;
    }

    if (cmdType & CMD_CHK_PHY_LP)
    {
        ret = PhyLoopbackTest(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            printf("phy loopback test done\n");
            goto do_exit;
        }
        printf("phy loopback test done\n");
    }
    
    if (cmdType & CMD_CHK_CABLE_LP)
    {
        ret = CableLoopbackTest(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            printf("cable loopback test done\n");
            goto do_exit;
        }
        printf("cable loopback test done\n");
    }     

    if (cmdType & (CMD_CHK_XSUM_IPV4|CMD_CHK_XSUM_L4|CMD_CHK_CXSUM)) 
    {
        ret = XsumTest(pAppAdapter);   
        if (ret != STATUS_SUCCESSFUL) {
            goto do_exit;
        }
        printf("phy loopback checksum offload test done\n");
    }

    if (cmdType & (CMD_CHK_LSO_V1|CMD_CHK_LSO_V2))
    {
        ret = LsoTest(pAppAdapter);
        if (ret != STATUS_SUCCESSFUL) {
            goto do_exit;
        }
        printf("phy loopback large size offload test done\n");
    }

do_exit:
    if (gPDriver)
    {
        gAthrDosappChar.Unload(gPDriver);
    }

    gAthrDosappChar.Close();
    
    return ret;    
}

u32 loopback_test(u32 type)
{
    int ret = STATUS_SUCCESSFUL;
    
    /* Initialize global variables */
    memset(&gAthrDosappChar, 0, sizeof(gAthrDosappChar));
    memset(&gAppAdapter, 0, sizeof(gAppAdapter));
    
    /* free App alloc memory */
    freeAppMem(&gAppAdapter);
    
    // pass these fields to drivers and let driver fill other required fields.
    if ((gPDriver = RegisterNIC(&gAthrDosappChar)) == NULL)
    {
        printf("\nFailed to register NIC...");
        printf("%s", get_error_desc(ret));
        printf("\n");
        return ret;
    }

    /* realloc app memory */
    if ((ret = allocAppMem(&gAppAdapter)) != STATUS_SUCCESSFUL)
    {
        printf("\nFailed to allocate APP memory...");
        printf("%s", get_error_desc(ret));
        printf("\n");
        return ret;
    }
    
    // Do not adjust the sequence of these procedures. They must be done as 
    // 1st step: determine adapter, either specified by user or 1st scanned
    if ((ret = determineAdapter(&gAppAdapter)) != STATUS_SUCCESSFUL)
    {
            printf("\nFailed to determine which adapter to test...");
            printf("%s", get_error_desc(ret));
            printf("\n");
            return ret;
    }
    
    // 2nd step: get hw paraemters, mac address, vlanStrip ....
    if ((ret = appHwPara(&gAppAdapter)) != STATUS_SUCCESSFUL)
    {
        printf("\nAPP failed to handle HW parameters...");
        printf("%s", get_error_desc(ret));
        printf("\n");
        return ret;
    }
    
    // 3rd step: handle each command
    gAppAdapter.UserCmd = type;
    ret = handleCommands(&gAppAdapter);

    freeAppMem(&gAppAdapter); 

    return ret;
}

void dbgPktToFile(IN PATHR_DOS_PACKET pPkt)
{
    static int fileNum = 1000;

    int i = 0;
    FILE *fpAdd, *fpMem;
    char dbgAddrFile[20] = "RxAdd";
    char dbgMemFile[20] = "RxMem";
    char temp[10] = "";

    if(pPkt == NULL)
    {
        printf("The packet to be dumped to file is NULL.\n");
        return;
    }

    snprintf(temp, sizeof(temp), "%d", fileNum);
    strcat(dbgAddrFile, temp+1);    // "1xxx" -> "xxx"
    strcat(dbgAddrFile, ".dbg");

    fpAdd = fopen(dbgAddrFile, "wt+");
    fprintf(fpAdd, "Pkt Address         = %p\n", pPkt);
    fprintf(fpAdd, "Next Pkt Address    = %p\n", pPkt->Next);
    fprintf(fpAdd, "Pkt Type            = %08x", pPkt->Type);
    if (pPkt->Type & PKT_TYPE_EII)
    {
        fprintf(fpAdd, " EII");
    }
    if (pPkt->Type & PKT_TYPE_802_3)
    {
        fprintf(fpAdd, " 802.3");
    }
    if (pPkt->Type & PKT_TYPE_SNAP)
    {
        fprintf(fpAdd, " Snap");
    }
    if (pPkt->Type & PKT_TYPE_IPV4)
    {
        fprintf(fpAdd, " IPv4");
    }
    if (pPkt->Type & PKT_TYPE_IPV6)
    {
        fprintf(fpAdd, " IPv6");
    }
    if (pPkt->Type & PKT_TYPE_TCP)
    {
        fprintf(fpAdd, " TCP");
    }
    if (pPkt->Type & PKT_TYPE_UDP)
    {
        fprintf(fpAdd, " UDP");
    }
    if (pPkt->Type & PKT_TYPE_TO_IPXSUM)
    {
        fprintf(fpAdd, " IP_xsum_offload");
    }
    if (pPkt->Type & PKT_TYPE_TO_L4XSUM)
    {
        fprintf(fpAdd, " L4_xsum_offload");
    }
    if (pPkt->Type & PKT_TYPE_TO_TSOV1)
    {
        fprintf(fpAdd, " TSOv1");
    }
    if (pPkt->Type & PKT_TYPE_TO_TSOV2)
    {
        fprintf(fpAdd, " TSOv2");
    }
    if (pPkt->Type & PKT_TYPE_IPXSUM_ERR)
    {
        fprintf(fpAdd, " IP_xsum_err");
    }
    if (pPkt->Type & PKT_TYPE_L4XSUM_ERR)
    {
        fprintf(fpAdd, " L4_xsum_err");
    }
    if (pPkt->Type & PKT_TYPE_INCOMPLETE_ERR)
    {
        fprintf(fpAdd, " incomplete_err");
    }
    fprintf(fpAdd, "\n");
    fprintf(fpAdd, "Pkt Length          = %08x\n", pPkt->Length);
    fprintf(fpAdd, "Pkt VlanID          = %08x\n", pPkt->VlanID);
    fprintf(fpAdd, "Pkt MSS             = %08x\n", pPkt->MSS);
    fprintf(fpAdd, "Pkt hash            = %08x\n", pPkt->Hash);
    fprintf(fpAdd, "Pkt cpu num         = %08x\n", pPkt->CpuNum);
    fprintf(fpAdd, "Pkt xsum            = %08x\n", pPkt->Xsum);
    fprintf(fpAdd, "Pkt CsumStart       = %08x\n", pPkt->CsumStart);
    fprintf(fpAdd, "Pkt CsumPos         = %08x\n", pPkt->CsumPos);
    fprintf(fpAdd, "Pkt UpLeverResv:  [%p][%p][%p][%p]\n\n",
            pPkt->UpLevelReserved[0], pPkt->UpLevelReserved[1],
            pPkt->UpLevelReserved[2], pPkt->UpLevelReserved[3]);
//    fprintf(fpAdd, "Pkt LowLeverResv: [%08x][%08x][%08x][%08x]\n\n",
//            pPkt->LowLevelReserved[0], pPkt->LowLevelReserved[1],
//            pPkt->LowLevelReserved[2], pPkt->LowLevelReserved[3]);

    for (i = 0; i < MAX_PKT_BUF; i++)
    {
        fprintf(fpAdd, "Buf[%d].Addr        = %p\n", i, pPkt->Buf[i].Addr);
        fprintf(fpAdd, "Buf[%d].Len         = %08x\n", i, pPkt->Buf[i].Length);
        fprintf(fpAdd, "SGList[%d].Addr     = %p\n", i, pPkt->SGList[i].Addr);
        fprintf(fpAdd, "SGList[%d].Len      = %08x\n\n", i, pPkt->SGList[i].Length);
    }
    fclose(fpAdd);


    strcat(dbgMemFile, temp+1);    // "1xxx" -> "xxx"
    strcat(dbgMemFile, ".dbg");

    fpMem = fopen(dbgMemFile, "wb+");
    //fwrite(const void*buffer,size_t size,size_t count,FILE*stream);
    for (i = 0; i < MAX_PKT_BUF; i++)
    {
        fwrite((void *)(pPkt->Buf[i].Addr), 1, pPkt->Buf[i].Length, fpMem);
    }
    fclose(fpMem);

    fileNum++;
    return;
}
