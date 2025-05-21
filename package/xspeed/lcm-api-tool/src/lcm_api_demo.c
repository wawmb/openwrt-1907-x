#include <stdio.h>
#ifndef _MCU_UPDATETOOL_CPUARCH_
#include <windows.h>
#endif /* #ifndef _MCU_UPDATETOOL_CPUARCH_ */
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "lcm_api_demo.h"

#ifdef _MCU_UPDATETOOL_CPUARCH_ /* linux 下需要兼容的 windows 命令和定义 */
typedef int HANDLE;
typedef unsigned char BOOL;
#define INVALID_HANDLE_VALUE -1
typedef unsigned long DWORD;

#define NOPARITY 0
#define ODDPARITY 1
#define EVENPARITY 2
#define MARKPARITY 3
#define SPACEPARITY 4

#define ONESTOPBIT 1
#define ONE5STOPBITS 1
#define TWOSTOPBITS 2

#define IGNORE 0
#define INFINITE 0xffffffff

#define CBR_110 110
#define CBR_300 300
#define CBR_600 600
#define CBR_1200 1200
#define CBR_2400 2400
#define CBR_4800 4800
#define CBR_9600 9600
#define CBR_14400 14400
#define CBR_19200 19200
#define CBR_38400 38400
#define CBR_56000 56000
#define CBR_57600 57600
#define CBR_115200 115200
#define CBR_128000 128000
#define CBR_256000 256000

#endif /* #ifndef _MCU_UPDATETOOL_CPUARCH_ */

#define CFG_DEBUG_COMID "COM3"

uint8_t busid;

#define trace_line() /* printf("\t%s : %d\n", __func__, __LINE__) */

uint8_t *gAppToolName = NULL;

#define my_free(p) \
    do             \
    {              \
        free(p);   \
        p = NULL;  \
    } while (0)

/* 缓冲区大小 */
#define BUF_SIZE 2048

#if 1 /* time API */
#include <time.h>

#ifdef _MCU_UPDATETOOL_CPUARCH_ /* linux 下需要兼容的 windows 命令和定义 */

static unsigned long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#define sys_gettickcounter GetTickCount
#define sys_getcounter GetTickCount
#define sys_delay_ms(n) usleep(1000 * (n))
#else /* windows */
#define sys_gettickcounter GetTickCount
#define sys_getcounter clock
#define sys_delay_ms Sleep
#endif

uint32_t time_need_delay(uint32_t msec)
{
    time_t start, end;
    double cost;
    time(&start);

    sys_delay_ms(msec);
    time(&end);
    cost = difftime(end, start);
    trace("%f\n", cost);
    return RET_SUCCESS;
}

#endif

void trace_buf(uint8_t *tt, uint8_t *p, uint32_t len)
{
    int i = 0;
    trace("[%ld]%s:%d\n", sys_getcounter(), tt, len);
    for (i = 0; i < len; i++)
    {
        trace("%02x ", p[i]);
    }
    trace("\n");
}

void trace_err_buf(uint8_t *tt, uint8_t *p, uint32_t len)
{
    int i = 0;
    trace_err("[%ld]%s:%d\n", sys_getcounter(), tt, len);
    for (i = 0; i < len; i++)
    {
        trace_err("%02x ", p[i]);
    }
    trace_err("\n");
}

/****************************************************************************************
**********************************  串口 UART API   **************************************
****************************************************************************************/

