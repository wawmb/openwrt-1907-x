#ifndef __LOCAL_DL_APP_H__
#define __LOCAL_DL_APP_H__

#include "crc.h"

#ifdef _MCU_UPDATETOOL_CPUARCH_
#include "drv_uart.h"
#endif

#define _MCU_UPDATETOOL_CPUARCH_ARM 1
#define _MCU_UPDATETOOL_CPUARCH_X86 0
#define _MCU_UPDATETOOL_CPUARCH_FREEX86 2

// digest 大小
#define MAX_DIGEST_SIZE 32
#define lOCALDL_COMPORT_NUM 1

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

#define RES_FILE_CRC_MAXSIZE 64
#define RES_FILE_CRC_MAXSIZE_HALF 32

#define trace printf
#define trace_err printf
#define trace_info printf

#define FILE_TYPE_BOOT 0x81   //  BOOT软件
#define FILE_TYPE_FONT 0xA0   //  字库文件
#define FILE_TYPE_APP 0x01    //  应用程序
#define FILE_TYPE_RESBMP 0x03 //  应用程序的数据文件
#define FILE_TYPE_LOGO 0x05   //  logo图片文件

#define DL_RETRY_SEND_NUM 3 // 配置设备重发次数 0=不重发

#define DL_LOCAL_STX (uint8_t)0xAA
#define DL_LOCAL_STXLEN (uint8_t)0x01
#define DL_LOCAL_FIXEDLEN (uint8_t)0x07 // cmdcode[1B]+return code[2B]+length[2B]+crc[2B]
#define DL_LOCAL_ETX (uint8_t)0x55
#define DL_LOCAL_ETXLEN (uint8_t)0x01

#define DL_CFG_PACKLEN (512)                       /* FLASH_PAGE_SIZE */
#define lOCALDL_BUFSIZE (DL_CFG_PACKLEN + 12 + 64) // 12=1+7+1+rfu3

#define DLCMD_GETTERMINFO 0x97 //  获取终端信息
#define DLCMD_LOADFILE 0x98    //  请求下载文件
#define DLCMD_TRANSDATA 0x99   //  传输数据
#define DLCMD_SAVEFILE 0x9A    //  保存文件
#define DLCMD_FINISH 0x9B      //  结束下载

#define DL_TIME_WAIT 60000 //

// 握手进入下载需要发送命令
#define DL_HANDSHAKE_CMD1 0xAA
#define DL_HANDSHAKE_CMD2 0xFE

// 内部读写flash识别字段
#define INFLASH_WRITE_STR "$A"
#define INFLASH_WRITE_STRLEN 2

#define INFLASH_WRITE_B_STR "$B"
#define INFLASH_WRITE_B_STRLEN 2

#define APP_CHANGEBDR_COMPLET "LCM modified Bdr."
#define APP_CHANGEBDR_COMPLET_LEN strlen(APP_CHANGEBDR_COMPLET)

// 返回值状态码定义
#define RET_SUCCESS 0
#define RET_INVALID_PARAM -1
#define RET_NOT_READY -2
#define RET_EXEC_ERROR -3
#define RET_BUF_TOO_SMALL -4
#define RET_LEN_ERROR -5
#define RET_BUSY_STATE -6
#define RET_TIMEOUT_ERR -7
#define RET_VERIFY_CRCERR -8

#define DIGEST_MODE_CRC32 0xC0 // default
#define DIGEST_MODE_SHA256 0xC1
#define DIGEST_MODE_MD5 0xC2 //

enum
{
    IS_OPRCODE_LISTINFO = 0,
    IS_OPRCODE_UPDATEAPP = 0x01,
    IS_OPRCODE_UPDATEAPP_FORCE = 0x02,
    IS_OPRCODE_UPDATEFONTLIB = 0x11,
    IS_OPRCODE_UPDATEFONTLIB_FORCE,
    IS_OPRCODE_UPDATERES = 0x21,
    IS_OPRCODE_UPDATERES_FORCE,
    IS_OPRCODE_UPDATELOGO = 0x31,
    IS_OPRCODE_UPDATELOGO_FORCE,
    IS_OPRCODE_UPDATEBOOT = 0x81,
    IS_OPRCODE_UPDATEBOOT_FORCE = 0x82,
    IS_OPRCODE_LISTRES = 0x90,
};

