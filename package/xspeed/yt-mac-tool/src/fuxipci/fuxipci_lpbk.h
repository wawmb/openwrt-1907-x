/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_lpbk.h

Abstract:

    FUXI loopback test function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __FUXIPCI_LPBK_H
#define __FUXIPCI_LPBK_H

//#define LOG_SENT_PKT
//#define LOG_SENT_PKT_START          0
//#define LOG_SENT_PKT_END            200
#define MAX_PACKET_COUNT            1
#define MAX_PKT_BUF                 1

#define MAX_DEVICE_COUNT             16

#define DEV_IOCTL_BIST          0x00000001L     /* do SRAM BIST test */
#define DEV_IOCTL_REGVAL        0x00000002L     /* do register reset/default value test */
#define DEV_IOCTL_SMBUS         0x00000004L     /* do SMBus test */
#define DEV_IOCTL_FLASH         0x00000008L     /* read/write external spi-flash */
#define DEV_IOCTL_EFUSE         0x00000010L     /* read/write efuse */
#define DEV_IOCTL_MEM_REG       0x00000020L     /* read/write register via memory mode */
#define DEV_IOCTL_IO_REG        0x00000040L     /* read/write register via io mode */
#define DEV_IOCTL_CFG_REG       0x00000080L     /* read/write register via configure mode */
#define DEV_IOCTL_PHYREG        0x00000100L     /* read/write phy register (0x1414 memory mode) */
#define DEV_IOCTL_PHYDBG        0x00000200L     /* read/write phy debug register */
#define DEV_IOCTL_SET_RSS_TBL   0x00000400L     /* set rss indirection table */
#define DEV_IOCTL_SET_RSS_KEY	0x00000800L		/* set rss algorithm key */
#define DEV_IOCTL_SET_PARAM		0x00001000L		/* set hardware parameter */
#define DEV_IOCTL_SET_RSS_BCPU	0x00002000L		/* set rss base cpu number */
#define DEV_IOCTL_GET_PARAM     0X00004000L     /* get hardware parameter */
#define DEV_IOCTL_PHY_EXTREG    0x00008000L     /* read/write phy extension register */

// definition of APP_ADAPTER.NicType
#define NICTYPE_L1C         0x01
#define NICTYPE_L1F         0x02
#define NICTYPE_GIGA        0x10   // Gigabit
#define NICTYPE_FAST        0x20   // Fast ethernet

// definition of APP_ADAPTER.enPacketType
#define TX_PKT_TYPE_NONE      0x0000
#define TX_PKT_TYPE_II        0x0001
#define TX_PKT_TYPE_802_3     0x0002
#define TX_PKT_TYPE_SNAP      0x0004
//#define TX_PKT_TYPE_NON_IP    0x0008
#define TX_PKT_TYPE_IPV4      0x0010
#define TX_PKT_TYPE_IPV6      0x0020
#define TX_PKT_TYPE_TCP       0x0040
#define TX_PKT_TYPE_UDP       0x0080
//#define TX_PKT_TYPE_FRAGMENT  0x0100 
#define TX_PKT_TYPE_FRAME     (TX_PKT_TYPE_II | TX_PKT_TYPE_802_3 | TX_PKT_TYPE_SNAP)
#define TX_PKT_TYPE_IP        TX_PKT_TYPE_IPV4 | TX_PKT_TYPE_IPV6
#define TX_PKT_TYPE_L4        TX_PKT_TYPE_TCP | TX_PKT_TYPE_UDP


#define LOAD_MODE_NORMAL         0x0000
#define LOAD_MODE_MAC_LOOPBACK   0x0001
#define LOAD_MODE_PHY_LOOPBACK   0x0002
#define LOAD_MODE_CABLE_LOOPBACK 0x0003

#define LINKMODE_MACLP_1000F            1
#define LINKMODE_MACLP_100F             2
#define LINKMODE_MACLP_100H             3
#define LINKMODE_MACLP_10F              4
#define LINKMODE_MACLP_10H              5