#ifndef _MCU_UPDATETOOL_CPUARCH_ /* windows  */
/*  打开串口 */
HANDLE OpenSerial(const char *com, /* 串口名称，如COM1，COM2*/
                  int baud,        /*波特率：常用取值：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800*/
                  int byteSize,    /*数位大小：可取值7、8；*/
                  int parity,      /*校验方式：可取值NOPARITY、ODDPARITY、EVENPARITY、MARKPARITY、SPACEPARITY*/
                  int stopBits)    /*停止位：ONESTOPBIT、ONE5STOPBITS、TWOSTOPBITS；*/
{
    DCB dcb;
    BOOL b = FALSE;
    COMMTIMEOUTS CommTimeouts;
    HANDLE comHandle = INVALID_HANDLE_VALUE;

    /*打开串口*/
    comHandle = CreateFile(com,                          /*串口名称*/
                           GENERIC_READ | GENERIC_WRITE, /*可读、可写   				 */
                           0,                            /* No Sharing                               */
                           NULL,                         /* No Security                              */
                           OPEN_EXISTING,                /* Open existing port only                     */
                           FILE_ATTRIBUTE_NORMAL,        /* Non Overlapped I/O                           */
                           NULL);                        /* Null for Comm Devices*/

    if (INVALID_HANDLE_VALUE == comHandle)
    {
        trace_err("CreateFile fail\r\n");
        return comHandle;
    }

    /* 设置读写缓存大小*/
    b = SetupComm(comHandle, BUF_SIZE, BUF_SIZE);
    if (!b)
    {
        trace_err("SetupComm fail\r\n");
    }

    /*设定读写超时*/
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;   /*读间隔超时*/
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;   /*读时间系数*/
    CommTimeouts.ReadTotalTimeoutConstant = 0;     /*读时间常量*/
    CommTimeouts.WriteTotalTimeoutMultiplier = 1;  /*写时间系数*/
    CommTimeouts.WriteTotalTimeoutConstant = 1;    /*写时间常量*/
    b = SetCommTimeouts(comHandle, &CommTimeouts); /*设置超时*/
    if (!b)
    {
        trace_err("SetCommTimeouts fail\r\n");
    }

    /*设置串口状态属性*/
    GetCommState(comHandle, &dcb);     /*获取当前*/
    dcb.BaudRate = baud;               /*波特率*/
    dcb.ByteSize = byteSize;           /*每个字节有位数*/
    dcb.Parity = parity;               /*无奇偶校验位*/
    dcb.StopBits = stopBits;           /*一个停止位*/
    b = SetCommState(comHandle, &dcb); /*设置*/
    if (!b)
    {
        trace_err("SetCommState fail\r\n");
    }

    return comHandle;
}
#endif /* #ifndef _MCU_UPDATETOOL_CPUARCH_ // windows */

#define UART_PORT_MAX 30
static const char *uart_port_strlist[UART_PORT_MAX] = {
    "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3", "/dev/ttyUSB4", "/dev/ttyUSB5", "/dev/ttyUSB6", "/dev/ttyUSB7", "/dev/ttyUSB8", "/dev/ttyUSB9",
    "/dev/ttyAMA0", "/dev/ttyAMA1", "/dev/ttyAMA2", "/dev/ttyAMA3", "/dev/ttyAMA4", "/dev/ttyAMA5", "/dev/ttyAMA6", "/dev/ttyAMA7", "/dev/ttyAMA8", "/dev/ttyAMA9",
    "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3", "/dev/ttyS4", "/dev/ttyS5", "/dev/ttyS6", "/dev/ttyS7", "/dev/ttyS8", "/dev/ttyS9"};
int fd_serial = -1;
HANDLE serial_open(const char *com, /*串口名称，如COM1，COM2*/
                   int baud,        /*波特率：常用取值：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800*/
                   int byteSize,    /*数位大小：可取值7、8；*/
                   int parity,      /*校验方式：可取值NOPARITY、ODDPARITY、EVENPARITY、MARKPARITY、SPACEPARITY*/
                   int stopBits)    /*停止位：ONESTOPBIT、ONE5STOPBITS、TWOSTOPBITS；*/
{
#ifndef _MCU_UPDATETOOL_CPUARCH_ /* windows */
    return OpenSerial(com, baud, byteSize, parity, stopBits);
#else /* linux*/
    int ret = -1;
    printf("com %s , baud %d\n", uart_port_strlist[busid], baud);
    ret = UART0_Open(&fd_serial, (char *)uart_port_strlist[busid]);
    if (ret == -1)
    {
        trace_err("open %s failed: 0x%x", uart_port_strlist[busid], ret);
        return ret;
    }
    ret = UART0_Init(fd_serial, baud, 0, byteSize, stopBits, 'N');
    if (ret == -1)
    {
        printf("config %s failed: baudrate:%d", uart_port_strlist[busid], baud);
        return ret;
    }
    printf("succeed connect %s %d %d %d %d\n", uart_port_strlist[busid], baud,
           byteSize, parity, stopBits);
#endif
    return fd_serial;
}

