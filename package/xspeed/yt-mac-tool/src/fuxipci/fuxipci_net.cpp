/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_net.cpp

Abstract:

    FUXI net function

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <net/if.h>

#include "fuxipci_comm.h"
#include "fuxipci_net.h"
#include "fuxipci_err.h"
#include "fuxipci_lpbk.h"

u8 TCP_FLAGS[] = {
    TCP_CTRL_ACK,
    TCP_CTRL_URG,
    TCP_CTRL_PSH,
    TCP_CTRL_RST,
    TCP_CTRL_FIN,
    TCP_CTRL_SYN,
    TCP_CTRL_FIN|TCP_CTRL_RST|TCP_CTRL_PSH|TCP_CTRL_ACK|TCP_CTRL_URG,
    TCP_CTRL_FIN|TCP_CTRL_RST|TCP_CTRL_PSH|TCP_CTRL_ACK|TCP_CTRL_URG|TCP_CTRL_SYN   
};


int 
build_ipv4_header(
        u16 totalLen,
        u8 optLen, 
        u8 protocol,
        u8* SrcIP, 
        u8* DesIP,
        u8* buffer,
        u8 ip_fragment,
        u8 calXsum)
{
    int i;
    IPHeader* iph = (IPHeader*) buffer;
    u8* p;
    u16 xsum;
    u8 iphl_ver;
    
    memset(iph, 0, sizeof(IPHeader));
    iphl_ver = 5 + (optLen >> 2);
    iphl_ver |= 0x40;
    iph->iph_verlen = iphl_ver;
    iph->iph_length = htons(totalLen);
    iph->iph_id = (u16) RAND();
    iph->iph_protocol = protocol;
    iph->iph_ttl = 0xff;

    if (ip_fragment) {
        iph->iph_offset = (u16) RAND();
        if (0 == (iph->iph_offset&0xFF1F)) {
            iph->iph_offset |= 0x20; // MF flag
        } else {
            int fff = RAND() % 2;
            if (0 == fff)
                iph->iph_offset |= 0x20; // MF flag
        }
        if (0 != (iph->iph_offset & 0x40))
            iph->iph_offset &= 0xFFBF;
    } else {
        if (0 == RAND()%2)
            iph->iph_offset |= 0x40; // // DF flag
    }        

    p = (u8*)&iph->iph_src;
    for (i=0; i<4; i++)
    {
        *p = SrcIP[i];
        *(p+4) = DesIP[i];
        p++;
    }
    p = buffer + 20;
    for (i=0; i<(int)optLen; i++)
    {
        *p++ = (u8) RAND();
    }
    iph->iph_xsum = 0;

    if (calXsum)
    {
        xsum = ComputeChecksum(buffer, (iphl_ver&0xf)*4, 0);
        iph->iph_xsum = xsum;
        //printf("ipheader xsum should be 0x%x\n", xsum);
    }

    return (int)((iphl_ver&0xf)*4);
}


u8 ext_header[]= 
{
    EXT_ROUTING     ,       
    EXT_FRAGMENT    ,
    EXT_AUTH        ,
    EXT_DESTINE                     
};  

    
#define MAX_EXT_H  (sizeof(ext_header)/sizeof(ext_header[0]))

u8 upper_header[] = 
{
    EXT_TCP       ,       
    EXT_UDP       ,
    EXT_ICMPV6              
}; 