struct LocalDlFrame
{
    uint8_t stx;                 // 帧头
    uint8_t cmd;                 // 指令码
    uint8_t recode[2];           // return-code
    uint8_t length[2];           // bin 长度
    uint8_t crc[2];              // bin crc
    uint8_t bin[DL_CFG_PACKLEN]; // bin 内容
    uint8_t etx;                 // 帧尾
    // uint8_t rfu[3];
};

typedef union _LOCALDL_CONTRL
{
    uint8_t byte;
    struct
    {
        uint8_t chagebps : 1;     // 1-chage bps
        uint8_t finish : 1;       //
        uint8_t finish_timer : 1; //
        uint8_t ctrl_update : 1;  //
        uint8_t boot_update : 1;  //
        uint8_t rfu_1 : 1;        // reserver for future
        uint8_t rfu_2 : 1;        //
        uint8_t rfu_3 : 1;        //
    } bit;
} LOCALDL_CONTRL;

/* 应用信息结构体定义 */
// typedef struct {
//     unsigned int  MagicStart;
//     unsigned char Name[32]; // x-speed nk05 boot
//     //unsigned char Ver[16]; // V1.0.0
//     unsigned int  Ver; // ver[3]=descript,default:0x00 =debug prototype; ver[2-1-0]=ver info
//     unsigned char BuildTime[24]; // Oct 25 2021 10:53:52
//     unsigned char Descript[16]; // 设备用途信息
//     unsigned int  filesize; // 0
//     //unsigned int  filecrc; // 0 - noused
//     unsigned int  MagicEnd;
// } bin_info_t;

typedef struct
{
    unsigned char crcmode;  // 校验方式 0xc0 = crc32,0xc1 = sha256 0xc2 = md5
    unsigned char filetype; // 下载文件类型
    unsigned char rfu[2];
    unsigned int datalen; // file total size
    unsigned char appdigest[MAX_DIGEST_SIZE];
    unsigned int offset;      // already dl offset
    unsigned int framelen;    // dl current frame size
    unsigned int startaddr;   // 下载文件存放位置
    unsigned int startmaxlen; // 下载文件位置最大空间
    unsigned int destaddr;    // 目的地址
    unsigned int destmaxlen;  // 目的地址空间最大
} ST_DL_INFO;

typedef struct
{
    unsigned char crcmode;  // 校验方式 0xc0 = crc32,0xc1 = sha256 0xc2 = md5
    unsigned char filetype; // 下载文件类型
    unsigned char rfu[2];
    unsigned int datalen; // file total size
    unsigned char appdigest[MAX_DIGEST_SIZE];
    unsigned int offset;   // already dl offset
    unsigned int framelen; // dl current frame size
    unsigned int flg;
    unsigned char *pdata;
} ST_DL_INFO_HOST;

struct LocalDlOpt
{
    uint8_t *serialbuf[lOCALDL_COMPORT_NUM];
    uint32_t seriallen[lOCALDL_COMPORT_NUM];
    struct LocalDlFrame *frame;

    uint32_t filelen;

    LOCALDL_CONTRL contrl;
    // uint8_t dllevel; //��������ģʽ:C:ctrl B:boot
    uint8_t commindex;
    uint32_t commbuf[lOCALDL_COMPORT_NUM];
    uint16_t loopdltime; // 下载交互超时时间
    uint16_t dlrecode;

    uint8_t crcmode;
    uint8_t request; // 下载请求 文件类型

    uint8_t appdigest[MAX_DIGEST_SIZE];
    uint32_t offset;      // already dl offset
    uint32_t framelen;    // dl current frame size
    uint32_t startaddr;   // 下载文件存放位置
    uint32_t startmaxlen; // 下载文件位置最大空间
    uint32_t destaddr;    // 目的地址
    uint32_t destmaxlen;  // 目的地址空间最大
};

int dlcom_open(uint32_t commport, int bps, uint8_t *buf_dl, uint32_t buf_size);
int dlcom_close(uint32_t commport);
int dlcom_check_readbuf(uint32_t commport);
int dlcom_clear(uint32_t commport);
int dlcom_read(uint32_t commport, uint8_t *output, uint32_t length, int t_out_ms);
int dlcom_write(uint32_t commport, uint8_t const *input, uint32_t length);

uint8_t localdl_com_open(struct LocalDlOpt *localdl);
void localdl_com_close(struct LocalDlOpt *localdl);

void local_download_proc(void);

#endif
