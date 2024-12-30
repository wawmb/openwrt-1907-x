/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_lso.cpp

Abstract:

    FUXI large size offload function

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

#include "fuxipci_err.h"
#include "fuxipci_comm.h"
#include "fuxipci_lpbk.h"
int cmpLsoPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, 
                       IN u16 num, IN OUT u16 *pTxPktOff, OUT u16 *pTxPktStep)
{
    int i, j, ret;
    u16 bufIndex = 0;
    u16 bufOffset = 0;
    u16 TxPktOff, TxPktStep;
    u32 RxBufLen;
    u8 *pTxBuf, *pRxBuf, *pTxBufOrg;
    PATHR_DOS_PACKET pTxTmp, pRxTmp;

    pAppAdapter = pAppAdapter;
    j = 0;
    pTxTmp = pTx;
    pRxTmp = pRx;

    TxPktOff = *pTxPktOff;
    TxPktStep = 0;
    pTxBufOrg = (u8 *) pTxTmp->Buf[0].Addr;  // Tx packet's buf will be continuous

    for (i = 0; i < num; i++)
    {
        if (pTxTmp == NULL || pRxTmp == NULL) // no packet to compare
        {
            printf("NULL packet of TX or RX packet...\n");
            goto ErrRet;
        }

        if (pRxTmp->Type & PKT_RX_STATUS)
        {
            printf("Rx packet error status [%08x].\n", pRxTmp->Type & (u32)PKT_RX_STATUS);
            goto ErrRet;
        }

        if (((pTxTmp->Type & PKT_TYPE_TO_TSOV1) && 
             (pTxTmp->Type & PKT_TYPE_IPV4) && 
             (pTxTmp->Type & PKT_TYPE_TCP)) ||
            ((pTxTmp->Type & PKT_TYPE_TO_TSOV2) && 
             (pTxTmp->Type & (PKT_TYPE_IPV4|PKT_TYPE_IPV6)) && 
             (pTxTmp->Type & PKT_TYPE_TCP)))
        {
            ret = getL4PayloadOffset(pRxTmp, &bufIndex, &bufOffset);
            if (ret != STATUS_SUCCESSFUL)
            {
                printf("getL4PayloadOffset() return failed.\n");
                goto ErrRet;
            }
            
            if (TxPktOff == 0)
            {
                TxPktOff += bufOffset;  // buf Index always be 0
            }
            pTxBuf = pTxBufOrg + TxPktOff;

            
            for (j = 0; j < MAX_PKT_BUF; j++)
            {
                if (j == 0)  // 1st buf, skip the MAC Hdr, IP header, Tcp header
                {
                    pRxBuf = (u8 *) (pRxTmp->Buf[bufIndex].Addr);
                    pRxBuf += bufOffset;
                    RxBufLen = pRxTmp->Buf[bufIndex].Length - bufOffset;
                }
                else
                {
                    pRxBuf = (u8 *) (pRxTmp->Buf[j].Addr);
                    RxBufLen = pRxTmp->Buf[j].Length;
                }
                
                if (TxPktOff + RxBufLen > pTxTmp->Length)   // if the last TSO < 46 bytes, padding must be removed
                {
                    RxBufLen -= (TxPktOff + RxBufLen - pTxTmp->Length);
                }
                if (memcmp(pTxBuf, pRxBuf, RxBufLen) != 0)
                {
                    //printf("Packet contents not equal ...\n");
                    goto ErrRet;
                }
                TxPktOff += (u16)RxBufLen;
                if (TxPktOff == pTxTmp->Length)
                {
                    TxPktStep++;
                    TxPktOff = 0;
                    pTxTmp = pTxTmp->Next;
                    
                    if (pTxTmp == NULL)
                        goto NormalRet; //reach the end of TX packet lsit

                    pTxBufOrg = (u8 *) pTxTmp->Buf[0].Addr;  // Tx packet's buf will be continuous
                }
                pTxBuf = pTxBufOrg + TxPktOff;
            }
    
            pRxTmp = pRxTmp->Next;
        }
        else
        {
            printf("Cannot do TSO over this type of packet...\n");
            goto ErrRet;
        }
    }
    
NormalRet:        
    *pTxPktOff = TxPktOff;
    *pTxPktStep = TxPktStep;
    return STATUS_SUCCESSFUL;

ErrRet:
    //DBG_PRINT(("Tx pkt offset:[0x%04x]\n", TxPktOff));    
    //DBG_PRINT(("Rx pkt buf index:[%d]\n", j));    
    //DBG_PRINT(("Rx TCP payload buf index:[0x%04x], offset:[0x%04x]\n", bufIndex, bufOffset));    
    //DBG_PRINT(("Press anykey to error return...."));
    //WAIT_KEY();
#ifdef LOG_TSO_ERR_PKT   //DBG only
    logTsoRecvErrPkt(pTxTmp, pRx, num, (u16)i);
#endif //LOG_TSO_ERR_PKT   //DBG only

    return STATUS_PACKET_INEQUAL | (i << 4);
}

