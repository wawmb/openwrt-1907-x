/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_rss.cpp

Abstract:

    FUXI rss function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/
                
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>
#include <net/if.h>

#include "fuxipci_err.h"
#include "fuxipci_comm.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_rss.h"

// default secret key used by both SW and HW, this key can be changed later
u8 rss_key[40] = 
{
    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
    0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
    0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
    0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
    0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa
};

// our HW supports up to 256 bytes indirection table now, and HASH mask is 8 bits
u8  rssIndirTbl[RSS_INDIR_TBL_SIZE];  

int genRandRssTbl( )
{
    int i;
    DBG_PRINT(("RSS idt Table:"));
    for (i = 0; i < RSS_INDIR_TBL_SIZE; i++)
    {
        rssIndirTbl[i] = (u8) RAND() % RSS_MAX_CPU_NUM;
        if (i%16 == 0) {DBG_PRINT(("\n"));}
        else if (i%8 == 0) {DBG_PRINT(("- "));}
        DBG_PRINT(("%d ", rssIndirTbl[i]));
    }
    DBG_PRINT(("\n"));
    return STATUS_SUCCESSFUL;
}

int genRandRsskey( )
{
    int i;
    for (i = 0; i < 40; i++)
    {
        rss_key[i] = (u8) RAND();
    }
    return STATUS_SUCCESSFUL;
}


int initRss(PAPP_ADAPTER pAppAdapter)
{
    int  ret = STATUS_SUCCESSFUL;
    u16  rssBaseCpuNum = pAppAdapter->BaseCpuNum;
    
    SET_DEV_PARAM setRssPara1 = {(u8 *)"RssEnable", true};
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setRssPara1, sizeof(setRssPara1), NULL, 0);
    if (ret != 0) {
        return ret; 
    }

    // use default Toeplitz hash function and default secret key
    // do not have to set this key to HW now
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_RSS_KEY, rss_key, sizeof(rss_key), NULL, 0);
    if (ret != 0) {
        return ret; 
    }
    
    // use random indirction table
    genRandRssTbl();
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_RSS_TBL, rssIndirTbl, RSS_INDIR_TBL_SIZE, NULL, 0);
    if (ret != 0) {
        return ret; 
    }
    
    // use default base cpu number
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_RSS_BCPU, &rssBaseCpuNum, sizeof(rssBaseCpuNum), NULL, 0);
    if (ret != 0) {
        return ret; 
    }

    // use default hash mask 7 bits ???
    // do not have to set this key to HW now

    // hash type -- ipv4, ipv4-tcp, ipv6, ipv6-tcp  ???????
    // This should be moved out from initRss since it's a golabl HW parameter, should only changed upon every HW reset.
    SET_DEV_PARAM setRssPara = {(u8 *)"RssType", pAppAdapter->RssType};
    ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_SET_PARAM, &setRssPara, sizeof(setRssPara), NULL, 0);
    if (ret != 0) {
        return ret; 
    }
    
    return ret;
}

u16 rssGetCpuNum(u32 hashVal, u16 BaseCpuNum)
{
    return rssIndirTbl[hashVal & RSS_HASH_MASK] + BaseCpuNum;
}


void shift_key(u8* pkey, int nkey)
{
    int len = nkey;
    int i;
    u8 carry = 0;

    for (i=len-1; i>=0; i--)
    {
        if (pkey[i]&0x80)
        {
            pkey[i] = (pkey[i] << 1) | carry;
            carry = 1;
        }
        else
        {
            pkey[i] = (pkey[i] << 1) | carry;
            carry = 0;
        }
    }
}

u32 get_most32_key(u8* pkey)
{
    u8 value[4];
    value[0] = pkey[3];
    value[1] = pkey[2];
    value[2] = pkey[1];
    value[3] = pkey[0];
    return *(u32 *)value;
}

u32 cal_hash(u8* input, u32 length)
{
    u32 index;
    u32 result = 0;
    u8  key[40];

    int j;

    memcpy(key, rss_key, 40);

    for (index=0; index<length; index++)
    {
        for (j=7; j>=0; j--)
        {
            if (input[index] & (1 << j))
            {
                result ^= get_most32_key(key);
            }
            shift_key(key, 40);
        }
    }
    return result;
}

u32 calHashIpv4Tcp(u8* srcIp, u8* destIp, u16 srcPort, u16 destPort)
{
    u8 input[12];
    srcPort = htons(srcPort);
    destPort = htons(destPort);
    
    memcpy(input, srcIp, 4);
    memcpy(input+4, destIp, 4);
    memcpy(input+8, (u8 *)&srcPort, 2);
    memcpy(input+10, (u8 *)&destPort, 2);


    return cal_hash(input, 12);
}


u32 calHashIpv4(u8* srcIp, u8* destIp)
{
    u8 input[8];

    memcpy(input, srcIp, 4);
    memcpy(input+4, destIp, 4);


    return cal_hash(input, 8);
}


u32 calHashIpv6Tcp(u8* srcIp, u8* destIp, u16 srcPort, u16 destPort)
{
    u8 input[36];
    srcPort = htons(srcPort);
    destPort = htons(destPort);

    memcpy(input, srcIp, 16);
    memcpy(input+16, destIp, 16);
    memcpy(input+32, (u8 *)&srcPort, 2);
    memcpy(input+34, (u8 *)&destPort, 2);


    return cal_hash(input, 36);
}


u32 calHashIpv6(u8* srcIp, u8* destIp)
{
    u8 input[32];

    memcpy(input, srcIp, 16);
    memcpy(input+16, destIp, 16);


    return cal_hash(input, 32);
}

