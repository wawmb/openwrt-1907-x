/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_lso.h

Abstract:

    FUXI large size offload head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/
                
#ifndef __APP_LSO_H__
#define __APP_LSO_H__

#define MAX_MSS_EII     4000
#define MIN_MSS_EII     600
#define MAX_MSS_SNAP    1400
#define MIN_MSS_SNAP    20

int cmpLsoPackets(IN PAPP_ADAPTER pAppAdapter, IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, 
                       IN u16 num, IN OUT u16 *pTxPktOff, OUT u16 *pTxPktStep);
int testLso(PAPP_ADAPTER pAppAdapter, u32 cmdType);
void logTsoRecvErrPkt(IN PATHR_DOS_PACKET pTx, IN PATHR_DOS_PACKET pRx, 
                      IN u16 totalNum, IN u16 errIndex);


#endif/*__APP_LSO_H__*/