void logTsoRecvErrPkt(IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, 
                      IN u16 totalNum, IN u16 errIndex)
{
    int j = 0;
    
    printf("Compare Error at [%d] packet...(0 Based)\n", errIndex);

    printf("Start dumping...\n");
    printf("Address of Tx[%d]   = %p\t\t", j, pTx);
    printf("Tx[%d].Next         = %p\n", j, pTx->Next);
    printf("Tx[%d].Length       = %08x\t\t", j, pTx->Length);
    printf("Tx[%d].Type         = %08x\n", j, pTx->Type);
    //printf("Tx[%d].Buf[0].Addr  = %08x\n", j, pTx->Buf[0].Addr);
    //printf("Tx[%d].Buf[0].Length= %08x\n", j, pTx->Buf[0].Length);
    //printf("Tx[%d].Buf[1].Addr  = %08x\n", j, pTx->Buf[1].Addr);
    //printf("Tx[%d].Buf[1].Length= %08x\n", j, pTx->Buf[1].Length);
    //printf("Tx[%d].Buf[2].Addr  = %08x\n", j, pTx->Buf[2].Addr);
    //printf("Tx[%d].Buf[2].Length= %08x\n", j, pTx->Buf[2].Length);
    //printf("Tx[%d].Buf[3].Addr  = %08x\n", j, pTx->Buf[3].Addr);
    //printf("Tx[%d].Buf[3].Length= %08x\n", j, pTx->Buf[3].Length);

    printf("Press any key to dump TX packet[%d] to file...\n", j);
    athr_getch();
    dbgPktToFile(pTx);


    for (j = 0; j < totalNum; j++)
    {
        // if comparing error occurs at packet errIndex, then at most dump 3 packets [errIndex-1, errIndex+1]
        if (errIndex > 1 && 
            (j < errIndex-1 || j > errIndex+1))
        {
            pRx = advancePackets(pRx, 1);
            continue;
        }
        if (errIndex <= 1 && j > errIndex+1)
        {
            pRx = advancePackets(pRx, 1);
            continue;
        }

        printf("Address of Rx[%d]   = %p\t\t", j, pRx);
        if (pRx == NULL)
        {
            break;
        }
        
        printf("Rx[%d].Next         = %p\n", j, pRx->Next);
        printf("Rx[%d].Length       = %08x\t\t", j, pRx->Length);
        printf("Rx[%d].Type         = %08x\n", j, pRx->Type);
        //printf("Rx[%d].Buf[0].Addr  = %08x\n", j, pRx->Buf[0].Addr);
        //printf("Rx[%d].Buf[0].Length= %08x\n", j, pRx->Buf[0].Length);
        //printf("Rx[%d].Buf[1].Addr  = %08x\n", j, pRx->Buf[1].Addr);
        //printf("Rx[%d].Buf[1].Length= %08x\n", j, pRx->Buf[1].Length);
        //printf("Rx[%d].Buf[2].Addr  = %08x\n", j, pRx->Buf[2].Addr);
        //printf("Rx[%d].Buf[2].Length= %08x\n", j, pRx->Buf[2].Length);
        //printf("Rx[%d].Buf[3].Addr  = %08x\n", j, pRx->Buf[3].Addr);
        //printf("Rx[%d].Buf[3].Length= %08x\n", j, pRx->Buf[3].Length);

        printf("Press any key to dump RX packet[%d] to file...\n", j);
        athr_getch();
        dbgPktToFile(pRx);

        //printf("Advancing RX packet to next...\n");
        pRx = advancePackets(pRx, 1);
    }
    printf("Dump done...\n");
    return  ;
}

