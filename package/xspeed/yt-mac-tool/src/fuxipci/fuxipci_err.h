/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_err.h

Abstract:

    FUXI error head file.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/
#ifndef __ERR_H__
#define __ERR_H__



typedef struct err_def_s {
    int           err_no;
    const char*   description;
} err_def_t;


#define STATUS_SUCCESSFUL               (0)
#define STATUS_FAILED                   (1)
#define ERR_USER_BREAK                  (2)
#define ERR_PHY_NONE                    (3)
#define ERR_PHY_UNKNOW                  (4)
#define ERR_MII_READ_NOMEMBASE          (5)
#define ERR_MII_READ_BUSYON             (6)
#define ERR_MII_WRITE_NOMEMBASE         (7)
#define ERR_MII_WRITE_BUSYON            (8)
#define ERR_MDIO_RW_FAILED              (9)
#define ERR_DESC_NUM_ILLEGAL            (10)
#define ERR_DESC_ACCESS_FAILED          (11)
#define ERR_REQ_DESC_BUFFER_TOO_LARGER  (12)
#define ERR_TPD_OUT_OF_RUNNING          (13)
#define ERR_WAIT_LPBACK_PACKET_TIMEOUT  (14)
#define ERR_ALWAYS_NOT_IDLE             (15)
#define ERR_ACCESS_MEM_ALIGN            (16)
#define ERR_PCI_BIOS_RET                (17)
#define ERR_PCI_BIOS_NOT_PRESENT        (18)
#define ERR_NO_MORE_ADAPTER             (19)
#define ERR_IN_SLEEP_MODE               (20)
#define ERR_PCI_CONFIG_RW               (21)
#define ERR_PCI_NO_BASE_MEM             (22)
#define ERR_MEM_MAPPED                  (23)
#define ERR_MEM_NOT_ACCESSABLE          (24)
#define ERR_NO_IRQ_LINE                 (25) 
#define ERR_MAP_PCIE_CONFIG             (26)
#define ERR_LOCK_MEM                    (27)
#define ERR_DOS_IRQ_INIT                (28)
#define ERR_INSTALL_HW_IRQ              (29)
#define ERR_INSTALL_TIMER_IRQ           (30)
#define ERR_LED                         (31)
#define ERR_REG_CHECK_HARD_RESET        (32)
#define ERR_REG_CHECK_READ_CLEAR        (33)
#define ERR_REG_CHECK_WRITE_CLEAR       (34)
#define ERR_REG_CHECK_READ_WRITE        (35)
#define ERR_REG_CHECK_SOFT_RESET        (36)
#define ERR_BIST_NOT_COMPLETE           (37)
#define ERR_BIST_FAILED                 (38)
#define ERR_SRAM_ZERO                   (39)
#define ERR_LINK_TIMEOUT                (40)
#define ERR_RX_TIMEOUT_LOOPBACK         (41)
#define ERR_RRD_FLAGS                   (42)
#define ERR_RX_PACKET_LEN_LOOPBACK      (43)
#define ERR_VLAN_LOOPBACK               (44)
#define ERR_PACKET_CONTENT_DIF_LOOPBACK (45)   
#define ERR_SEG_PACKET_LENGTH           (46)  
#define ERR_SEG_PACKET_MAC_HEADER       (47)
#define ERR_SEG_PACKET_IP_HEADER        (48)
#define ERR_SEG_PACKET_TCPIP_HEADER     (49)
#define ERR_SEG_PACKET_IPHL             (50)
#define ERR_SEG_PACKET_TCPHEADER        (51)
#define ERR_SEG_PACKET_SEQUENCE_NUM     (52)
#define ERR_SEG_PACKET_FLAG_PSH_FIN     (53)
#define ERR_SEG_PACKET_PSH_EXIST        (54)
#define ERR_SEG_PACKET_OFFSET           (55)
#define ERR_PACKET_NOT_TCPIP            (56)
#define ERR_ALL_SEG_TIMEOUT             (57)
#define ERR_SEG_TCP_PAYLOAD_DIF         (58)
#define ERR_CRT_INIT                    (59)
#define ERR_OUPUT_OPEN                  (60)
#define ERR_SCRIPT_OPEN                 (61)
#define ERR_PARAMTER_OPEN               (62)
#define ERR_RUN_ENV                     (63)
#define ERR_OPTION_UNSUPPORT            (64)
#define ERR_NO_SCRIPT_MULTITEST         (65)
#define ERR_BUS_NUMBER                  (66)
#define ERR_DEVICE_NUMBER               (67)
#define ERR_SCRIPT_TMP                  (68)
#define ERR_SPI_FLASH_NOT_EXIST         (71)
#define ERR_FLASH_UNKNOW                (72)
#define ERR_SPI_OPRDID_TIMEOUT          (73)
#define ERR_SPI_OPREAD_TIMEOUT          (74)
#define ERR_SPI_OPWREN_TIMEOUT          (75)
#define ERR_SPI_OPRDSR_TIMEOUT          (76)
#define ERR_SPI_OPWRITE_TIMEOUT         (77)
#define ERR_SPI_OPCHIPERASE_TIMEOUT     (78)
#define ERR_SPI_FLASH_TOO_BIG           (79)
#define ERR_SPI_FLASH_FILE_TOO_LARGE    (80)
#define ERR_SPI_FLASH_PROGRAM           (81)
#define ERR_SPI_FLASH_VERIFY            (82)
#define ERR_SPI_DATA_DISMATCH           (83)
#define ERR_TWSI_READ_TIMEOUT           (84)
#define ERR_TWSI_NO_DEV                 (85)
#define ERR_TWSI_ADDR_ALIGN             (86)
#define ERR_TWSI_LENGTH_ALIGN           (87)
#define ERR_TWSI_ADDR_OUT_OF_RANG       (88)
#define ERR_VPP_NO_HEADER               (89)
#define ERR_VPD_KEY_NOT_FOUND           (90)
#define ERR_VPD_KEY_OUT_OF_RANGE        (91)
#define ERR_VPD_READONLY_CHKSUM         (92)
#define ERR_VPD_KEYWORD_NO_WR           (93)
#define ERR_VPD_KEY_CUT                 (94)
#define ERR_VPD_TRANSFER_TIMEOUT        (95)
#define ERR_VPD_OVERFLOW                (96)
#define ERR_VPD_KEY_NOT_VALID           (97)
#define ERR_VPD_ENCODING                (98)
#define ERR_VPD_NO_ERROR_LOG_MSG        (99)
#define ERR_VPD_RO_LEN                  (100)
#define ERR_VPD_MULTI_RO_REGION         (101)
#define ERR_VPD_MULTI_RW_REGION         (102)
#define ERR_VPD_PN_CANNT_DEL            (103)
#define ERR_VPD_BUF_INVALID             (104)
#define ERR_MAKE_WIN                    (105)
#define ERR_FILE_WRITE                  (106)
#define ERR_FILE_OPEN                   (107)
#define ERR_VPD_RW_LEN                  (108)
#define ERR_TWSI_ADDR_OUT_OF_RANGE      (109)
#define ERR_TWSI_WRITE_TIMEOUT          (110)
#define ERR_NO_MEM                      (111)
#define ERR_SEND_PACKETS_TIMEOUT        (112)
#define ERR_LINK_RES_NOT_MATCH          (113)
#define ERR_MCP_MAC_MDIO_ADR_MDRW_LOCKED (114)
#define ERR_MCP_MAC_MINTR_MREI_PENDING  (115)
#define ERR_PHY_NUMBER                  (116)
#define ERR_MODE                        (117)
#define ERR_NOT_SUPPORT_64MEM           (118)
#define ERR_RST_MAC                     (119)
#define ERR_NO_XMS                      (120)
#define ERR_XMS_ALLOC                   (121)
#define ERR_XMS_LOCK                    (122)
#define ERR_NO_SELECTOR                 (123)
#define ERR_INIT_PHY                    (124)
#define ERR_PHY_LINK                    (125)
#define ERR_INVALID_MACADDR             (126)
#define ERR_TSO1_IPH_NOT_EQU            (127)
#define ERR_TSO1_TCPH_NOT_EQU           (128)
#define ERR_TSO1_SEQ                    (129)
#define ERR_TSO1_PSHFIN                 (130)
#define ERR_TSO1_PUSH_ALREADY_RCV       (131)
#define ERR_TSO1_DATOFFSET              (132)
#define ERR_TSO2_IPH_NOT_EQU            (133)
#define ERR_TSO2_TCPH_NOT_EQU           (134)
#define ERR_TSO2_SEQ                    (135)
#define ERR_TSO2_PSHFIN                 (136)
#define ERR_TSO2_PUSH_ALREADY_RCV       (137)
#define ERR_TSO2_DATOFFSET              (138)
#define ERR_TSO_DATA_MISMATCH           (139)
#define ERR_MAC_LOOP                    (140)
#define ERR_NO_SMBUS                    (141)
#define ERR_SMBUS_NO_BASEIO             (142)
#define ERR_SMBUS_BUSY                  (143)
#define ERR_REG_OUT_OF_RANG             (144)
#define SMB_UNKNOWN_FAILURE             (145)
#define SMB_ADDRESS_NOT_ACKNOWLEDGED    (146)
#define SMB_DEVICE_ERROR                (147)
#define SMB_COMMAND_ACCESS_DENIED       (148)
#define SMB_UNKNOWN_ERROR               (149)
#define SMB_DEVICE_ACCESS_DENIED        (150)
#define SMB_TIMEOUT                     (151)
#define SMB_UNSUPPORTED_PROTOCOL        (152)
#define SMB_BUS_BUSY                    (153)
#define SMB_DEV_BUSY                    (154)
#define SMB_READ_DIFF                   (155)   
#define ERR_PHY_ADCAP                   (156)
#define ERR_PHY_WR_CMP                  (157)     
#define ERR_FAILED_LOAD_DRIVER          (158)     
#define ERR_NO_USER_CMD                 (159)
#define ERR_INCORRECT_USER_CMD          (160)
#define ERR_FUNCTION_NUMBER             (161)
#define ERR_REG_IO_ACCESS               (162)
#define ERR_REG_CFG_ACCESS              (163)
        

extern const char* get_error_desc(int err_no);





#endif/*__ERR_H__*/