#define LINKMODE_PHYLP_1000F            0x10
#define LINKMODE_PHYLP_100F             0x20
#define LINKMODE_PHYLP_100H             0x30
#define LINKMODE_PHYLP_10F              0x40
#define LINKMODE_PHYLP_10H              0x50

#define LINKMODE_CBLLP_1000F            0xA0
#define LINKMODE_CBLLP_100F             0xB0
#define LINKMODE_CBLLP_100H             0xC0
#define LINKMODE_CBLLP_10F              0xD0
#define LINKMODE_CBLLP_10H              0xE0

#define LINKMODE_NORMAL_1000F           0x70
#define LINKMODE_NORMAL_100F            0x80

/* parameter: LinkCap */
#define LC_TYPE_10H      0x01
#define LC_TYPE_10F      0x02
#define LC_TYPE_100H     0x04
#define LC_TYPE_100F     0x08
#define LC_TYPE_1000F    0x10
#define LC_TYPE_ALL     (LC_TYPE_10H|LC_TYPE_10F|LC_TYPE_100H|LC_TYPE_100F|LC_TYPE_1000F)

#define PKT_FLAG_TO_IPXSUM          0x00000001L     /* do ip checksum offload, TO:task offload */
#define PKT_FLAG_TO_L4XSUM          0x00000002L     /* do tcp/udp checksum offload */
//#define PKT_FLAG_TO_VLANINST        0x00000004L     /* insert vlan tag */
#define PKT_FLAG_TO_TSOV1           0x00000008L     /* do tcp large send v1 */
#define PKT_FLAG_TO_TSOV2           0x00000010L     /* do tcp large send v2 */
#define PKT_FLAG_TO_CXSUM           0x00000020L     /* do custom checksum offload */
//#define PKT_FLAG_VLANTAGGED         0x00000040L     /* packet contain vlan tag */
//#define PKT_FLAG_IPV4               0x00000080L     /* ipv4 */
//#define PKT_FLAG_IPV6               0x00000100L     /* ipv6 */
//#define PKT_FLAG_TCP                0x00000200L     /* tcp */
//#define PKT_FLAG_UDP                0x00000400L     /* udp */
//#define PKT_FLAG_EII                0x00000800L     /* ethernet II */
//#define PKT_FLAG_802_3              0x00001000L     /* 802.3 */
//#define PKT_FLAG_SNAP               0x00002000L     /* 802.2/snap */
#define PKT_FLAG_FRAGMENT           0x00004000L     /* fragment ip packet */
//#define PKT_FLAG_SGLIST_VALID       0x00008000L     /* SGList valid */
//#define PKT_FLAG_HASH_VLAID         0x00010000L     /* Hash valid */
//#define PKT_FLAG_CPUNUM_VALID       0x00020000L     /* CpuNum valid */
//#define PKT_FLAG_XSUM_VALID         0x00040000L
#define PKT_FLAG_IPXSUM_ERR         0x00080000L
#define PKT_FLAG_L4XSUM_ERR         0x00100000L
#define PKT_FLAG_CALC_RSS           0x01000000L
#define PKT_FLAG_PTP_OVER_UDP       0x02000000L
#define PKT_FLAG_PTP_OVER_MAC       0x04000000L
#define PKT_FLAG_LLDP               0x08000000L
#define PKT_FLAG_ERR                0x10000000L
#define PKT_FLAG_ERRADDR            0x20000000L

