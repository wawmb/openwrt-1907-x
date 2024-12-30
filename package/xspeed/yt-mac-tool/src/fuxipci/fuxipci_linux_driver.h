/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_linux_driver.h

Abstract:

    FUXI Linux driver head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __FUXIPCI_LINUX_DRIVER_H__
#define __FUXIPCI_LINUX_DRIVER_H__

//#include <net/if.h> //for IFNAMSIZ

/**************************************************************************
 *          PCI BIOS Call Definition 
 *************************************************************************/
#define PCIE_PCI_FUNCTION_ID            0xB1
#define PCIE_PCI_BIOS_PRESENT           0x01
#define PCIE_FIND_PCI_DEVICE            0x02
#define PCIE_FIND_PCI_CLASS_CODE        0x03
#define PCIE_GENERATE_SPECIAL_CYCLE     0x06
#define PCIE_READ_CONFIG_BYTE           0x08
#define PCIE_READ_CONFIG_WORD           0x09
#define PCIE_READ_CONFIG_DWORD          0x0A
#define PCIE_WRITE_CONFIG_BYTE          0x0B
#define PCIE_WRITE_CONFIG_WORD          0x0C
#define PCIE_WRITE_CONFIG_DWORD         0x0D
#define PCIE_GET_IRQ_ROUTING_OPTIONS    0x0E
#define PCIE_SET_PCI_IRQ                0x0F

// PCI BIOS call return code:          
#define PCIE_EXIT_SUCCESSFUL            0x00
#define PCIE_EXIT_FUNC_NOT_SUPPORTED    0x81
#define PCIE_EXIT_BAD_VENDOR_ID         0x83
#define PCIE_EXIT_DEVICE_NOT_FOUND      0x86
#define PCIE_EXIT_BAD_REGISTER_NUMBER   0x87
#define PCIE_EXIT_SET_FAILED            0x88
#define PCIE_EXIT_BUFFER_TOO_SMALL      0x89

#define		MAX_DEVICE_COUNT			16

struct adapter
{
    char     if_name[IFNAMSIZ];
    int      sockfd;
    int      debugfs_fd;

    u16      DevID;
    u8       RevID;
    u8       PermAddr[6];
    u16      MaxFrameSize; /* mac-header+payload, exclude crc, vlan. */

    u16      Mode;              /* Normal, Mac loopback, Phy loopback */
    u8       LinkMode;
    u8       LinkCapability;
    bool     VlanStrip;
    bool     CrcAddedBySW;
    bool     Padding;
    bool     enDbgMode;
    bool     enBC;
    bool     enTxFC;
    bool     enRxFC;
    bool     enAllMulti;
    bool     rxchkRSS;
    bool     lldpChkAddr;

    bool     bRegCfged;
    u32      orgRXQ;
    u32      orgTXQ;

    // RSS parameters
    u8        HashType;
    u8        IdtTable[256];     /* RSS Indirection Table 4 - 8 - 16 - 32 - 64 - 128*/
    u16       IdtSize;           /* RSS Indirection Table Size */
    u16       BaseCpuNum;
    u8        HashSecretKey[40]; /* Secret hash key */
    u16       HashKeySize;
};

typedef struct adapter ADAPTER;
typedef ADAPTER * PADAPTER;
typedef PADAPTER PETHCONTEXT;

#define TX_RING_SZ_DEF      256
#define RX_RING_SZ_DEF      512
#define RX_BUFBLOCK_SZ_DEF  1536 /* 16bytes aligment */

#define TX_BUF_SZ_DEF       (8*1024*1024)

#define MTU_DEF             1514    /* mac-header+payload, exclude crc, vlan. */
#define MAX_INTNUM          2500 //5000    /* MaxIntPerSec */





#define RSS_TYPE_NONE           0x00
#define RSS_TYPE_IPV4           0x01
#define RSS_TYPE_IPV4_TCP       0x02
#define RSS_TYPE_IPV6           0x04
#define RSS_TYPE_IPV6_TCP       0x08

void AdapterDefault(PADAPTER pAdapter, char* paramFile);
void NicSetParam(PADAPTER pAdapter, PSET_DEV_PARAM pParam);
int NicGetParam(PADAPTER pAdapter, char* sParamName, PVOID pOutBuf, u32 *pOutBufLen);

int test_registers(PADAPTER pAdapter);
int test_smbus(PADAPTER pAdapter);


void NicRssIdt(PADAPTER pAdapter, u8* IdtTable, u16 IdtSize);
void NicRssKey(PADAPTER pAdapter, u8* RssKey, u16 KeySize);
void reconfig_rss(PADAPTER pAdapter);
bool HwInitPHY2(PADAPTER pAdapter, u8 LinkMode);
bool HwResetPHY2NormalMode(PADAPTER pAdapter);
bool HwSetPhyReg2CableLoopbackMode(PADAPTER pAdapter);

u32 Open_Linux(
        char  *DevName
        );
u32 Close_Linux();


u32 Send_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16              NumPacket
        );

u32 GetStats_Linux(
        PVOID               Driver,
        PATHR_DOS_STATS     Stats
        );

PVOID Load_Linux(
        u16              NICPos,
        u16              Mode
        );

u32 Unload_Linux(
        PVOID               Driver
        );

u32 Reset_Linux(
        PVOID               Driver,
        bool             bActive
        );

u32 ReturnPackets_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16              NumPacket
        );

u32 IoCtrl_Linux(
        PVOID               Driver,
        u32              CtrlCode,
        PVOID               pInBuf,
        u32              InBufLen,
        PVOID               pOutBuf,
        u32              OutBufLen
        );

PVOID SetIntMask_Linux(
        PVOID               Driver,
        bool             bEnable
        );

PVOID EnableInterrupt_Linux(
        PVOID               Driver,
        bool             bEnable
        );

u16 Receive_Linux(
        PVOID               Driver,
        PATHR_DOS_PACKET    pPacketArray,
        u32                 size
        );

#endif /*__L1C_DRV_H__*/

