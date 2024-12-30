/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_xsum.cpp

Abstract:

    FUXI checksum function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <net/if.h>

#include "fuxipci_comm.h"
#include "fuxipci_err.h"
#include "fuxipci_lpbk.h"
#include "fuxipci_xsum.h"

int cmpXsumPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, IN u16 num)
{
    int i = 0;
    int ret;
    u16 bufIndex, bufOffset;
    u8 *p;
    u16 txXsum, rxXsum;
    PATHR_DOS_PACKET pTxTmp, pRxTmp;

    ret = cmpPackets(pAppAdapter, pTx, pRx, num);
    if (ret != STATUS_SUCCESSFUL)
        return ret;

    pTxTmp = pTx;
    pRxTmp = pRx;

    for (i = 0; i < num; i++)
    {
        if (pTxTmp == NULL || pRxTmp == NULL) // no packet to compare
        {
            printf("NULL packet of TX or RX packet...\n");
            goto ErrRet;
        }

        if (pTxTmp->Type & PKT_TYPE_TO_IPXSUM)
        {
            if (pTxTmp->Type & PKT_TYPE_IPV4)
            {
                if (getIpXsumOffset(pRxTmp, &bufIndex, &bufOffset) == STATUS_SUCCESSFUL)
                {
                    p = (u8 *) pRxTmp->Buf[bufIndex].Addr;
                    p += bufOffset;
                    rxXsum = *((u16 *)p); // no ntohs ????
                    txXsum = (u16) ((size_t) pTxTmp->UpLevelReserved[0]);
                    if (txXsum != rxXsum)
                    {
                        printf("IPv4 xsum inequal: %04x <--> %04x\n", txXsum, rxXsum);
                        goto ErrRet;
                    }
                }
                else
                {
                    printf("Getting IPv4 xsum from Rx packet fails\n");
                    goto ErrRet;
                }
            }
            else
            {
                printf("Cannot check IP xsum function for non-ipv4 packet...\n");
                goto ErrRet;
            }
        }
        
        if (pTxTmp->Type & PKT_TYPE_TO_L4XSUM)
        {
            if (pTxTmp->Type & PKT_TYPE_UDP || pTxTmp->Type & PKT_TYPE_TCP)
            {
                if (getL4XsumOffset(pRxTmp, &bufIndex, &bufOffset) == STATUS_SUCCESSFUL)
                {
                    //printf("L4Xsum[bufIndex=%d, bufOffset=%d ", bufIndex, bufOffset);   //DBG only
                    p = (u8 *) pRxTmp->Buf[bufIndex].Addr;
                    p += bufOffset;
                    rxXsum = *((u16 *)p); // no ntohs ????
                    txXsum = (u16) ((size_t) pTxTmp->UpLevelReserved[1]);
                    if (txXsum != rxXsum)
                    {
                        printf("L4 xsum inequal: %04x <--> %04x\n", txXsum, rxXsum);
                        goto ErrRet;
                    }
                }
                else
                {
                    printf("Getting L4 xsum from Rx packet fails\n");
                    goto ErrRet;
                }
            }
            else
            {
                printf("Cannot check L4 xsum function for non-tcp/udp packet...\n");
                goto ErrRet;
            }
        }

        if (pTxTmp->Type & PKT_TYPE_TO_CXSUM)
        {
            // cannot do cs xsum on these packets, skip over them
            if ((pTxTmp->CsumPos == 0xFFFF) || (pTxTmp->CsumStart == 0xFFFF))  
            {
                pTxTmp = pTxTmp->Next;
                pRxTmp = pRxTmp->Next;
                continue;
            }
            getCsXsumOffset(pTxTmp->CsumPos, pRxTmp, &bufIndex, &bufOffset);
            p = (u8 *) pRxTmp->Buf[bufIndex].Addr;
            p += bufOffset;
            rxXsum = *((u16 *)p); // no ntohs ????
            txXsum = (u16) ((size_t) pTxTmp->UpLevelReserved[0]);
            if (txXsum != rxXsum)
            {
                printf("Custom xsum inequal: %04x <--> %04x\n", txXsum, rxXsum);
                goto ErrRet;
            }
        }

        pTxTmp = pTxTmp->Next;
        pRxTmp = pRxTmp->Next;
    }
        
    return STATUS_SUCCESSFUL;
    //return cmpPackets(pAppAdapter, pTx, pRx, num);

ErrRet:

#ifdef LOG_RECV_ERR_PKT   //DBG only
logRecvErrPkt(pTx, pRx, num, (u16)i);
#endif //LOG_RECV_ERR_PKT   //DBG only

    return STATUS_PACKET_INEQUAL | (i << 4);
}