static int serial_close(HANDLE comHandle)
{
#ifndef _MCU_UPDATETOOL_CPUARCH_ /* windows */
    return CloseHandle(comHandle);
#else /* linux*/
    UART0_Close(comHandle);
    return 1;
#endif
}

static int serial_send(HANDLE comHandle, uint8_t *pbuf, uint8_t len)
{
    BOOL b = FALSE;
    int reallen = 0;
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        trace_err("serial send failed, com error!\r\n");
        return -1;
    }
    if (NULL == pbuf)
    {
        trace_err("serial send failed, buffer error!\r\n");
        return -2;
    }
    if (0 == len)
    {
        trace_err("serial send failed, len error!\r\n");
        return -3;
    }

    /*回复*/
    trace_buf("send frame", pbuf, len);
#ifdef _MCU_UPDATETOOL_CPUARCH_
    reallen = UART0_Send(comHandle, pbuf, len);
#else
    b = WriteFile(comHandle, pbuf, len, &reallen, NULL);
#endif
    if (!b)
    {
        trace_err("send fail\r\n");
    }
    return reallen;
}

typedef struct
{
    uint8_t flag; /* 0 无数据 1 有数据*/
    uint8_t rfu[3];
    uint32_t fifolen;
    uint8_t fifobuf[BUF_SIZE << 2];
} ST_SERIAL_FIFO_TEMP;
ST_SERIAL_FIFO_TEMP st_serial_fifo;

static int serial_recv(HANDLE comHandle, uint8_t *pbuf, int len)
{
    BOOL b = FALSE;
    int reallen = 0;
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        trace_err("serial recv failed, com error!\r\n");
        return -1;
    }
    if (NULL == pbuf)
    {
        trace_err("serial recv failed, buffer error!\r\n");
        return -2;
    }
    if (0 == len)
    {
        trace_err("serial recv failed, len error!\r\n");
        return -3;
    }
#ifdef _MCU_UPDATETOOL_CPUARCH_
    reallen = UART0_Recv(comHandle, pbuf, len);
#else
    b = ReadFile(comHandle, pbuf, len, &reallen, NULL);
#endif
    if (!b)
    {
        trace_err("recv fail\r\n");
    }
    return reallen;
}

static int serial_checkbuf(HANDLE comHandle, uint32_t nlen)
{
    int ret, i;
    int iret = RET_LEN_ERROR;
    ST_SERIAL_FIFO_TEMP *pfifo = &st_serial_fifo;

    ret = 0;
    if (pfifo->fifolen + nlen > (BUF_SIZE << 2))
    {
        pfifo->fifolen = 0;
        trace("fifolen out range \n");
    }
    for (i = 0; i < 10; i++)
    {
        ret = serial_recv(comHandle, pfifo->fifobuf + pfifo->fifolen, nlen);
        sys_delay_ms(10);

        trace("recv ret %d %d\n", ret, nlen);

        if (ret > 0) /* 读取数据*/
        {
            if (ret == nlen) /* 读取到等额数据*/
            {
                pfifo->fifolen += nlen;
                trace("recv fifo read data %d\n", pfifo->fifolen);
                iret = RET_SUCCESS;
                break;
            }
            /* 读取较少数据*/
            pfifo->fifolen += ret;
            nlen -= ret;
        }
    }
    return iret;
}

