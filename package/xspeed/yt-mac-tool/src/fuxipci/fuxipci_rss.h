/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_rss.h

Abstract:

    FUXI rss head file.

Environment:
    
    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/
                
#ifndef __APP_RSS_H__
#define __APP_RSS_H__

#define RSS_TYPE_NONE           0x00
#define RSS_TYPE_IPV4           0x01
#define RSS_TYPE_IPV4_TCP       0x02
#define RSS_TYPE_IPV6           0x04
#define RSS_TYPE_IPV6_TCP       0x08


#define RSS_MAX_CPU_NUM     8
#define RSS_INDIR_TBL_SIZE  128     // 2 ^ 7
#define RSS_HASH_MASK       0x7F    // 7 bits

extern  u8  rssIndirTbl[RSS_INDIR_TBL_SIZE];

int initRss(PAPP_ADAPTER pAppAdapter);
int testRss(PAPP_ADAPTER pAppAdapter);
u16 rssGetCpuNum(u32 hashVal, u16 BaseCpuNum);
u32 calHashIpv4Tcp(u8* srcIp, u8* destIp, u16 srcPort, u16 destPort);
u32 calHashIpv4(u8* srcIp, u8* destIp);
u32 calHashIpv6Tcp(u8* srcIp, u8* destIp, u16 srcPort, u16 destPort);
u32 calHashIpv6(u8* srcIp, u8* destIp);
int cmpRssPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, IN u16 num);



#endif/*__APP_RSS_H__*/
