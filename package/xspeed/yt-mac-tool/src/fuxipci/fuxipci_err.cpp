/*++

Copyright (c) 2022  Motorcomm Corporation

Module Name:

    fuxipci_err.cpp

Abstract:

    FUXI error function.

Environment:

    User mode.

Revision History:
    2022-03-15    xiaogang.fan@motor-comm.com    created

--*/

#include "fuxipci_comm.h"
#include "fuxipci_err.h"

   
err_def_t  gErrArray[] = 
{
    {STATUS_SUCCESSFUL                 , "successful"},
    {STATUS_FAILED                     , "general fail"},
    {ERR_USER_BREAK                    , "user break"},
    {ERR_PHY_NONE                      , "no phy found"},
    {ERR_PHY_UNKNOW                    , "detect unknow phy"},
    {ERR_MII_READ_NOMEMBASE            , "mii_read: base mem not mapped"},
    {ERR_MII_READ_BUSYON               , "mii_read: BUSY still active after waiting n ms"},
    {ERR_MII_WRITE_NOMEMBASE           , "mii_write: base mem not mapped"},
    {ERR_MII_WRITE_BUSYON              , "mii_write: BUSY still active after waiting n ms"},
    {ERR_MDIO_RW_FAILED                , "AtMdioRWTest: write/read value different"},
    {ERR_DESC_NUM_ILLEGAL              , "AtDgDescInit: description number illegal"},
    {ERR_DESC_ACCESS_FAILED            , "AtDgDescInit: description buffer access failed"},
    {ERR_REQ_DESC_BUFFER_TOO_LARGER    , "AtDgDescInit: request description buffer too large"},
    {ERR_TPD_OUT_OF_RUNNING            , "AtDgSendPackets: tpd out of running"},
    {ERR_WAIT_LPBACK_PACKET_TIMEOUT    , "wait loopback packet timeout"},
    {ERR_ALWAYS_NOT_IDLE               , "status still be busy after soft reset <wait 20ms>"},
    {ERR_ACCESS_MEM_ALIGN              , "reg/mem access not align"},
    {ERR_PCI_BIOS_RET                  , "pci-bios call failed"},
    {ERR_PCI_BIOS_NOT_PRESENT          , "pci-bios not present"},
    {ERR_NO_MORE_ADAPTER               , "can't find any more adapter"},
    {ERR_IN_SLEEP_MODE                 , "adapter cann't operational in non-D0 state"},
    {ERR_PCI_CONFIG_RW                 , "pci config register read/write not equal"},
    {ERR_PCI_NO_BASE_MEM               , "no base memory in pci configuratio space"},
    {ERR_MEM_MAPPED                    , "cann't map specific memory"},
    {ERR_MEM_NOT_ACCESSABLE            , "cann't access this memory/register"},
    {ERR_NO_IRQ_LINE                   , "IRQ Line not set or wrong"},
    {ERR_MAP_PCIE_CONFIG               , "PCI Express Config could not be mapped"},
    {ERR_LOCK_MEM                      , "can't lock memory and function for interrupt"},
    {ERR_DOS_IRQ_INIT                  , "dos_irq_init failed"},
    {ERR_INSTALL_HW_IRQ                , "cann't install hardware irq"},
    {ERR_INSTALL_TIMER_IRQ             , "cann't install timer irq"},
    {ERR_LED                           , "The user has decided that the LED test has been failed"},
    {ERR_REG_CHECK_HARD_RESET          , "hard reset value error"},
    {ERR_REG_CHECK_READ_CLEAR          , "register read clear error"},
    {ERR_REG_CHECK_WRITE_CLEAR         , "register write 1 clear error"},
    {ERR_REG_CHECK_READ_WRITE          , "register read/write error"},
    {ERR_REG_CHECK_SOFT_RESET          , "regsiter value error after soft-reset"},
    {ERR_BIST_NOT_COMPLETE             , "bist not complete"},
    {ERR_BIST_FAILED                   , "bist failed"},      
    {ERR_SRAM_ZERO                     , "sram zero-check error"},
    {ERR_LINK_TIMEOUT                  , "link timeout"},
    {ERR_RX_TIMEOUT_LOOPBACK           , "wait rx-packets timeout in loopback mode"},
    {ERR_RRD_FLAGS                     , "rrd flags error"},
    {ERR_RX_PACKET_LEN_LOOPBACK        , "rx-packet's length error in loopback mode"},
    {ERR_VLAN_LOOPBACK                 , "vlan not equal or vlan-flag different"},
    {ERR_PACKET_CONTENT_DIF_LOOPBACK   , "rx vs. tx packet content different"},
    {ERR_SEG_PACKET_LENGTH             , "rx seg-packet length < 54"},
    {ERR_SEG_PACKET_MAC_HEADER         , "rx seg-packet mac-header different with sent"},
    {ERR_SEG_PACKET_IP_HEADER          , "rx seg-packet ip-header different with sent"},
    {ERR_SEG_PACKET_TCPIP_HEADER       , "rx seg-packet length  < mac+ip+tcp header"},
    {ERR_SEG_PACKET_IPHL               , "rx seg-packet length < iphl-14"},
    {ERR_SEG_PACKET_TCPHEADER          , "rx seg-packet tcp header error"},
    {ERR_SEG_PACKET_SEQUENCE_NUM       , "rx seg-packet sequence number error"},
    {ERR_SEG_PACKET_FLAG_PSH_FIN       , "last segment's PSH&FIN flag incorrect"},
    {ERR_SEG_PACKET_PSH_EXIST          , "PSH already recved"},
    {ERR_SEG_PACKET_OFFSET             , "rx seg-packet data offset > last segmetn offset"},
    {ERR_PACKET_NOT_TCPIP              , "not TCP/IP packet indicated by rrd flag"},
    {ERR_ALL_SEG_TIMEOUT               , "timeout waitting for all segment packet recv"},
    {ERR_SEG_TCP_PAYLOAD_DIF           , "tcp payload recved does not match the sent"},
    {ERR_CRT_INIT                      , "CRT terminal init failed"},
    {ERR_OUPUT_OPEN                    , "output log file open failed"},
    {ERR_SCRIPT_OPEN                   , "script file open failed"},
    {ERR_PARAMTER_OPEN                 , "parameter file open failed"},
    {ERR_RUN_ENV                       , "program cann't run under this OS"},
    {ERR_OPTION_UNSUPPORT              , "option not supported"},
    {ERR_NO_SCRIPT_MULTITEST           , "muti-device test without script file"},
    {ERR_BUS_NUMBER                    , "illegal bus number"},
    {ERR_DEVICE_NUMBER                 , "illegal device number"},
    {ERR_SCRIPT_TMP                    , "cann't create&write tmp.txt file"},
    {ERR_SPI_FLASH_NOT_EXIST           , "spi flash not exist"},
    {ERR_FLASH_UNKNOW                  , "spi flash type unknow"},
    {ERR_SPI_OPRDID_TIMEOUT            , "spi flash op-RDID command timeout"},
    {ERR_SPI_OPREAD_TIMEOUT            , "spi flash op-READ command timeout"},
    {ERR_SPI_OPWREN_TIMEOUT            , "spi flash op-WREN command timeout"},
    {ERR_SPI_OPRDSR_TIMEOUT            , "spi flash op-RDSR command timeout"},
    {ERR_SPI_OPWRITE_TIMEOUT           , "spi flash op-WRITE command timeout"},
    {ERR_SPI_OPCHIPERASE_TIMEOUT       , "spi flash op-CHIP ERASE command timeout"},
    {ERR_SPI_FLASH_TOO_BIG             , "spi flash too big to hold in FlashBuffer"},
    {ERR_SPI_FLASH_FILE_TOO_LARGE      , "flash file too large"},
    {ERR_SPI_FLASH_PROGRAM             , "flash program error"},
    {ERR_SPI_FLASH_VERIFY              , "flash verify error"},
    {ERR_SPI_DATA_DISMATCH             , "flash data dismatch with written"},
    {ERR_TWSI_READ_TIMEOUT             , "twsi read data timeout"},
    {ERR_TWSI_WRITE_TIMEOUT            , "twsi write data timeout"},
    {ERR_TWSI_NO_DEV                   , "vpd no device exist"},
    {ERR_TWSI_ADDR_ALIGN               , "twsi rw address not align with 4"},
    {ERR_TWSI_LENGTH_ALIGN             , "twsi rw length not be 4 times"},
    {ERR_TWSI_ADDR_OUT_OF_RANGE        , "twsi rw to address - out of range"},
    {ERR_VPP_NO_HEADER                 , "vpd header (0x82) not found"},
    {ERR_VPD_KEY_NOT_FOUND             , "< Keyword not present >"},                         // 1
    {ERR_VPD_KEY_OUT_OF_RANGE          , "vpd keyword out of TWSI capacity"},
    {ERR_VPD_READONLY_CHKSUM           , "vpd read only field checksum error"},
    {ERR_VPD_KEYWORD_NO_WR             , "vpd RW keyword not exist"},
    {ERR_VPD_KEY_CUT                   , "Warning: VPD Full! Keyword value was cut"},        // 2
    {ERR_VPD_TRANSFER_TIMEOUT          , "Error: VPD transfer timed out"},                   // 3
    {ERR_VPD_OVERFLOW                  , "Error: VPD overflow! Keyword was not written"},    // 4
    {ERR_VPD_KEY_NOT_VALID             , "Error: VPD keyword ID not valid"},                 // 5
    {ERR_VPD_ENCODING                  , "Error: Fatal VPD Encoding error"},                 // 6
    {ERR_VPD_NO_ERROR_LOG_MSG          , "No VPD Error Log Messages found"},
    {ERR_VPD_RO_LEN                    , "VPD Read Only Length error"},
    {ERR_VPD_RW_LEN                    , "VPD Read Write Length error"},
    {ERR_VPD_MULTI_RO_REGION           , "VPD multi-Read-Only region"},
    {ERR_VPD_MULTI_RW_REGION           , "VPD multi-Read-Write region"},
    {ERR_VPD_PN_CANNT_DEL              , "VPD Product Name cann't be delete"},
    {ERR_VPD_BUF_INVALID               , "VPD host buffer invalid"},
    {ERR_MAKE_WIN                      , "make window failed"},
    {ERR_FILE_WRITE                    , "file write error"},
    {ERR_FILE_OPEN                     , "cann't open file"},
    {ERR_NO_MEM                        , "cann't allocate memory any more"},
    {ERR_SEND_PACKETS_TIMEOUT          , "send packets timeout"},
    {ERR_LINK_RES_NOT_MATCH            , "neg result not match with user setting"},
    {ERR_MCP_MAC_MDIO_ADR_MDRW_LOCKED  , "mdio op locked"},
    {ERR_MCP_MAC_MINTR_MREI_PENDING    , "mdio read error interrupt occur"},
    {ERR_PHY_NUMBER                    , "illegal phy number"},
    {ERR_MODE                          , "illegal mode"},
    {ERR_NOT_SUPPORT_64MEM             , "can't support 64bit Device Memory for DOS"},
    {ERR_RST_MAC                       , "reset mac failed"},
    {ERR_NO_XMS                        , "no XMS available"},
    {ERR_XMS_ALLOC                     , "allocate memory via XMS fialed"},
    {ERR_XMS_LOCK                      , "lock xms memory faild"},
    {ERR_NO_SELECTOR                   , "can't allocate selector"},
    {ERR_INIT_PHY                      , "init phy failed"},
    {ERR_PHY_LINK                      , "phy error while waiting for Link"},
    {ERR_INVALID_MACADDR               , "can't get valiad mac-address"},
    {ERR_TSO1_IPH_NOT_EQU              , "[tsov4]:ipheader(tx)!=ipheader(rx)"},
    {ERR_TSO1_TCPH_NOT_EQU             , "[tsov4]:tcpheader(tx)!=tcpheader(rx)"},
    {ERR_TSO1_SEQ                      , "[tsov4]:sequence number (tcph) error"},
    {ERR_TSO1_PSHFIN                   , "[tsov4]:PSH/FIN of last segment incorrect"},
    {ERR_TSO1_PUSH_ALREADY_RCV         , "[tsov4]:PUSH/FIN already recved"},
    {ERR_TSO1_DATOFFSET                , "[tsov4]:data offset > last_seg offset"},
    {ERR_TSO2_IPH_NOT_EQU              , "[tsov6]:ipheader(tx)!=ipheader(rx)"},
    {ERR_TSO2_TCPH_NOT_EQU             , "[tsov6]:tcpheader(tx)!=tcpheader(rx)"},
    {ERR_TSO2_SEQ                      , "[tsov6]:sequence number (tcph) error"},
    {ERR_TSO2_PSHFIN                   , "[tsov6]:PSH/FIN of last segment incorrect"},
    {ERR_TSO2_PUSH_ALREADY_RCV         , "[tsov6]:PUSH/FIN already recved"},
    {ERR_TSO2_DATOFFSET                , "[tsov6]:data offset > last_seg offset"},
    {ERR_TSO_DATA_MISMATCH             , "tcp payload received does not match the sent"},
    {ERR_MAC_LOOP                      , "mac-loopback error"},
    {ERR_NO_SMBUS                      , "No SM-BUS device found"},
    {ERR_SMBUS_NO_BASEIO               , "SMBUS no BaseIO"},
    {ERR_SMBUS_BUSY                    , "SMBus Host Controller is busy"},
    {ERR_PHY_ADCAP                     , "Phy Reg4 Capability error"},
    {ERR_PHY_WR_CMP                    , "Phy Reg Write/Read Compare error"},
    {ERR_FAILED_LOAD_DRIVER            , "Failed to load NIC driver"},
    {ERR_NO_USER_CMD                   , "No user input commands"},
    {ERR_INCORRECT_USER_CMD            , "Incorrect user commands or parameters"},
    {ERR_FUNCTION_NUMBER               , "Incorrect function number when specifying NIC"},
    {ERR_REG_IO_ACCESS                 , "Failed to r/w NIC regs using IO access"},    
    {ERR_REG_CFG_ACCESS                , "Failed to r/w NIC regs using CFG access"},    
    
};

/* get_error_desc
 * purpose:
 *      get description of specific error
 * Input:
 *      errno: error number
 * Output:
 *      errno description
 */
const char* get_error_desc(int err_no)
{
    int Index, ArrySz = sizeof(gErrArray)/sizeof(err_def_t);
    const char* ret = "unknown error!";
    
    for (Index=0; Index < ArrySz; Index++)
    {
        if (err_no == gErrArray[Index].err_no)
        {
            ret = gErrArray[Index].description;
            break;
        }
    }
    return ret;
}