static int serial_fiforecv(HANDLE comHandle, uint8_t *pbuf, int len)
{
    ST_SERIAL_FIFO_TEMP *pfifo = &st_serial_fifo;
    /* if (pfifo->flag) // 有数据*/
    {
        if (pfifo->fifolen >= len)
        {
            trace("recv fifo already data %d\n", pfifo->fifolen);
            memcpy(pbuf, pfifo->fifobuf, len);
            trace_buf("recvfifo", pfifo->fifobuf, pfifo->fifolen);
            pfifo->fifolen -= len;
            return len;
        }
    }

    return RET_LEN_ERROR;
}

static int serial_fifoclear(int mode)
{
    memset((uint8_t *)&st_serial_fifo, 0x00, sizeof(st_serial_fifo));
    return RET_SUCCESS;
}
/****************************************************************************************
**********************************  串口 UART API   **************************************
****************************************************************************************/
void app_version_prt(char *argv)
{
    trace_info("%s version:%s\n", argv, "2.0.1");
    trace_info("build date:%s %s\n", __DATE__, __TIME__);
}

typedef struct
{
    uint8_t *str;
    uint8_t cmd;
} st_oprcmd_info;

const st_oprcmd_info opr_info[] = {
    {
        .str = "-productinfo",
        .cmd = E_DISP_PRODUCTINFO,
    },
    {
        .str = "-reset",
        .cmd = E_DISP_RESET,
    },
    {
        .str = "-cleanscreen",
        .cmd = E_DISP_CLS,
    },
    {
        .str = "-openlcd",
        .cmd = E_DISP_OPENLCD,
    },
    {
        .str = "-closelcd",
        .cmd = E_DISP_CLOSELCD,
    },
    {
        .str = "-light",
        .cmd = E_DISP_LIGHTNESS,
    },
    {
        .str = "-contract",
        .cmd = E_DISP_LATTICE,
    },
    {
        .str = "-setcursor",
        .cmd = E_DISP_SETCURSOR,
    },
    {
        .str = "-invert",
        .cmd = E_DISP_INVERT,
    },
    /* { .str = "-charsetspace", .cmd = E_DISP_ZFJJ, },*/
    {
        .str = "-string",
        .cmd = E_DISP_STRING,
    },
    {
        .str = "-string2",
        .cmd = E_DISP_STRING_2,
    },
    {
        .str = "-autoline",
        .cmd = E_DISP_AUTOLINE,
    },
    {
        .str = "-rolltime",
        .cmd = E_DISP_ROLLTIME,
    },
    {
        .str = "-cleantime",
        .cmd = E_DISP_CLEANTIME,
    },
    {
        .str = "-dispbmp",
        .cmd = E_DISP_BMPNUM,
    },
    {
        .str = "-dispblock",
        .cmd = E_DISP_BLOCKPIC,
    },

    /* { .str = "-exitdebug", .cmd = E_SPECFUNC_EXITTHREAD, },*/
    {
        .str = "-changeBaudRate",
        .cmd = E_SPECFUNC_CHANGEBDR,
    },
    /* { .str = "-EntryDl", .cmd = E_SPECFUNC_ENTRYDLMODE, },*/
};