int 
build_ipv6_header(
    u8 protocol,
    u8* SrcIP, 
    u8* DesIP,
    u8* buffer,
    u8 Frag,
    u16 max_ext_len)  
{
    int i, j;
    int ext_start, num_ext_headers;
    int tmp;
    u16 iphdrlen;
    u16 next_header;
    u16 ext_len = 0;
    
    IPV6Header* ipv6h = (IPV6Header*) buffer;
    ipv6h->ver = 6;
    ipv6h->tc4 = RAND();
    ipv6h->tc8 = RAND();
    ipv6h->fwl16 = RAND();
    ipv6h->hop_limit = RAND();

    for (i=0; i<16; i++)
    {
        ipv6h->saddr[i] = SrcIP[i];
        ipv6h->daddr[i] = DesIP[i];
    }

    tmp = RAND();
    if (max_ext_len != 0 && (0 != (tmp % 2) || Frag)) // with extension header
    {
        while (1) {
            if (Frag)   ext_start = RAND() % 2;
            else        ext_start = RAND() % MAX_EXT_H;

            num_ext_headers = RAND() % (MAX_EXT_H-ext_start);
            if (0 == num_ext_headers) {
                num_ext_headers = 1;
            }

            ext_len = max_ext_len / num_ext_headers;
            ext_len = (ext_len + 7) & ~7;
            if (ext_len < 8)
                ext_len = 8;
            while (1) {
                if ((ext_len * num_ext_headers) > max_ext_len) {
                    ext_len -= 8;
                    if (0 == ext_len) {
                        num_ext_headers--;
                        if (0 == num_ext_headers) {
                            num_ext_headers = 1;
                            ext_len = 8;
                            break;
                        }
                        ext_len  = 32;
                    }
                }
                else break;
            }

            if (0 != Frag) {
                int i, hav_frag = 0;
                for (i=0; i < num_ext_headers; i++) {
                    if (ext_header[ext_start+i] == EXT_FRAGMENT) {
                        hav_frag = 1;
                        break;
                    }
                }
                if (hav_frag)
                    break;
            } else {
                break;
           }
        } // while (1)

        max_ext_len = ext_len * num_ext_headers;
        for (i=0, j=0x55; i<max_ext_len; i++, j++) {
            ((u8*)(ipv6h+1))[i] = (u8)j;
        }
        next_header = ipv6h->next_header = ext_header[ext_start];
        iphdrlen = sizeof(IPV6Header);
        //printf("exth=%x ", ipv6h->next_header);

        for (j=0; j < num_ext_headers; j++, ext_start++)
        {
            IPV6ExtenHeader* exth = (IPV6ExtenHeader*) 
                     ((u8*)ipv6h + iphdrlen);
            if (j == num_ext_headers-1){
                exth->next_header = protocol;
            } else {
                exth->next_header = ext_header[ext_start+1];
            }
            if (next_header == EXT_AUTH) {
                exth->hdr_length = (ext_len - 8) / 4;
                iphdrlen += ext_len;
            }
            else if (next_header != EXT_FRAGMENT) {
                exth->hdr_length = (ext_len - 8) / 8;
                iphdrlen += ext_len;
                //printf("ext_len=%d ", ext_len);
            } else { // Fragment ExtHeader size is fixed 8bytes
                ExtFragment* ph = (ExtFragment*)exth;
                int fff = RAND() % 2;
                ph->res1 = 0;
                ph->offset2 = ph->offset1 = 0;
                if (Frag) {
                    ph->offset1 = (u8) RAND();
                    ph->offset2 = 0x1;
                    if (0 == fff)
                        ph->offset2 |= 0x80; // MF flag
                }
                ph->id[0] = (u8) RAND();
                ph->id[1] = ph->id[2] = ph->id[3] = 0;
                iphdrlen += 8;
            }
            next_header = exth->next_header;
        }
    }
    else
    { // no extension header except TCP/UDP
        ipv6h->next_header = protocol;
        iphdrlen = sizeof(IPV6Header);
    }
    return (int)(iphdrlen - sizeof(IPV6Header));   
}
    
int 
build_tcp_header(   
        u8 tcpOptLen,
        u16 srcPort, 
        u16 desPort,
        u32 seqNum,
        u32 ackNum,
        u8 tcpFlags,
        u16 WindowSize,
        u16 UrgentPoint,
        u8* srcIP,
        u8* desIP,
        u16 ippaylen,
        u16 mss,
        u8 tso,
        u8* buffer)
{
    u16 xsum;
    TCPHeader* tcph = (TCPHeader*) buffer;
    u8 tcpHeaderLength;
    u16 tcpDataLen;
    u16 tcpl;

    tcpHeaderLength = (tcpOptLen>>2) + 5;

    memset(buffer, 0, tcpHeaderLength*4);
    tcpDataLen = ippaylen - tcpHeaderLength*4;

    tcph->tcp_src = htons(srcPort);
    tcph->tcp_dest = htons(desPort);
    tcph->tcp_seq = ((seqNum>>20)&0xff) |
        (((seqNum>>12)&0xff) << 8) | 
        (((seqNum&0xfff)>>8) << 16) |
        ((seqNum&0xff) << 24);
    tcph->tcp_ack = ((ackNum>>20)&0xff) |
        (((ackNum>>12)&0xff) << 8) | 
        (((ackNum&0xfff)>>8) << 16) |
        ((ackNum&0xff) << 24);
    
    tcph->tcp_flags = 
    (((u16)tcpFlags) << 8) | 
    (((u16)tcpHeaderLength) << 4);
    tcph->tcp_window = htons(WindowSize);
    tcph->tcp_urgent = htons(UrgentPoint);
    
    if (tso)
    {
        //printf("TSO !");
        if (tcpDataLen != 0)
            tcpl = 0;
        else
            tcpl = htons(((u16)(tcpHeaderLength*4)));
    }
    else
        tcpl = htons(ippaylen);
    
    xsum = ComputePsduXsum(                    
               *(u32*)srcIP,
               *(u32*)desIP,
               6, //TCP protocol
               tcpl);
    
    tcph->tcp_xsum = ~xsum; //pesudo checksum
    
    // set MSS
    if (tcpFlags&TCP_CTRL_SYN)
    {
        buffer[20] = 2;
        buffer[21] = 4;
        *(u16*)(&buffer[22]) = htons(mss);
    }
    
    return (int)(tcpHeaderLength*4);
}