bool chkBufNeeded(PAPP_ADAPTER pAppAdapter, PATHR_DOS_PACKET pPkt, 
                  u32 *totalSplitPktCount, u32 *totalRxBufNeeded)
{
    bool ret = true;
    u32 segCount = 0;    // TCP segmentation count
    u16 bufIndex, payOffset;
    u32 segLength, lastSegLen;
    u32 bufNeeded1, bufNeeded2, tmp;

    if (getL4PayloadOffset(pPkt, &bufIndex, &payOffset) == STATUS_SUCCESSFUL)
    {
        segCount = ((pPkt->Length - payOffset) + (pPkt->MSS - 1)) / pPkt->MSS;
        segLength = payOffset + pPkt->MSS + 4;      // header + TCP data + CRC
        lastSegLen = pPkt->Length - pPkt->MSS * (segCount - 1) + 4;
        bufNeeded1 = (segLength + (pAppAdapter->RxBufBlockSize - 1))
                   / pAppAdapter->RxBufBlockSize;
        bufNeeded2 = (lastSegLen + (pAppAdapter->RxBufBlockSize - 1))
                   / pAppAdapter->RxBufBlockSize;
        
        tmp = *totalRxBufNeeded + bufNeeded1 * (segCount - 1) + bufNeeded2;
        if (tmp >= (u32)(pAppAdapter->RxRingSize * 99 / 100))  // leave 1%
        {
            return false;
        }
    }
    else
    {
        printf("getL4PayloadOffset() return failed.\n");
        return false;
    }

    *totalSplitPktCount += segCount;
    *totalRxBufNeeded = tmp;
    return ret;
}