#define PKT_TYPE_TO_IPXSUM          0x00000001L     /* do ip checksum offload, TO:task offload */
#define PKT_TYPE_TO_L4XSUM          0x00000002L     /* do tcp/udp checksum offload */
#define PKT_TYPE_TO_VLANINST        0x00000004L     /* insert vlan tag */
#define PKT_TYPE_TO_TSOV1           0x00000008L     /* do tcp large send v1 */
#define PKT_TYPE_TO_TSOV2           0x00000010L     /* do tcp large send v2 */
#define PKT_TYPE_TO_CXSUM           0x00000020L     /* do custom checksum offload */
#define PKT_TYPE_VLANTAGGED         0x00000040L     /* packet contain vlan tag */
#define PKT_TYPE_IPV4               0x00000080L     /* ipv4 */
#define PKT_TYPE_IPV6               0x00000100L     /* ipv6 */
#define PKT_TYPE_TCP                0x00000200L     /* tcp */
#define PKT_TYPE_UDP                0x00000400L     /* udp */
#define PKT_TYPE_EII                0x00000800L     /* ethernet II */
#define PKT_TYPE_802_3              0x00001000L     /* 802.3 */
#define PKT_TYPE_SNAP               0x00002000L     /* 802.2/snap */
#define PKT_TYPE_FRAGMENT           0x00004000L     /* fragment ip packet */
#define PKT_TYPE_SGLIST_VALID       0x00008000L     /* SGList valid */
#define PKT_TYPE_HASH_VLAID         0x00010000L     /* Hash valid */
#define PKT_TYPE_CPUNUM_VALID       0x00020000L     /* CpuNum valid */
#define PKT_TYPE_XSUM_VALID         0x00040000L
#define PKT_TYPE_IPXSUM_ERR         0x00080000L
#define PKT_TYPE_L4XSUM_ERR         0x00100000L
#define PKT_TYPE_802_3_LEN_ERR      0x00200000L
#define PKT_TYPE_INCOMPLETE_ERR     0x00400000L
#define PKT_TYPE_CRC_ERR            0x00800000L
#define PKT_TYPE_RX_ERR             0x01000000L
#define PKT_TYPE_PTP                0x02000000L     /* 1588 PTP */
#define PKT_TYPE_LLDP               0x04000000L     /* IEEE LLDP */


#define PKT_TYPE_TX_PARM_ERR        0x10000000L
#define PKT_TYPE_TX_CABLE_ERR       0x20000000L
#define PKT_TYPE_TX_BUSY_ERR        0x40000000L

#define EXT_HOP_BY_HOP          0
#define EXT_TCP                 6
#define EXT_UDP                 17
#define EXT_EPS_IPV6            41
#define EXT_ROUTING             43
#define EXT_FRAGMENT            44
#define EXT_ESP_SECU            50
#define EXT_AUTH                51
#define EXT_ICMPV6              58
#define EXT_NONE                59
#define EXT_DESTINE             60

#define MAX_LOOP_COUNT          200

#define STATUS_PACKET_EQUAL     0
#define STATUS_PACKET_INEQUAL   1
#define CMD_CHK_TCP         0x100  /* if L4 packet is Tcp packet,set 1; if L4 packet is Udp packet, set 0 */
#define CMD_CHK_VLAN        0x4000  /* check the vlan insert/strip function */
#define CMD_CHK_XSUM_IPV4   0x8000  /* check the ipv4 checksum function */
#define CMD_CHK_XSUM_L4     0x10000 /* check the level4(tcp/udp) checksum function */
#define CMD_CHK_LSO_V1      0x20000 /* check the large send offload v1 function */
#define CMD_CHK_LSO_V2      0x40000 /* check the large send offload v2 function */
#define CMD_CHK_RSS_INTX    0x80000 /* check the RSS function in INT_X mode */
#define CMD_CHK_CXSUM       0x100000 /* check the custom checksum function */
#define CMD_CHK_IO_REG      0x200000 /* -o */       
#define CMD_CHK_PTP         0x400000 /* check 1588 PTP frame */
#define CMD_CHK_LLDP        0x800000 /* check IEEE LLDP frame */
        
#define CMD_NONE            0       /* none */
#define CMD_LIST_ADAPTER    1       
#define CMD_CHK_REGISTER    2       
#define CMD_CHK_TWSI        4       
#define CMD_CHK_MDIO        8       
#define CMD_CHK_BIST        0x10    /* -b */
#define CMD_CHK_MAC_LP      0x20    /* -g mac*/ 
#define CMD_CHK_PHY_LP      0x40    /* -g phy*/
#define CMD_CHK_SMBUS       0x80    /* -s */
#define CMD_RUN_SCRIPT      0x200   /**/
#define CMD_HELP            0x1000  
#define CMD_CHK_CABLE_LP    0x2000  
//#define CMD_CHK_BIST_SW     0x2000  /**/