int testRss(PAPP_ADAPTER pAppAdapter)
{
    u32  loopCount, pktCount, recvdPktCount; 
    u32  i, v32;
    u16  num;
    int  ret = STATUS_SUCCESSFUL;
    u8*  pBuf = NULL;
    time_t t0, t1;
    double pause;
    PATHR_DOS_PACKET    pPktArry, pPktTmp, pPktSent;
    GENERATE_PKT_TYPE   pktType;

    memset(&pktType, 0, sizeof(pktType));
    pktType.vlan_type = vlan_type_none;
    pktType.flag |= PKT_FLAG_CALC_RSS;

    for (loopCount = 0; loopCount < pAppAdapter->LoopCount; loopCount++)
    {
        pBuf = pAppAdapter->pAlignedDmaVirAddr;
        pPktArry = pPktTmp = pPktSent = NULL;

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

            // onle generate Ethernet II  or SNAP frame randomly
            pktType.frame_type = frame_type_IIorSnap;
            pktType.l3_type = l3_type_random;
            pktType.l4_type = l4_type_random;

            generateRandomPacket(pAppAdapter, &pktType, pBuf, &pPktTmp);
            v32 = pPktTmp->Length;
            v32 += 0xF;
            v32 &= 0xFFFFFFF0;
            pBuf += v32;

            pPktArry = linkPackets(pPktArry, pPktTmp);
        }

        // send packets
        DBG_PRINT(("**Rss loop[%d](0x%08x)** Sending %d(0x%02x) packets...\n", 
                loopCount, loopCount, pktCount, pktCount));

        pPktSent = pPktArry;
        gAthrDosappChar.Send(gPDriver, pPktArry, (u16)pktCount);
        t0 = time(NULL);

        // recieve packet by polling the value of gRecvdPktNum 
        recvdPktCount = 0;
        num = 0;
        while (1)
        {
            num = gAthrDosappChar.Receive(gPDriver, gpRecvdPkt, gBufSize);
            if (num != 0)
            {
                //printf("Recieved %d packets ...", u16);  // for DBG only
                //printf("gpRecvdPkt = %08x ...", gpRecvdPkt);  // for DBG only
#if 0
                int i = 0; PATHR_DOS_PACKET pDbg = gpRecvdPkt;
                for (i = 0; i < num; i++)
                {
                    dbgPktToFile(pDbg);
                    pDbg = advancePackets(pDbg, 1);
                }
#endif
                int dbgRet;
                recvdPktCount += num;
                if ((dbgRet = cmpRssPackets(pAppAdapter, pPktArry, gpRecvdPkt, num)) != STATUS_PACKET_EQUAL)
                {
                    dbgRet >>= 4;
                    printf("ERROR: Inequal at pkt[%d] (1 based)...", (int)(recvdPktCount-num+1+dbgRet));
                    ret = STATUS_PACKET_INEQUAL;
                    break;
                }

                pPktArry = advancePackets(pPktArry, num);   //advancing the sending packet list

                if (recvdPktCount == pktCount)  // recieved packets already == xmitted packets
                {
                    break;
                }
            }

            t1 = time(NULL);
            pause = difftime(t1, t0);
            if (pause > 5) 
            {   // 6sec
            	printf("ERROR: timeout(%ds) waitting for recveiving all packets!\n", \
    				(int)(pause));
            	ret = ERR_RX_TIMEOUT_LOOPBACK;
                
#ifdef  LOG_RECV_TIME_OUT
                DBG_PRINT(("Start dumping...\n"));
                dbgDumpPktInfo(pPktArry);
                dbgPktToFile(pPktArry);
#endif  //LOG_RECV_TIME_OUT

            	break;
            }
        }     

        // free the packet list random generated for sending
        freePackets(pPktSent, pktCount);

        if (ret != STATUS_SUCCESSFUL)   // if failed in any loop, do not do loop any more
            break;        
    }

    return ret;
}

int cmpRssPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, IN u16 num)
{
    int i = 0;
    int ret;
    PATHR_DOS_PACKET pTxTmp, pRxTmp;
    pTxTmp = pTx;
    pRxTmp = pRx;
    
    ret = cmpPackets(pAppAdapter, pTx, pRx, num);
    if (ret != STATUS_SUCCESSFUL)
        return ret;

    for (i = 0; i < num; i++)
    {
        if (pTxTmp == NULL || pRxTmp == NULL) // no packet to compare
        {
            goto ErrRet;
        }

        if (pTxTmp->Hash != pRxTmp->Hash)
        {
            RW_REG rwReg = {0};
            printf("Hash Value inequal (%08X - %08X)... ", pTxTmp->Hash, pRxTmp->Hash);

            rwReg.reg = 0x15B0; // hash val
            rwReg.bRead = true;
            ret = gAthrDosappChar.IoCtrl(gPDriver, DEV_IOCTL_MEM_REG, &rwReg, sizeof(rwReg), NULL, 0);
            printf("Reg 0x15B0=%08X... ", rwReg.val);

            goto ErrRet;
        }

        if (pTxTmp->CpuNum != pRxTmp->CpuNum)
        {
            printf("CPU num inequal...");
            goto ErrRet;
        }

        pTxTmp = pTxTmp->Next;
        pRxTmp = pRxTmp->Next;
    }

    return STATUS_SUCCESSFUL;
    
ErrRet:

#ifdef LOG_RECV_ERR_PKT   //DBG only
logRecvErrPkt(pTx, pRx, num, (u16)i);
#endif //LOG_RECV_ERR_PKT   //DBG only

    return STATUS_PACKET_INEQUAL | (i << 4);
}