int testLso(PAPP_ADAPTER pAppAdapter, u32 cmdType)
{
    u32  loopCount, pktCount, recvdPktCount, recvdSplitPktCount, recvCount; 
    u32  totalSplitPktCount, totalRxBufNeeded;
    u32  i, v32;
    u16  txPktOff, txPktStep;
    u16  num;
    int  ret = STATUS_SUCCESSFUL;
    time_t t0, t1;
    double pause;
    u8*  pBuf = NULL;
    PATHR_DOS_PACKET    pPktArry, pPktTmp, pPktSent;
    GENERATE_PKT_TYPE   pktType;

    //DBG_PRINT(("\n"));

    memset(&pktType, 0, sizeof(pktType));
    pktType.vlan_type = vlan_type_none;
    pktType.l4_type = l4_type_tcp;

    //for (loopCount = 0; loopCount < 2; loopCount++)
    recvCount = 0;
    for (loopCount = 0; loopCount < pAppAdapter->LoopCount; loopCount++)
    {
        //printf("\n\nFXG: send No.%d packet, pAppAdapter->LoopCount is %d!\n", loopCount, pAppAdapter->LoopCount);
        pBuf = pAppAdapter->pAlignedDmaVirAddr;
        pPktArry = pPktTmp = pPktSent = NULL;
        totalSplitPktCount = 0;
        totalRxBufNeeded = 0;

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

        pAppAdapter->LoopIndex = loopCount;

        // generate packets

        for (i = 0; i < pktCount; i++)
        {
            pAppAdapter->PktIndex = i;

            // onle generate Ethernet II  or SNAP frame randomly
            pktType.frame_type = frame_type_IIorSnap;
            pktType.flag = 0;       // clear the flag of last time

            // LSO v1 only can be done over IPv4, v2 can be done over either IPv4 or IPv6
            if (cmdType & CMD_CHK_LSO_V1)
            {
                pktType.frame_type = frame_type_ethernetII;
                pktType.l3_type = l3_type_ipv4;
                pktType.flag |= PKT_FLAG_TO_TSOV1;
            }
            if (cmdType & CMD_CHK_LSO_V2)
            {
                pktType.frame_type = frame_type_ethernetII;
                pktType.l3_type = l3_type_ipv4;
                pktType.flag |= PKT_FLAG_TO_TSOV2;
            }
            /* get ipaddr */
            genIpAddr(pAppAdapter, &pktType);
            
            /* generate packet */
            generateRandomPacket(pAppAdapter, &pktType, pBuf, &pPktTmp);

            if (chkBufNeeded(pAppAdapter, pPktTmp, &totalSplitPktCount, &totalRxBufNeeded) == false)
            {
                //DBG_PRINT(("reduce pkt count from %lu to %d.\n", pktCount, i));
                pktCount = i;       // do not generate any more packets
                free(pPktTmp);
                break;
            }

            v32 = pPktTmp->Length;
            v32 += 0xF;
            v32 &= 0xFFFFFFF0;
            pBuf += v32;

            pPktArry = linkPackets(pPktArry, pPktTmp);
        }

        // send packets
        if((loopCount + 1) % 20 == 0){
            DBG_PRINT(("**Tso loop[%d]** Send test packets ...\n", 
                    (int)loopCount + 1));
        }

        if (pktCount == 0)
        {
            continue;
        }

        clearStat();
        pPktSent = pPktArry;
        gAthrDosappChar.Send(gPDriver, pPktArry, (u16)pktCount);
        t0 = time(NULL);

        txPktOff = 0;
        recvdPktCount = 0;
        recvdSplitPktCount = 0;
        num = 0;
        //while (1)
        {
            memset(gpRecvdPkt, 0, gBufSize);
            usleep(50 * 1000);
            num = gAthrDosappChar.Receive(gPDriver, gpRecvdPkt, gBufSize);
            if (num != 0)
            {
                int dbgRet;
                recvdSplitPktCount += num;
                if ((dbgRet = cmpLsoPackets(pAppAdapter, pPktArry, gpRecvdPkt, num, &txPktOff, &txPktStep))
                    != STATUS_PACKET_EQUAL)
                {
                    dbgRet >>= 4;
                    //printf("ERROR: TSO inequal at pkt[%d-%d] (1 based)...\n", 
                    //       (int)(recvdPktCount+1), (int)(recvdSplitPktCount-num+1+dbgRet));
                    if (totalRxBufNeeded >= pAppAdapter->RxRingSize)
                    {
                        printf("\n* please make sure rx buffers are enough!\n");
                    }
                    ret = STATUS_PACKET_INEQUAL;
                }else{
                    recvCount++;
                }

                recvdPktCount += txPktStep;
                pPktArry = advancePackets(pPktArry, txPktStep);   //advancing the sending packet list

                if (recvdPktCount != pktCount)  // recieved packets already == xmitted packets
                {
                    //printf("current packet receive fail, recvdPktCount is %d, pktCount is %d\n", recvdPktCount, pktCount);
                }
            }else{
                //printf("receive current packet fail, receive packet num is %d .\n", num);
            }

            t1 = time(NULL);
            pause = difftime(t1, t0);
            if (pause > 10) 
            {   // 11sec
                printf("ERROR: timeout(%ds) waitting for recveiving all packets!\n", \
                    (int)(pause));
                ret = ERR_RX_TIMEOUT_LOOPBACK;
                
#ifdef  LOG_RECV_TIME_OUT
                DBG_PRINT(("Press any key to start dumping...\n"));
                WAIT_KEY();

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
            //DBG_PRINT(("Rx %u\n", recvdSplitPktCount));
            //break;
        }
    }

    printf("large size offload test send %d packets, recv %d packets.\n", pAppAdapter->LoopCount, recvCount);
    if(recvCount != pAppAdapter->LoopCount)
    {
        ret = STATUS_FAILED;
    }
    else{
        ret = STATUS_SUCCESSFUL;
    }
    //dumpReg();

    return ret;
}