#define FIELD_OFFSET(type, field)    ((size_t)&(((type *)0)->field))    
#define MP_OFFSET(type, field)   ((int)FIELD_OFFSET(type, field))
#define MP_SIZE(type, field)     sizeof(((type*)0)->field)

#ifdef  USING_SEED
#define RAND()  seedRand()
#else
#define RAND()  rand()
#endif  //TIME_SEED

#define PKT_RX_STATUS           (\
        PKT_TYPE_IPXSUM_ERR     |\
        PKT_TYPE_L4XSUM_ERR     |\
        PKT_TYPE_802_3_LEN_ERR  |\
        PKT_TYPE_INCOMPLETE_ERR |\
        PKT_TYPE_CRC_ERR        )

#define PKT_TX_STATUS           (\
        PKT_TYPE_TX_PARM_ERR    |\
        PKT_TYPE_TX_CABLE_ERR   |\
        PKT_TYPE_TX_BUSY_ERR    )

typedef struct _APP_ADAPTER {
#if 1
    u8      NicType;            // L1c, L1f
    bool    SpecifiedByUser;    // Nic position (Bus/Dev/Func number) specified by user
                                // if not, use the 1st supported NIC
    u8      DispIndex;
    u16     DevId;
    u8      RevId;
    char    DevName[64];
    u32     UserCmd;            // Commands specified by end user
    u8*     pDmaVirAddr;
    u8*     pAlignedDmaVirAddr;
    char*   pDiagName;          // name of the diagnousitic program
    char*   pInitFileName;      // Init file to be passed to driver, driver related config
    char*   pOptScript;         // 

    u8   MacAddr[6];         // Local mac address
    u8   MacAddrDst[6];
    u16  MacProtocol;        // Protocol type in L2
    u8   IpAddr[16];         // Local IP address
    u8   IpAddrDst[16];
    u16  Port;               // Port number in transport layer

    // App parameters
    u32  LoopCount;
    bool bVerbose;       // Display operational messages
    bool bOneByOne;      // send only one packet each loop
    u8   LinkCapability;
    u16  enPacketType;
    u8   RssType;
    u16  BaseCpuNum;     // set when doing RSS test

    // some global parameters for MAC, these parameters 
    // should not be changed from packet to packet
    u16  MTU;            // get the MTU set by HW then set MSS accordingly
    u16  RxRingSize;
    u16  RxBufBlockSize;
    bool bVlanStrip;     // Strip the Vlan tag when receiving a tagged packet
    bool bLldpChkAddr;

    bool bRxChkIpXsum;
    bool bRxChkTcpXsum;
    bool bRxChkUdpXsum;
    bool bRxChkRss;

    // L1d[HW bug]: Rx Side, if 100M, IPv6, Packet Length > 2K, then Checksum calculate error.    
    bool bXsumBug;
    // < L1f-B[HW bug]: if IP header size not fixed, the IP xsum offload error.
    bool bIpXsumBug;

    //For debug only >>>
    u32  LoopIndex;
    u32  PktIndex;
    //<<<
#endif
} APP_ADAPTER, *PAPP_ADAPTER;

typedef struct _ATHR_DOS_BUF {
    u8* Addr;
    u32 Offset;
    u32 Length;
} ATHR_DOS_BUF, *PATHR_DOS_BUF;