void usage(const char *app)
{
    printf("\nUsage:\tttyUSB0-9\tttyAMA0-9\tttyS0-9\n");
    printf("\nUsage:\t%s -h\n", app);
    printf("\tprintf help informations\n");
    printf("\nUsage:\t%s -v\n", app);
    printf("\tprintf version informations\n");
    printf("\t baudrate only support 9600/115200 \n");
    printf("\nUsage:\t%s baudrate <busid> -productinfo\n", app);
    printf("\t lcd display prodction info.\n");
    printf("\nUsage:\t%s baudrate <busid> -reset\n", app);
    printf("\t reset lcd\n");
    printf("\nUsage:\t%s baudrate <busid> -cleanscreen\n", app);
    printf("\t clean lcd screen\n");
    printf("\nUsage:\t%s baudrate <busid> -openlcd\n", app);
    printf("\t open lcd display function.\n");
    printf("\nUsage:\t%s baudrate <busid> -closelcd\n", app);
    printf("\t close lcd display function.\n");
    printf("\nUsage:\t%s baudrate <busid> -light <num>\n", app);
    printf("\t set lcd lightness [0-255],if lcd unsuppot backlight config,it's on/off light\n");
    printf("\nUsage:\t%s baudrate <busid> -contract <num>\n", app);
    printf("\t set lcd contract [0-255]\n");
    printf("\nUsage:\t%s baudrate <busid> -setcursor x y\n", app);
    printf("\t set lcd current posion and  x[1-7] y[1-192]\n");
    printf("\nUsage:\t%s baudrate <busid> -invert <num>\n", app);
    printf("\t set lcd display if invert, 1/0=On/Off. default: OFF\n");
    printf("\nUsage:\t%s baudrate <busid> -string <string...>\n", app);
    printf("\t display string, please use with <-setcursor x y>\n");
    printf("\nUsage:\t%s baudrate <busid> -string2 x y num <string...>\n", app);
    printf("\t string: display string, x y: display posion, num: line break count\n");
    // printf("\nUsage:\t%s baudrate <busid> -autoline -string <string...>\n", app);
    // printf("\t display string, automatic line wrapping, default setcursor 1 0\n");
    // printf("\t -cleantime no default 3s set\n");
    // printf("\nUsage:\t%s baudrate <busid> -autoline -cleantime <num> -string <string...>\n", app);
    // printf("\t display string, The string on the screen will be cleared after <num> s, last reservation\n");
    printf("\nUsage:\t%s baudrate <busid> -dispbmp <num >> 8> <num>\n", app);
    printf("\t Lcd display bmp ,is according to the bmp's num.[0-19]\n");
    printf("\nUsage:\t%s baudrate <busid> -dispblock <num >> 8> <num> x y ex ey w h\n", app);
    printf("\t display bmp at the range[x ex] and [y ey], w/h is bmp real size, it is ignore.\n");
    printf("\nUsage:\t%s baudrate <busid> -changeBaudRate <BDR_NUM>\n", app);
    printf("\t switch current baudrate, BDR_NUM is 0/1=9600/115200,default:9600>\n");
}

void next_line_cmd_auto_adjust_func(uint8_t cur_line_, uint8_t *buf_p)
{
    buf_p[0] = 0x04;
    buf_p[1] = 0xAA;
    buf_p[2] = 0x20;
    buf_p[3] = cur_line_ % 8;
    buf_p[4] = 0x00;
}
void next_line_cmd_func(uint8_t cur_line_, uint8_t *buf_p)
{
    buf_p[0] = 0x04;
    buf_p[1] = 0xAA;
    buf_p[2] = 0x20;
    buf_p[3] = cur_line_;
    buf_p[4] = 0x00;
}

void first_line_cmd_func(uint8_t *buf_p, uint8_t x, uint8_t y)
{
    buf_p[0] = 0x04;
    buf_p[1] = 0xAA;
    buf_p[2] = 0x20;
    buf_p[3] = x;
    buf_p[4] = y;
}

int clear_screen(HANDLE comHandle)
{
    uint8_t buf[2] = {0xAA, 0x10};
    return serial_send(comHandle, buf, 2);
}

uint8_t cmd_buf[128][32] = {0};
uint8_t string_auto_line = 0;
uint8_t string_roll_time = 2;
uint8_t string_clean_time = 3;

