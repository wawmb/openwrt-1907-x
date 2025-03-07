/*===========================================================================
FILE:
   QMI.h

DESCRIPTION:
   Qualcomm QMI driver header
   
FUNCTIONS:
   Generic QMUX functions
      ParseQMUX
      FillQMUX
   
   Generic QMI functions
      GetTLV
      ValidQMIMessage
      GetQMIMessageID

   Get sizes of buffers needed by QMI requests
      QMUXHeaderSize
      QMICTLGetClientIDReqSize
      QMICTLReleaseClientIDReqSize
      QMICTLReadyReqSize
      QMIWDSSetEventReportReqSize
      QMIWDSGetPKGSRVCStatusReqSize
      QMIDMSGetMEIDReqSize
      QMICTLSyncReqSize

   Fill Buffers with QMI requests
      QMICTLGetClientIDReq
      QMICTLReleaseClientIDReq
      QMICTLReadyReq
      QMIWDSSetEventReportReq
      QMIWDSGetPKGSRVCStatusReq
      QMIDMSGetMEIDReq
      QMICTLSetDataFormatReq
      QMICTLSyncReq
      
   Parse data from QMI responses
      QMICTLGetClientIDResp
      QMICTLReleaseClientIDResp
      QMIWDSEventResp
      QMIDMSGetMEIDResp

Copyright (c) 2011, Code Aurora Forum. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Code Aurora Forum nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
===========================================================================*/

#pragma once

/*=========================================================================*/
// Definitions
/*=========================================================================*/
#include "Structs.h"

extern int fibo_debug;
// DBG macro
enum G_LOG_LEVEL {
    G_LOG_ERR = 1,
    G_LOG_INFO,
    G_LOG_MAIN,
    G_LOG_DEBUG,
};

#define G_ERR( format, arg... ) do { \
    if (fibo_debug >= G_LOG_ERR)\
    { \
        printk( KERN_ERR "GobiNet::%s " format, __FUNCTION__, ## arg ); \
    } }while(0)

#define G_INFO( format, arg... ) do { \
    if (fibo_debug >= G_LOG_INFO)\
    { \
        printk( KERN_DEBUG "GobiNet::%s " format, __FUNCTION__, ## arg ); \
    } }while(0)

#define G_MAIN( format, arg... ) do { \
    if (fibo_debug >= G_LOG_MAIN)\
    { \
        printk( KERN_INFO "GobiNet::%s " format, __FUNCTION__, ## arg ); \
    } }while(0)

#define G_DEBUG( format, arg... ) do { \
    if (fibo_debug >= G_LOG_DEBUG)\
    { \
        printk( KERN_INFO "GobiNet::%s " format, __FUNCTION__, ## arg ); \
    } }while(0)


// QMI Service Types
#define QMICTL 0
#define QMIWDS 1
#define QMIDMS 2
#define QMINAS 3
#define QMIUIM 11
#define QMIWDA 0x1A

#define u8        unsigned char
#define u16       unsigned short
#define u32       unsigned int
#define u64       unsigned long long

#define bool      u8
#define true      1
#define false     0

#define ENOMEM    12
#define EFAULT    14
#define EINVAL    22
#ifndef ENOMSG
#define ENOMSG    42
#endif
#define ENODATA   61

#define TLV_TYPE_LINK_PROTO 0x10

/*=========================================================================*/
// Struct sQMUX
//
//    Structure that defines a QMUX header
/*=========================================================================*/
typedef struct sQMUX
{
   /* T\F, always 1 */
   u8         mTF;

   /* Size of message */
   u16        mLength;

   /* Control flag */
   u8         mCtrlFlag;
   
   /* Service Type */
   u8         mQMIService;
   
   /* Client ID */
   u8         mQMIClientID;

}__attribute__((__packed__)) sQMUX;

struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS
{
   u8  TLVType;
   u16 TLVLength;
   u8  QOSSetting;
} __packed;

struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV
{
   u8  TLVType;
   u16 TLVLength;
   u32  Value;
} __packed;

struct QMIWDS_ENDPOINT_TLV
{
   u8  TLVType;
   u16 TLVLength;
   u32  ep_type;
   u32  iface_id;
} __packed;

struct QMIWDS_ADMIN_SET_DATA_FORMAT_REQ_MSG
{
    u8  CtlFlags;      // 0: single QMUX Msg; 1:
    u16 TransactionId;
    u16 Type;
    u16 Length;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV_QOS QosDataFormatTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV UnderlyingLinkLayerProtocolTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV UplinkDataAggregationProtocolTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV DownlinkDataAggregationProtocolTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV DownlinkDataAggregationMaxDatagramsTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV DownlinkDataAggregationMaxSizeTlv;
    struct QMIWDS_ENDPOINT_TLV epTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV  dl_minimum_padding;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV UplinkDataAggregationMaxDatagramsTlv;
    struct QMIWDS_ADMIN_SET_DATA_FORMAT_TLV UplinkDataAggregationMaxSizeTlv;
} __packed;