int testXsum(PAPP_ADAPTER pAppAdapter, u32 cmdType)
{
    u32  loopCount, pktCount, recvdPktCount, recvCount; 
    int  ret = STATUS_SUCCESSFUL;
    u32  i, v32;
    u16  num;
    u8*  pBuf = NULL;
    time_t t0, t1;
    double pause;
    PATHR_DOS_PACKET    pPktArry, pPktTmp, pPktSent;
    GENERATE_PKT_TYPE   pktType;
    enum L4_TYPE l4Type;

    /* get l4 packet type */
    if(cmdType & CMD_CHK_TCP){
        l4Type = l4_type_tcp;
    }
    else{
        l4Type = l4_type_udp;
    }

    pktType.vlan_type = vlan_type_none;
    recvCount = 0;
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
            if (0x1B == key) {
                ret = ERR_USER_BREAK;
                break;
            }
        }

        pAppAdapter->LoopIndex = loopCount;

        // generate packets
        for (i = 0; i < pktCount; i++)
        {
            pAppAdapter->PktIndex = i;

            pktType.flag = 0;       // clear the flag of last time

            if (cmdType & CMD_CHK_XSUM_L4)
            {
                pktType.frame_type = frame_type_ethernetII;
                pktType.l3_type = l3_type_ipv4;
                pktType.l4_type = l4Type;
                pktType.flag |= PKT_FLAG_TO_L4XSUM;
            }
            // if do IPv4 checksum, only generate ipv4 packets, ipv6 has no xsum field
            if (cmdType & CMD_CHK_XSUM_IPV4)
            {
                pktType.frame_type = frame_type_ethernetII;
                pktType.l3_type = l3_type_ipv4;
                pktType.l4_type = l4Type;
                pktType.flag |= PKT_FLAG_TO_IPXSUM;
            }
            if (cmdType & CMD_CHK_CXSUM)
            {
                pktType.frame_type = frame_type_random;
                pktType.l3_type = l3_type_none;
                pktType.l4_type = l4Type;
                pktType.flag |= PKT_FLAG_TO_CXSUM;
            }

            /* get ipaddr */
            genIpAddr(pAppAdapter, &pktType);

            /* generate packet */
            generateRandomPacket(pAppAdapter, &pktType, pBuf, &pPktTmp);

            v32 = pPktTmp->Length;
            v32 += 0xF;
            v32 &= 0xFFFFFFF0;
            pBuf += v32;

            pPktArry = linkPackets(pPktArry, pPktTmp);
        }

        // send packets
        if((loopCount + 1) % 20 == 0){
            DBG_PRINT(("**Xsum loop[%d]** Sending test packets...\n", 
                loopCount + 1));
        }
        pPktSent = pPktArry;
        gAthrDosappChar.Send(gPDriver, pPktArry, (u16)pktCount);
        t0 = time(NULL);

        recvdPktCount = 0;
        num = 0;
        //while (1)
        {
            memset(gpRecvdPkt, 0, gBufSize);
            usleep(50 * 1000);
            num = gAthrDosappChar.Receive(gPDriver, gpRecvdPkt, gBufSize);
            if (num != 0)
            {
                int dbgRet;
                recvdPktCount += num;
                if ((dbgRet = cmpXsumPackets(pAppAdapter, pPktArry, gpRecvdPkt, num))
                    != STATUS_PACKET_EQUAL)
                {
                    dbgRet >>= 4;
                    //printf("ERROR: XSUM inequal at pkt[%d] (1 based)...\n", 
                    //    (int)(recvdPktCount-num+1+dbgRet));
                    //ret = STATUS_PACKET_INEQUAL;
                }else{
                    recvCount++;
                }

                pPktArry = advancePackets(pPktArry, num);   //advancing the sending packet list

                if (recvdPktCount != pktCount)  // recieved packets already == xmitted packets
                {
                    printf("FXG: recvdPktCount is %d, pktCount is %d \n", recvdPktCount,     pktCount);
                }
            }
            else{
                //printf("recieve loopback data fail and has received %d packets.\n", recvCount);
                //ret = STATUS_FAILED;
            }

            t1 = time(NULL);
            pause = difftime(t1, t0);
            if (pause > 5) 
            {   // 6sec
                printf("ERROR: timeout(%ds) waitting for recveiving all packets!\n", \
                    (int)(pause));
                ret = ERR_RX_TIMEOUT_LOOPBACK;
                break;
            }
        }
        

        // free the packet list random generated for sending
        freePackets(pPktSent, pktCount);

        if (ret != STATUS_SUCCESSFUL)   // if failed in any loop, do not do loop any more
        {
            //WAIT_KEY();
            //break;
        }
    }
    printf("checksum offload test send %d packets, recv %d packets.\n", pAppAdapter->LoopCount, recvCount);
    if(recvCount != pAppAdapter->LoopCount)
    {
        ret = STATUS_FAILED;
    }
    else{
        ret = STATUS_SUCCESSFUL;
    }

    return ret;
}