/* 导入参数解析命令*/
int argv_parse_proc(HANDLE comHandle, int argc, char *argv[])
{
    int ret = RET_SUCCESS;
    uint8_t cur_buf_point = 1;

    uint8_t string_display_x = 0, string_display_y = 0, string_display_num = 0;
    uint8_t buf[BUF_SIZE] = {0};
    uint8_t *buf_p = cmd_buf[cur_buf_point++]; /*[1] 如果是txt [0]位置放坐标命令*/
    uint32_t sendlen = 0;
    uint8_t mode = 0;
    uint8_t cur_pinfo_cmd = 0;
    int i;
    st_oprcmd_info *pinfo = (st_oprcmd_info *)&opr_info;
    uint32_t nlen = 0;

    if (argc <= 0 || argv == NULL)
    {
        trace_err("\tinput invalid param argc=%d.\n", argc);
        return RET_INVALID_PARAM;
    }
    buf_p[sendlen++] = 0xAA;
    for (i = 0; i < BUFFER_DIM(opr_info); i++)
    {
        if (strcmp(pinfo[i].str, argv[0]) == 0)
        {
            buf_p[sendlen++] = pinfo[i].cmd;
            cur_pinfo_cmd = pinfo[i].cmd;
            trace("operation code:%X\n", pinfo[i].cmd);
            break;
        }
    }
    if (i >= BUFFER_DIM(opr_info))
    {
        trace_err("input invalid paramete!\n");
        return RET_NOT_FOUND;
    }

    trace("operation code:%d %s\n", argc, argv[0]);

    switch (cur_pinfo_cmd)
    {
    case E_DISP_PRODUCTINFO:
    case E_DISP_RESET:
    case E_DISP_CLS:
    case E_DISP_OPENLCD:
    case E_DISP_CLOSELCD:
    case E_SPECFUNC_ENTRYDLMODE:
    case E_SPECFUNC_EXITTHREAD:
        break;
    case E_DISP_LIGHTNESS:
    case E_DISP_LATTICE:
    case E_DISP_INVERT:
    case E_DISP_ZFJJ:
    case E_SPECFUNC_CHANGEBDR:
    { /* 1byte*/
        nlen = 1;
    }
    break;

    case E_DISP_SETCURSOR:
    case E_DISP_BMPNUM:
    { /* 2 byte*/
        nlen = 2;
    }
    break;
    case E_DISP_AUTOLINE:
        string_auto_line = 1;
        return RET_UPDATE_PARAMETER_1;
    case E_DISP_ROLLTIME:
        string_roll_time = strtoul(argv[1], 0, 0);
        return RET_UPDATE_PARAMETER_2;
    case E_DISP_CLEANTIME:
        string_clean_time = strtoul(argv[1], 0, 0);
        return RET_UPDATE_PARAMETER_2;
    case E_DISP_STRING_2:
        if (argc < 5)
        {
            return RET_INVALID_PARAM;
        }
        string_display_x = strtoul(argv[1], 0, 0);
        string_display_y = strtoul(argv[2], 0, 0);
        string_display_num = strtoul(argv[3], 0, 0);
        nlen = strlen(argv[4]);
        break;
    case E_DISP_STRING:
    { /* argut length // 字符串 以获取到 0x00为结束*/
        nlen = strlen(argv[1]);
    }
    break;
    case E_DISP_BLOCKPIC:
    { /* 8 byte*/
        nlen = 8;
    }
    break;

    default:
        app_version_prt(gAppToolName);
        usage(gAppToolName);
        return RET_INVALID_PARAM;
    }
    if (cur_pinfo_cmd == E_DISP_STRING_2)
    {
        uint8_t cur_line_ = string_display_x;
        /*调整命令 buf_p[0] 放置长度 ，不会发送*/
        first_line_cmd_func(cmd_buf[0], cur_line_, string_display_y);
        cur_line_ += 2;
        sendlen = 1;
        buf_p[sendlen++] = 0xAA;
        buf_p[sendlen++] = E_DISP_STRING;
        nlen = (nlen > (256 - 3)) ? (256 - 3) : nlen;

        for (i = 0; i < nlen; i++)
        {
            if (sendlen == (string_display_num + 3))
            {
                /*长度达到标准 +3 是前缀三个字符*/
                buf_p[sendlen++] = 0x00;
                buf_p[0] = sendlen;
                buf_p = cmd_buf[cur_buf_point++];
                /*填充下一行命令*/
                next_line_cmd_func(cur_line_, buf_p);
                cur_line_ += 2;
                buf_p = cmd_buf[cur_buf_point++];
                sendlen = 1;
                buf_p[sendlen++] = 0xAA;
                buf_p[sendlen++] = E_DISP_STRING;
            }
            buf_p[sendlen++] = argv[4][i];
        }
        buf_p[sendlen++] = 0x00;
        buf_p[0] = sendlen;
    }
    else if (string_auto_line == 1 && cur_pinfo_cmd == E_DISP_STRING) /* 变长命令*/
    {
        uint8_t cur_line_ = 1;
        /*调整命令 buf_p[0] 放置长度 ，不会发送*/
        next_line_cmd_auto_adjust_func(cur_line_, cmd_buf[0]);
        cur_line_ += 2;
        sendlen = 1;
        buf_p[sendlen++] = 0xAA;
        buf_p[sendlen++] = E_DISP_STRING;
        nlen = (nlen > (256 - 3)) ? (256 - 3) : nlen;

        for (i = 0; i < nlen; i++)
        {
            if (argv[1][i] == 0x0a || sendlen == 27)
            {
                /*回车符 或 长度达到标准*/
                buf_p[sendlen++] = 0x00;
                buf_p[0] = sendlen;
                buf_p = cmd_buf[cur_buf_point++];
                /*填充下一行命令*/
                next_line_cmd_func(cur_line_, buf_p);
                cur_line_ += 2;
                buf_p = cmd_buf[cur_buf_point++];
                sendlen = 1;
                buf_p[sendlen++] = 0xAA;
                buf_p[sendlen++] = E_DISP_STRING;
                if (argv[1][i] == 0x0a)
                    continue;
            }
            buf_p[sendlen++] = argv[1][i];
        }
        buf_p[sendlen++] = 0x00;
        buf_p[0] = sendlen;
    }
    else if (cur_pinfo_cmd == E_DISP_STRING) /* 变长命令*/
    {
        nlen = (nlen > (256 - 3)) ? (256 - 3) : nlen;
        for (i = 0; i < nlen; i++)
        {
            buf_p[sendlen++] = argv[1][i];
        }
        buf_p[sendlen++] = 0x00;
    }
    else /* 固定长度命令*/
    {
        if (argc < (nlen + 1))
        {
            trace_err("\tinput invalid param %d.\n", __LINE__);
            return RET_INVALID_PARAM;
        }
        for (i = 0; i < nlen; i++)
        {
            mode = strtoul(argv[1 + i], 0, 0);
            buf_p[sendlen++] = mode;
        }
    }
    /* 发送 数据*/
    if (cur_pinfo_cmd == E_DISP_STRING_2)
    {
        printf("cur_buf_point %d\n", cur_buf_point);
        for (i = 0; i < cur_buf_point; i++)
        {
            serial_fifoclear(0);
            ret = serial_send(comHandle, cmd_buf[i] + 1, cmd_buf[i][0]);
        }
    }
    else if (string_auto_line && cur_pinfo_cmd == E_DISP_STRING)
    {
        for (i = 0; i < cur_buf_point; i++)
        {
            serial_fifoclear(0);
            ret = serial_send(comHandle, cmd_buf[i] + 1, cmd_buf[i][0]);
            if (i % 8 == 7 && cur_buf_point != (i + 1))
            {
                sleep(string_clean_time);
                clear_screen(comHandle); /*清屏*/
            }
        }
    }
    else
    {
        serial_fifoclear(0);
        ret = serial_send(comHandle, buf_p, sendlen);
    }
    if (ret < 0 || ret != sendlen)
    {
        trace_err("frame send failed!\n");
    }

    /* 接收 数据*/
    uint8_t rbuf[BUF_SIZE];
    unsigned long starttick = 0;
    starttick = sys_getcounter();
    memset(rbuf, 0x00, sizeof(rbuf));
    nlen = BUF_SIZE;
    while (1)
    {
        if (serial_checkbuf(comHandle, nlen) == RET_SUCCESS)
        {
            ret = serial_fiforecv(comHandle, rbuf, nlen);
            if (ret == nlen)
            {
                ret = RET_SUCCESS;
                trace("recv data finish!\n");
                break;
            }
        }
        if (sys_getcounter() - starttick > 100)
        {
            ret = serial_fiforecv(comHandle, rbuf, nlen);
            trace_err("%d:recv outtime, or no data respond!\n", __LINE__);
            return RET_SUCCESS;
        }
    }
    trace("Recv:%s\n", rbuf);

    return ret;
}