u16 QMUXHeaderSize( void );

u16 QMICTLGetClientIDReqSize( void );

u16 QMICTLReleaseClientIDReqSize( void );

u16 QMICTLReadyReqSize( void );

u16 QMIWDSSetEventReportReqSize( void );

u16 QMIWDSGetPKGSRVCStatusReqSize( void );

u16 QMIDMSGetMEIDReqSize( void );

u16 QMIWDASetDataFormatReqSize( int qmap_version );

u16  QMICTLSyncReqSize( void );

int ParseQMUX(
   u16 *    pClientID,
   void *   pBuffer,
   u16      buffSize );

int FillQMUX(
  u16      clientID,
  void *   pBuffer,
  u16      buffSize );

int GetTLV(
 void *   pQMIMessage,
 u16      messageLen,
 u8       type,
 void *   pOutDataBuf,
 u16      bufferLen );

int ValidQMIMessage(
void *   pQMIMessage,
u16      messageLen );

int GetQMIMessageID(
   void *   pQMIMessage,
   u16      messageLen );

int QMICTLGetClientIDReq(
  void *   pBuffer,
  u16      buffSize,
  u8       transactionID,
  u8       serviceType );

int QMICTLReleaseClientIDReq(
 void *   pBuffer,
 u16      buffSize,
 u8       transactionID,
 u16      clientID );

int QMICTLReadyReq(
    void *   pBuffer,
    u16      buffSize,
    u8       transactionID );

int QMIWDSSetEventReportReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

int QMIWDSGetPKGSRVCStatusReq(
  void *   pBuffer,
  u16      buffSize,
  u16      transactionID );

int QMIDMSGetMEIDReq(
 void *   pBuffer,
 u16      buffSize,
 u16      transactionID );

int QMIWDASetDataFormatReq(
    void *   pBuffer,
    u16      buffSize,
    bool     bRawIPMode, int qmap_version, u32 rx_size,
    u16      transactionID );

int QMICTLSyncReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

int QMICTLGetClientIDResp(
  void * pBuffer,
  u16    buffSize,
  u16 *  pClientID );

int QMICTLReleaseClientIDResp(
 void *   pBuffer,
 u16      buffSize );

int QMIWDSEventResp(
    void *   pBuffer,
    u16      buffSize,
    u32 *    pTXOk,
    u32 *    pRXOk,
    u32 *    pTXErr,
    u32 *    pRXErr,
    u32 *    pTXOfl,
    u32 *    pRXOfl,
    u64 *    pTXBytesOk,
    u64 *    pRXBytesOk,
    bool *   pbLinkState,
    bool *   pbReconfigure );

int QMIDMSGetMEIDResp(
   void *   pBuffer,
   u16      buffSize,
   char *   pMEID,
   int      meidSize,
   char *   pESN,
   char *   pIMEI );


int QMIWDASetDataFormatResp(
  void *   pBuffer,
  u16      buffSize, bool bRawIPMode, int *qmap_version, int *rx_size, int *tx_size, QMAP_SETTING *set);

int QMICTLSyncResp(
 void *pBuffer,
 u16  buffSize );

u16 QMIWDSBindMuxDataPortReqSize( void );

int QMIWDSBindMuxDataPortReq(
   u16 platid,
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID,
   int      mux_id );

int QMIWDSBindMuxDataPortResp(
 void *   pBuffer,
 u16      buffSize );

void GetDevLinkStatus(
    sGobiUSBNet *    pDev
);

u16 QMIWDSBindMuxDataPortPreReqSize( void );

int QMIWDSBindMuxDataPortPreReq(
    void *   pBuffer,
    u16      buffSize,
    u16      transactionID,
    unsigned int index );


#if 0
/*=========================================================================*/
// Generic QMUX functions
/*=========================================================================*/

// Remove QMUX headers from a buffer
int ParseQMUX(
   u16 *    pClientID,
   void *   pBuffer,
   u16      buffSize );

// Fill buffer with QMUX headers
int FillQMUX(
   u16      clientID,
   void *   pBuffer,
   u16      buffSize );

/*=========================================================================*/
// Generic QMI functions
/*=========================================================================*/

// Get data buffer of a specified TLV from a QMI message
int GetTLV(
   void *   pQMIMessage,
   u16      messageLen,
   u8       type,
   void *   pOutDataBuf,
   u16      bufferLen );

// Check mandatory TLV in a QMI message
int ValidQMIMessage(
   void *   pQMIMessage,
   u16      messageLen );