typedef struct _ATHR_DOS_PACKET {
    struct _ATHR_DOS_PACKET* Next;
    u32                      Length;             /* total length of the packet(buffers) */
    u32                      Type;               /* packet type, vlan, ip checksum, TSO, etc. */

    ATHR_DOS_BUF             Buf[MAX_PKT_BUF];
    ATHR_DOS_BUF             SGList[MAX_PKT_BUF];
    u16                      VlanID;                                                         
    u16                      MSS;                                                            
    u32                      Hash;                                                           
    u16                      CpuNum;                                                         
    u16                      Xsum;               /* rx, ip-payload checksum */
    u16                      CsumStart;          /* custom checksum offset to the mac-header */
    u16                      CsumPos;            /* custom checksom position (to the mac_header) */                                          
    void*                    UpLevelReserved[4];                                                         
    void*                    LowLevelReserved[4];                                                            
} ATHR_DOS_PACKET, *PATHR_DOS_PACKET;

typedef struct _pci_dev_id {
    u16 ven_id;
    u16 dev_id;
} pci_dev_id;

typedef struct _RW_REG
{
    u8  bRead;
    u8  bDirect;
    u8  dev;         // only for R/W PHY extension register
    u16 reg;
    u32 val;
}RW_REG, *PRW_REG;

typedef struct _SET_DEV_PARAM {
    u8* name;
    u32 val;
} SET_DEV_PARAM, *PSET_DEV_PARAM;


enum VLAN_TYPE 
{
    vlan_type_none,
    vlan_type_insert,
    vlan_type_tagged,
    vlan_type_random
};
enum FRAME_TYPE 
{
    frame_type_ethernetII,
    frame_type_802_3_snap,
    frame_type_802_3,
    frame_type_random,
    frame_type_IIorSnap
};
enum L3_TYPE 
{
    //l3_type_none,
    l3_type_ipv4,
    l3_type_ipv6,
    l3_type_random,
    l3_type_none
};
enum L4_TYPE 
{
    //l4_type_none,
    l4_type_tcp,
    l4_type_udp,
    l4_type_random,
    l4_type_none
};

enum PTP_TYPE1
{
    //ptp_type_none,
    ptp_type_primary,
    ptp_type_pdelay
};

enum PTP_TYPE2
{
    //ptp_type_none,
    ptp_type_event,
    ptp_type_general
};

struct PTP_TYPE
{
    enum PTP_TYPE1 type1;
    enum PTP_TYPE2 type2;
};

typedef struct _GENERATE_PKT_TYPE {
    enum VLAN_TYPE  vlan_type;
    enum FRAME_TYPE frame_type;
    enum L3_TYPE    l3_type;
    enum L4_TYPE    l4_type;
    struct PTP_TYPE ptp_type;
    u32             flag;       //flag TSO v1/v2, ipxsum err .....
} GENERATE_PKT_TYPE, *PGENERATE_PKT_TYPE;

typedef u32 (*OpenDevice)(
        char                *DevName
        );


typedef u32 (*CloseDevice)(
        );
        


typedef u32 (*SendPackets)(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket
        );


typedef u32 (*IndicatePackets)(
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket,
        u16                 Atrrib
        );
        
typedef u32 (*ReturnPackets)(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket
        );
        
typedef u32 (*DevIoCtrl)(
        PVOID               Driver,
        u32                 CtrlCode,
        PVOID               pInBuf,
        u32                 InBufLen,
        PVOID               pOutBuf,
        u32                 OutBufLen
        );

typedef u32 (*UnloadNIC)(
        PVOID               Driver
        );

typedef u32 (*ResetNIC)(
        PVOID               Driver,
        bool                bActive
        );

typedef u32 (*GetPhyAddress)(
        PVOID               VirAddr
        );


typedef u32 (*CompleteSendPackets)(
        PATHR_DOS_PACKET    PacketArry,
        u16                 NumPacket
        );

typedef u16 (*ReceivePackets)(
        PVOID               Driver,
        PATHR_DOS_PACKET    PacketArray,
        u32                 Size
        );

typedef struct _ATHR_DOS_STATS {
    u32 TxOK;
    u32 RxOK;
    u32 RxCrc;
    u32 RxFrag;
    u32 TxAbort;
} ATHR_DOS_STATS, *PATHR_DOS_STATS;


typedef u32 (*GetStats)(
        PVOID               Driver,
        PATHR_DOS_STATS     Stats
        );