int main(int argc, char *argv[])
{
    BOOL b = FALSE;
    HANDLE comHandle = INVALID_HANDLE_VALUE; /*串口句柄 */
    int ret = RET_SUCCESS;
    int baudrate = 9600;
    uint8_t argv_lcm_start_ = 3;
    /* 新增的*/
    busid = 1;

#if 0
    trace("test time and delay\n");
    unsigned long tick1, tick2 = 0;
    long clk1 = 0, clk2 = 0;
    tick1 = sys_gettickcounter();
    clk1 = sys_getcounter();
    time_need_delay(1000);
    tick2 = sys_gettickcounter();
    clk2 = sys_getcounter();
    trace("tick:%d\tclk:%d\n", (tick2-tick1), (clk2 - clk1));

    tick1 = sys_gettickcounter();
    clk1 = sys_getcounter();
    sys_delay_ms(1000);
    tick2 = sys_gettickcounter();
    clk2 = sys_getcounter();
    trace("tick:%d\tclk:%d\n", (tick2-tick1), (clk2 - clk1));
#endif

    gAppToolName = argv[0];

    if (argc < 2 || argc > 12)
    {
        usage(gAppToolName);
        return RET_INVALID_PARAM;
    }
    trace_line();

    switch (argv[1][1]) /* exe -v/-h 直接处理*/
    {
    case 'v':
        app_version_prt(gAppToolName);
        return RET_SUCCESS;
    case 'h':
        app_version_prt(gAppToolName);
        usage(gAppToolName);
        return RET_SUCCESS;
    }

    baudrate = strtoul(argv[1], 0, 0);
    busid = strtoul(argv[argv_lcm_start_ - 1], 0, 0);

    /*打开串口*/
    if (busid >= UART_PORT_MAX || busid < 0)
    {
        trace_err("input busid error,please input valid id,is [0-2]\n");
        return RET_INVALID_PARAM;
    }
    serial_fifoclear(0);
    const char *com = CFG_DEBUG_COMID;
    comHandle = serial_open(com, baudrate, 8, NOPARITY, ONESTOPBIT);
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        trace_err("OpenSerial %s fail!\r\n", com);
        return -1;
    }

    do
    {
        ret = argv_parse_proc(comHandle, argc - argv_lcm_start_, &argv[argv_lcm_start_]);
        if (ret == RET_UPDATE_PARAMETER_1)
        {
            argv_lcm_start_ += 1;
        }
        else if (ret == RET_UPDATE_PARAMETER_2)
        {
            argv_lcm_start_ += 2;
        }
        else
        {
            break;
        }

    } while (1);

exit_finish:
    /*关闭串口*/
    b = serial_close(comHandle);
    if (!b)
    {
        trace_err("CloseHandle fail\r\n");
    }

    trace_info("Program Execute %d\r\n", ret);
    return ret;
}