/*
 * ComputeChecksum
 * Purpos:
 *  compute 16-bit (add) checksum
 * Param:
 *  pesudo: pre-checksum in Host Order
 * Return:
 *  checksum in Host order
 */

u16 ComputeChecksum(u8* pData, u32 length, u16 psdu)
{
    u32 len = length & ~1;
    u32 i;
    u32 sum ;
    u16* p = (u16*) pData;
    
    sum = psdu;
    len >>= 1;
    
    for (i=0; i < len; i++)
    {
        sum += *p++;
        if (sum > 0xffff)
        {
            sum = (sum&0xffff) + (sum>16);
        }
    }

    if (length & 1)
    {
        sum += *(u8*)p;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (u16) (~sum);
}



u16 ComputePsduXsum2(
        u8* saddr,
        u8* daddr,
        u8 ptcl,
        u16 tcpl)
{    
    IPV6PSDHeader psdhr;

    memcpy(psdhr.saddr, saddr, 16);
    memcpy(psdhr.daddr, daddr, 16);
    psdhr.paylen = htons(tcpl);
    psdhr.next_header = ptcl;
    psdhr.res[0] = psdhr.res[1] = psdhr.res[2] = 0;

//    printf("next_header=%x, paylen=%x\n", ptcl, tcpl);

    return ComputeChecksum((u8*)&psdhr, sizeof(psdhr), 0);
}


u16 ComputePsduXsum(
        u32 saddr,
        u32 daddr,
        u8 ptcl,
        u16 tcpl)
{                      
    PSDHeader   psdhdr;
    psdhdr.saddr = saddr;
    psdhdr.daddr = daddr;
    psdhdr.mbz = 0;
    psdhdr.ptcl = ptcl;
    psdhdr.tcpl = tcpl;

    return ComputeChecksum((u8*)&psdhdr, sizeof(psdhdr), 0);
}

int
build_udp_header(       
            u16 srcPort,
            u16 desPort,
            u16 length,
            u8* srcIP,
            u8* desIP,
            u8* buffer)
{
    u16 xsum;
    UDPHeader* udph = (UDPHeader*) buffer;
    memset(buffer, 0, sizeof(UDPHeader));
    
    udph->udp_src = htons(srcPort);
    udph->udp_dest = htons(desPort);
    udph->length = htons(length);
    
    xsum = ComputePsduXsum(
         *(u32*)srcIP,
         *(u32*)desIP,
         17, // UDP protocol
         htons(length));

    udph->udp_xsum = ~xsum; //pesudo checksum
/*
    printf("udp-srcip=%lx,desIP=%lx,pro=%d, h-len=%x, xsum=%x\n",
       *(u32*)srcIP,
       *(u32*)desIP,
       17,
       htons(length), 
       xsum);
*/
    return sizeof(UDPHeader);
}

/* get_ipv6_info
        return 
            0: ok
            1: failed
 */
int get_ipv6_info(IPV6Header* ipv6h, u16 ipTotalLen,
                    u16* pIphdrLen,
                    u16* pPayLen,
                    u8* pL4type)
{
     IPV6ExtenHeader* extp;
     u16 offset;
     u8 next_header;
    
    *pPayLen = htons(ipv6h->paylen);

    if ((*pPayLen + sizeof(IPV6Header)) > ipTotalLen)
    {
        printf("paylen=%x, ipTotalLen=%x\n", *pPayLen, ipTotalLen);
        return 1;
    }

    if (ipv6h->next_header == EXT_ICMPV6 ||
        ipv6h->next_header == EXT_UDP ||
        ipv6h->next_header == EXT_TCP ||
        ipv6h->next_header == EXT_NONE)
    {
        next_header = ipv6h->next_header;
        offset = sizeof (IPV6Header);
    }
    else
    {
        next_header = ipv6h->next_header;
        offset = sizeof(IPV6Header);
        extp = (IPV6ExtenHeader*)(ipv6h + 1);
        while (next_header != EXT_NONE && (offset+8) <= ipTotalLen)
        {
            //printf("nexth=%d, offset=%d ", next_header, offset);
            if (next_header == EXT_FRAGMENT)
            {
                ExtFragment* ph = (ExtFragment*)extp;
                if (0 == (ph->offset2&0x80) && 
                    0 == ph->offset1 && 
                    0 == (ph->offset2&0x1F))
                { // non fragment
                }
                else 
                { // fragment
                    next_header = EXT_NONE;
                    offset += (extp->hdr_length+1)*8;
                    break;
                }
            }

            if (next_header == EXT_AUTH)
            {
                offset += (8 + extp->hdr_length*4);
                next_header = extp->next_header;
                extp = (IPV6ExtenHeader*)((u8*)ipv6h + offset);
                continue;
            }
            
            if (next_header != EXT_ICMPV6 &&
                next_header != EXT_UDP &&
                next_header != EXT_TCP &&
                next_header != EXT_NONE)
            {
                //printf("hdrlen=%d ", extp->hdr_length);
                offset += (extp->hdr_length+1)*8;
                next_header = extp->next_header;
                extp = (IPV6ExtenHeader*)((u8*)ipv6h + offset);
                continue;
            }
            else
            {
                break;
            }
        }
        if (next_header != EXT_ICMPV6 &&
            next_header != EXT_UDP &&
            next_header != EXT_TCP &&
            next_header != EXT_NONE)
        {
            printf("bb\n");
            return 1;
        }
    }  
    
    *pIphdrLen = offset;
    *pL4type = next_header;
    return 0;
      
}

                
u16 calcL4Psdu(u8* iph, bool isv4,
          bool isUDP, u16 tL4len)
{
    if (isv4) {
        IPHeader* v4 = (IPHeader*) iph;
        
        return ComputePsduXsum(
                   v4->iph_src,
                   v4->iph_dest,
                   v4->iph_protocol,
                   htons(htons(v4->iph_length) - (v4->iph_verlen&0xf)*4));
    } else {
        IPV6Header* ipv6h = (IPV6Header*) iph;
        return ComputePsduXsum2(
                  ipv6h->saddr,
                  ipv6h->daddr,
                  isUDP ? 17:6,
                  tL4len);
    }   
}




u16 seg_len[4096];
int nSegs;
char FIN_PUSH;
u16 lastSegOffset;

void init_tcp_segment(void)
{
    nSegs = 0;
    FIN_PUSH = 0;
    lastSegOffset = 0;  
}


int assemble_tcp_segment(
        IPHeader* pXmitIpHeader,
        TCPHeader*pXmitTcpHeader,
        IPHeader* pRcvIpHeader,
        TCPHeader*pRcvTcpHeader,
        u8* data,
        u16 data_len,
        u8* buffer)           
{
    u32 offset, newSeq, oldSeq;

    //seg_len[nSegs++] = seg_length;

    if (pXmitIpHeader->iph_verlen != pRcvIpHeader->iph_verlen ||
   /*     pXmitIpHeader->iph_id != pRcvIpHeader->iph_id || */
        pXmitIpHeader->iph_protocol != pRcvIpHeader->iph_protocol ||
        pXmitIpHeader->iph_src != pRcvIpHeader->iph_src ||
        pXmitIpHeader->iph_dest != pRcvIpHeader->iph_dest )
    {
        return ERR_TSO1_IPH_NOT_EQU;
    }

    if (pXmitTcpHeader->tcp_src != pRcvTcpHeader->tcp_src ||
        pXmitTcpHeader->tcp_dest != pRcvTcpHeader->tcp_dest ||
        pXmitTcpHeader->tcp_ack != pRcvTcpHeader->tcp_ack ||
        pXmitTcpHeader->tcp_urgent != pRcvTcpHeader->tcp_urgent)
    {
        return ERR_TSO1_TCPH_NOT_EQU;
    }        

    newSeq = ntohl(pRcvTcpHeader->tcp_seq);
    oldSeq = ntohl(pXmitTcpHeader->tcp_seq);
    if (newSeq < oldSeq)
    {
        offset = 0xffffffff - (oldSeq - newSeq);
    }
    else
        offset = newSeq - oldSeq;
    
    if (offset > 0xffff)
    {
        return ERR_TSO1_SEQ;
    }
    if (((pRcvTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN))
    {
        if (   (((pXmitTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN))
            != (((pRcvTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN)) )
        {
            return ERR_TSO1_PSHFIN;
        }
        if (FIN_PUSH)
        {
            return ERR_TSO1_PUSH_ALREADY_RCV;
        }
        FIN_PUSH = 1;
        lastSegOffset = (u16)offset;           
    }
    else if (FIN_PUSH)
    {
        if ((u16)offset >= lastSegOffset)
        {
            //printf("ERROR: data offset 0x%lx > last segmetn offset 0x%x ",
            //  offset, lastSegOffset);
            return ERR_TSO1_DATOFFSET;
        }
    }
    
    memcpy(buffer+offset, data, data_len);
    
    return 0;
}


// IPV6
int assemble_tcp_segment2(
        IPV6Header* pXmitIpHeader,
        TCPHeader*pXmitTcpHeader,
        IPV6Header* pRcvIpHeader,
        TCPHeader*pRcvTcpHeader,
        u8* data,
        int data_len,
        u8* buffer)          
{
    u32 offset, newSeq, oldSeq;

//    seg_len[nSegs++] = seg_length;

    
    if (pXmitIpHeader->tc4 != pRcvIpHeader->tc4 ||
        pXmitIpHeader->ver != pRcvIpHeader->ver ||
        pXmitIpHeader->tc8 != pRcvIpHeader->tc8 ||
        pXmitIpHeader->fwl4 != pRcvIpHeader->fwl4 ||
                pXmitIpHeader->fwl16 != pRcvIpHeader->fwl16 ||
        pXmitIpHeader->next_header != pRcvIpHeader->next_header ||
        pXmitIpHeader->hop_limit != pRcvIpHeader->hop_limit )
    {
        return ERR_TSO2_IPH_NOT_EQU;
    }
    
    if (memcmp(pXmitIpHeader->saddr, pRcvIpHeader->saddr, 16) ||
        memcmp(pXmitIpHeader->daddr, pRcvIpHeader->daddr, 16) )
    {
        return ERR_TSO2_IPH_NOT_EQU;
    }
    

    if (pXmitTcpHeader->tcp_src != pRcvTcpHeader->tcp_src ||
        pXmitTcpHeader->tcp_dest != pRcvTcpHeader->tcp_dest ||
        pXmitTcpHeader->tcp_ack != pRcvTcpHeader->tcp_ack ||
        pXmitTcpHeader->tcp_urgent != pRcvTcpHeader->tcp_urgent)
    {
//        printf("ERROR: tcp header not equal!\n");
        return ERR_TSO2_TCPH_NOT_EQU;
    }        

    newSeq = ntohl(pRcvTcpHeader->tcp_seq);
    oldSeq = ntohl(pXmitTcpHeader->tcp_seq);
    if (newSeq < oldSeq)
    {
        offset = 0xffffffff - (oldSeq - newSeq);
    }
    else
        offset = newSeq - oldSeq;
    
    if (offset > 0xffff)
    {
//      printf("ERROR: sequence number (0x%lx:0x%lx) incorrect!\n",
//      pXmitTcpHeader->tcp_seq,
//      pRcvTcpHeader->tcp_seq);
        return ERR_TSO2_SEQ;
    }
    if (((pRcvTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN))
    {
        if (   (((pXmitTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN))
            != (((pRcvTcpHeader->tcp_flags>>8)&0xFF)&(TCP_CTRL_PSH|TCP_CTRL_FIN)) )
        {
            return ERR_TSO2_PSHFIN;
        }
        if (FIN_PUSH)
        {
            return ERR_TSO2_PUSH_ALREADY_RCV;
        }
        FIN_PUSH = 1;
        lastSegOffset = (u16)offset;           
    }
    else if (FIN_PUSH)
    {
        if ((u16)offset >= lastSegOffset)
        {
//          printf("Seg=%d", nSegs);
//          printf("ERROR: data offset 0x%lx > last segmetn offset 0x%x ",
//              offset, lastSegOffset);
            return ERR_TSO2_DATOFFSET;
        }
    }
 
    memcpy(buffer+offset, data, data_len);
    
    return 0;
}
        
        