/* Load NIC, after load, the NIC should be active */
/* UINT16 NICPos  -- Bus-Dev-Fun number of NIC
[Bit15-Bit8] - Bus number, 
[Bit7-Bit3] - Device number, 
[Bit2-Bit0] - Function number */
typedef PVOID (*LoadNIC)(
        u16              NICPos,
        u16              Mode
        );
      
typedef PVOID (*EnableInterrupt)(
        PVOID               Driver,
        bool                bEnable
        );        
        

typedef struct _ATHR_DOSAPP_CHAR {

    /* following function ptrs is provided by DOS App,
       in general, they can't be called at Driver ISR */

    IndicatePackets     Indicate;       /* tell the underlying NIC recv interface of the DOS App */
    CompleteSendPackets SendComplete;   /* complete the sending packet */   
    GetPhyAddress       GetPhyAddr;     /* tell the underlying NIC to call this function to get phy-addr
                                           for TX-packet buffers */
    u8*                 InitFileName;   /* init file, contains the hw-specific configuration */

    /* following vars is provided by DOS NIC Driver */

    u8                  NicName[16];    /* return the name of the underlying NIC */
    u32                 DrvVer;         /* underlying driver version for NIC */
    OpenDevice          Open;
    CloseDevice         Close;
    SendPackets         Send;           /* return the xmit interface of the underlying NIC */
    ReceivePackets      Receive;
    UnloadNIC           Unload;         /* return the unload interface of the underlying NIC */
    GetStats            Stats;          /* return the stats interface of the underlying NIC */
    LoadNIC             Load;           /* return the Init interface of the NIC */
    ResetNIC            Reset;          /* return the Reset Interface of the NIC */
    ReturnPackets       ReturnPkts;     /* return the Return Interface of the NIC */
    DevIoCtrl           IoCtrl;         /* return the IoControl Interface of the NIC */
    EnableInterrupt     EnInt;
} ATHR_DOSAPP_CHAR, *PATHR_DOSAPP_CHAR;

struct param_entry {
    const char* name;
    int         format; // 0:decimal interger, 1:hex interger
    int         offset; // value offset to the adapter
    int         size;   // size (in bytes) of the variable
    int         def;
    int         min;
    int         max;
};


extern ATHR_DOSAPP_CHAR    gAthrDosappChar;
extern PVOID               gPDriver;
extern APP_ADAPTER         gAppAdapter;
extern PATHR_DOS_PACKET    gpRecvdPkt;    
extern u16                 gRecvdPktNum;  
extern u16                 gRecvdPktAttr;    
extern u32                 gBufSize;


int getL4PayloadOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset);
int athr_getch();
void dbgPktToFile(IN PATHR_DOS_PACKET pPkt);
PATHR_DOS_PACKET advancePackets(IN PATHR_DOS_PACKET pPkt, IN u16 num);
int athr_kbhit();
int generateRandomPacket(IN PAPP_ADAPTER pAppAdapter,
                               IN PGENERATE_PKT_TYPE pPacketType,
                               IN u8* pPktBuf,
                               OUT PATHR_DOS_PACKET *ppDosPacket);
PATHR_DOS_PACKET linkPackets(IN PATHR_DOS_PACKET pOldPacket, IN PATHR_DOS_PACKET pNewPacket);
int clearStat();
PATHR_DOS_PACKET freePackets(IN PATHR_DOS_PACKET pPkt, IN u32 num);
int dumpReg();
int cmpPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, IN u16 num);
int getIpXsumOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset);
int getL4XsumOffset(IN PATHR_DOS_PACKET pRx, OUT u16 *pBufIndex, OUT u16 *pBufOffset);
int getCsXsumOffset(IN u32 CsumPos, IN PATHR_DOS_PACKET pRx, 
                         OUT u16 *pBufIndex, OUT u16 *pBufOffset);

u32 loopback_test(u32 type);
void genIpAddr(PAPP_ADAPTER pAppAdapter, PGENERATE_PKT_TYPE pPktType);


#endif//__FUXILPBK_H
