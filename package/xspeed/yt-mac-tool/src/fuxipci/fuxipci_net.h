/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_net.h

Abstract:

    FUXI net head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#ifndef __APP_NET_H__
#define __APP_NET_H__                                                          

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

typedef struct IPHeader {
    u8 iph_verlen;               // Version and length
    u8 iph_tos;                  // Type of service
    u16 iph_length;              // Total length of datagram.
    u16 iph_id;                  // Identification.
    u16 iph_offset;              // Flags and fragment offset.
    u8 iph_ttl;                  // Time ot live.
    u8 iph_protocol;             // Protocol.
    u16 iph_xsum;                // Header checksum.
    u32 iph_src;                  // Source address.
    u32 iph_dest;                 // Destination address.
} IPHeader;

typedef struct PSDHeader {
    u32 saddr;
    u32 daddr;
    char mbz;
    char ptcl;
    u16 tcpl;
} PSDHeader;
   
typedef struct TCPHeader {
    u16 tcp_src;                 // Source port.
    u16 tcp_dest;                // Destination port.
    u32 tcp_seq;                  // Sequence number.
    u32 tcp_ack;                  // ack number.
    u16 tcp_flags;               // flags and data offset.
    u16 tcp_window;              // Window offered
    u16 tcp_xsum;                // Checksum
    u16 tcp_urgent;              // Urgent pointer.
} TCPHeader;

typedef struct UDPHeader {
        u16  udp_src;
        u16  udp_dest;
        u16  length;
        u16  udp_xsum;
} UDPHeader;

        
#define PHXSUM(s, d, p, l)              \
    (u32)((u32)*(u16*)(s) + \
                    (u32)*(u16*)((char*)(s) + sizeof(u16)) + \
                    (u32)*(u16*)(d) + \
                    (u32)*(u16*)((char*)(d) + sizeof(u16)) + \
                    (u32)(u16)( (((u16)(p)&0xff)<<8) | (((u16)(p)&0xff00)>>8)) + \
                    (u32)(u16)( (((u16)(l)&0xff)<<8) | (((u16)(l)&0xff00)>>8)) )


#define TCP_CTRL_FIN    0x1
#define TCP_CTRL_SYN    0x2
#define TCP_CTRL_RST    0x4
#define TCP_CTRL_PSH    0x8
#define TCP_CTRL_ACK    0x10
#define TCP_CTRL_URG    0x20
            


int 
build_ipv4_header(
        u16 totalLen,
        u8 optLen, 
        u8 protocol,
        u8* SrcIP, 
        u8* DesIP,
        u8* buffer,
        u8 ip_fragment,
        u8 calXsum);
        
int
build_udp_header(       u16 srcPort,
                    u16 desPort,
                    u16 length,
                    u8* srcIP,
                    u8* desIP,
                    u8* buffer);

int 
build_tcp_header(   u8 tcpOptLen,
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
            u8* buffer);
            

u16 ComputeChecksum(
        u8* pData, 
        u32 length,
        u16 psdu);                                                      


int is_ip_packet(u8* packet); // 0 : no, 1 : ip
int is_tcp_packet(u8* packet);// 0 : none-tcp, 1: tcp
int is_udp_packet(u8* packet);
void init_tcp_segment(void);

int assemble_tcp_segment(
        IPHeader* pXmitIpHeader,
        TCPHeader*pXmitTcpHeader,
        IPHeader* pRcvIpHeader,
        TCPHeader*pRcvTcpHeader,
        u8* data,
        u16 data_len,
        u8* buffer); 

int verify_ip_xsum(u8* packet);
int verify_tcp_xsum(u8* ptcp, u16 length, u16 psdu);
int verify_udp_xsum(u8* pudp, u16 length, u16 psdu);
u16 ComputePsduXsum(
            u32 saddr,
            u32 daddr,
            u8  ptcl,
            u16 tcpl);
void 
get_ip_header(
      u8* packet,
      IPHeader**ppiph);
        
//////////////////////////////////////////////////////////////////////////

typedef struct _IPV6Header {
    u16 tc4               :4;
    u16 ver               :4;
    u16 fwl4              :4;
    u16 tc8               :4;
    u16 fwl16;
    u16 paylen;
    u8 next_header;
    u8 hop_limit;
    u8 saddr[16];
    u8 daddr[16]; 
} IPV6Header;

typedef struct _IPV6PSDHeader {
    u8  saddr[16];
    u8  daddr[16];
    u32 paylen;
    u8  res[3];
    u8  next_header;
} IPV6PSDHeader;

typedef struct _IPV6ExtenHeader {
    u8 next_header;
    u8 hdr_length;
    u8 options[6];
} IPV6ExtenHeader;

typedef struct _ExtFragment {
    u8 next_header;
    u8 res1;
    u8 offset1;
    u8 offset2; // msb:MF
    u8 id[4];
} ExtFragment;



int 
build_ipv6_header(
    u8 protocol,
    u8* SrcIP, 
    u8* DesIP,
    u8* buffer,
    u8 Frag,
    u16 max_ext_len)  ;

u16 ComputePsduXsum2(
        u8* saddr,
        u8* daddr,
        u8 ptcl,
        u16 tcpl);
        
int get_ipv6_info(IPV6Header* ipv6h, u16 ipTotalLen,
                    u16* pIphdrLen,
                    u16* pPayLen,
                    u8* pL4type);

u16 calcL4Psdu(u8* iph, bool isv4,
bool isUDP, u16 tL4len);
                            
                            
int assemble_tcp_segment2(
        IPV6Header* pXmitIpHeader,
        TCPHeader*pXmitTcpHeader,
        IPV6Header* pRcvIpHeader,
        TCPHeader*pRcvTcpHeader,
        u8* data,
        int data_len,
        u8* buffer);
                                    
extern u16 seg_len[4096];
extern int nSegs;
extern char FIN_PUSH;
extern u16 lastSegOffset;
extern u8 TCP_FLAGS[8]; //size must be specified here

#endif//__APP_NET_H__