// Get the message ID of a QMI message
int GetQMIMessageID(
   void *   pQMIMessage,
   u16      messageLen );

/*=========================================================================*/
// Get sizes of buffers needed by QMI requests
/*=========================================================================*/

// Get size of buffer needed for QMUX
u16 QMUXHeaderSize( void );

// Get size of buffer needed for QMUX + QMICTLGetClientIDReq
u16 QMICTLGetClientIDReqSize( void );

// Get size of buffer needed for QMUX + QMICTLReleaseClientIDReq
u16 QMICTLReleaseClientIDReqSize( void );

// Get size of buffer needed for QMUX + QMICTLReadyReq
u16 QMICTLReadyReqSize( void );

// Get size of buffer needed for QMUX + QMIWDSSetEventReportReq
u16 QMIWDSSetEventReportReqSize( void );

// Get size of buffer needed for QMUX + QMIWDSGetPKGSRVCStatusReq
u16 QMIWDSGetPKGSRVCStatusReqSize( void );

u16 QMIWDSSetQMUXBindMuxDataPortSize( void );

// Get size of buffer needed for QMUX + QMIDMSGetMEIDReq
u16 QMIDMSGetMEIDReqSize( void );

// Get size of buffer needed for QMUX + QMIWDASetDataFormatReq
u16 QMIWDASetDataFormatReqSize( int qmap_mode );

// Get size of buffer needed for QMUX + QMICTLSyncReq
u16 QMICTLSyncReqSize( void );

/*=========================================================================*/
// Fill Buffers with QMI requests
/*=========================================================================*/

// Fill buffer with QMI CTL Get Client ID Request
int QMICTLGetClientIDReq(
   void *   pBuffer,
   u16      buffSize,
   u8       transactionID,
   u8       serviceType );

// Fill buffer with QMI CTL Release Client ID Request
int QMICTLReleaseClientIDReq(
   void *   pBuffer,
   u16      buffSize,
   u8       transactionID,
   u16      clientID );

// Fill buffer with QMI CTL Get Version Info Request
int QMICTLReadyReq(
   void *   pBuffer,
   u16      buffSize,
   u8       transactionID );

// Fill buffer with QMI WDS Set Event Report Request
int QMIWDSSetEventReportReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

// Fill buffer with QMI WDS Get PKG SRVC Status Request
int QMIWDSGetPKGSRVCStatusReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

u16 QMIWDSSetQMUXBindMuxDataPortReq(
   void *   pBuffer,
   u16      buffSize,
   u8 MuxId,
   u16      transactionID );

// Fill buffer with QMI DMS Get Serial Numbers Request
int QMIDMSGetMEIDReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

// Fill buffer with QMI WDA Set Data Format Request
int QMIWDASetDataFormatReq(
   void *   pBuffer,
   u16      buffSize,
   bool     bRawIPMode, int qmap_mode, u32 rx_size,
   u16      transactionID );

#if 0
int QMIWDASetDataQmapReq(
        void *   pBuffer,
        u16      buffSize,
        u16      transactionID );
#endif

int QMICTLSyncReq(
   void *   pBuffer,
   u16      buffSize,
   u16      transactionID );

/*=========================================================================*/
// Parse data from QMI responses
/*=========================================================================*/

// Parse the QMI CTL Get Client ID Resp
int QMICTLGetClientIDResp(
   void * pBuffer,
   u16    buffSize,
   u16 *  pClientID );

// Verify the QMI CTL Release Client ID Resp is valid
int QMICTLReleaseClientIDResp(
   void *   pBuffer,
   u16      buffSize );

// Parse the QMI WDS Set Event Report Resp/Indication or
//    QMI WDS Get PKG SRVC Status Resp/Indication
int QMIWDSEventResp(
   void *   pBuffer,
   u16      buffSize,
   u32 *    pTXOk,
   u32 *    pRXOk,
   u32 *    pTXErr,
   u32 *    pRXErr,
   u32 *    pTXOfl,
   u32 *    pRXOfl,
   u64 *    pTXBytesOk,
   u64 *    pRXBytesOk,
   bool *   pbLinkState,
   bool *   pbReconfigure );

// Parse the QMI DMS Get Serial Numbers Resp
int QMIDMSGetMEIDResp(
   void *   pBuffer,
   u16      buffSize,
   char *   pMEID,
   int      meidSize );

// Parse the QMI DMS Get Serial Numbers Resp
int QMIWDASetDataFormatResp(
   void *   pBuffer,
   u16      buffSize, bool bRawIPMode, int *qmap_enabled, int *rx_size, int *tx_size);

// Pasre the QMI CTL Sync Response
int QMICTLSyncResp(
   void *pBuffer,
   u16  buffSize );
#endif

