/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_xsum.h

Abstract:

    FUXI checksum head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/
                
#ifndef __APP_XSUM_H__
#define __APP_XSUM_H__


int cmpXsumPackets(IN PAPP_ADAPTER pAppAdapter, 
                        IN PATHR_DOS_PACKET pTx, 
                        IN PATHR_DOS_PACKET pRx, 
                        IN u16 num);
int testXsum(PAPP_ADAPTER pAppAdapter, u32 cmdType);


#endif/*__APP_XSUM_H__*/
