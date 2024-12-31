#include <stdio.h>
#ifndef _MCU_UPDATETOOL_CPUARCH_
#include <windows.h>
#endif // #ifndef _MCU_UPDATETOOL_CPUARCH_
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include "ldl.h"
#include "nk_map.h"

#ifdef _MCU_UPDATETOOL_CPUARCH_ // linux 下需要兼容的 windows 命令和定义
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
#define CBR_230400 230400
// 文件路径解析
#define _MAX_PATH 260
#define _MAX_DRIVE 3
#define _MAX_DIR 256
#define _MAX_FNAME 256
#define _MAX_EXT 256

#if 1 // _splitpath
static void _split_whole_name(const char *whole_name, char *fname, char *ext)
{
    char *p_ext;

    p_ext = rindex(whole_name, '.');
    if (NULL != p_ext)
    {
        strcpy(ext, p_ext);
        snprintf(fname, p_ext - whole_name + 1, "%s", whole_name);
    }
    else
    {
        ext[0] = '\0';
        strcpy(fname, whole_name);
    }
}

void _splitpath(const char *path, char *drive, char *dir, char *fname, char *ext)
{
    char *p_whole_name;

    drive[0] = '\0';
    if (NULL == path)
    {
        dir[0] = '\0';
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }

    if ('/' == path[strlen(path)])
    {
        strcpy(dir, path);
        fname[0] = '\0';
        ext[0] = '\0';
        return;
    }

    p_whole_name = rindex(path, '/');
    if (NULL != p_whole_name)
    {
        p_whole_name++;
        _split_whole_name(p_whole_name, fname, ext);

        snprintf(dir, p_whole_name - path, "%s", path);
    }
    else
    {
        _split_whole_name(path, fname, ext);
        dir[0] = '\0';
    }
}
#endif // #if 1 // _splitpath

#endif // #ifndef _MCU_UPDATETOOL_CPUARCH_

#define CFG_DEBUG_COMID "COM5"

uint8_t busid;
uint8_t *pbinfile = NULL;
ST_DL_INFO_HOST gdlinfo;

#define trace_line() // printf("\t%s : %d\n", __func__, __LINE__)

#define my_free(p) \
    do             \
    {              \
        free(p);   \
        p = NULL;  \
    } while (0)

#if 1 // 开启DEBUG打印
#define LOGD(...) printf(__VA_ARGS__)
#else // 关闭DEBUG打印
#define LOGD(...)
#endif

#if 1 // 开启ERROR打印
#define LOGE(...) printf(__VA_ARGS__)
#else // 关闭ERROR打印
#define LOGE(...)
#endif

// 缓冲区大小
#define BUF_SIZE 2048
#define EXIT_STR "exit"
#define I_EXIT "I exit.\r\n"
#define I_RECEIVE "I receive.\r\n"

#if 1 // time API
#include <time.h>

#ifdef _MCU_UPDATETOOL_CPUARCH_ // linux 下需要兼容的 windows 命令和定义

static unsigned long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

#define sys_gettickcounter GetTickCount
#define sys_getcounter GetTickCount
#define sys_delay_ms(n) usleep(1000 * (n))
#else // windows
#define sys_gettickcounter GetTickCount
#define sys_getcounter clock
#define sys_delay_ms Sleep
#endif

void download_info(unsigned int a, unsigned int b)
{
    unsigned int i = (a * 100) / b;
    trace_err("Download: %d%% \r", i);
    fflush(stdout);
    if (100 == i)
    {
        trace_err("\ndownload complete!\n");
    }
}

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

uint16_t msb_byte2_to_uint16(const uint8_t byte[2])
{
    return ((byte[0] << 8) + byte[1]);
}

// scope:no data to return
void localdl_form_reurncode(struct LocalDlFrame *frame, uint16_t recode)
{
    frame->recode[1] = (uint8_t)recode;
    recode >>= 8;
    frame->recode[0] = (uint8_t)recode;
    // frame->length[1] = DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN + DL_LOCAL_ETXLEN;
    // frame->length[1] = 0;
}

void msb_uint16_to_byte2(uint16_t byte2, uint8_t byte[2])
{
    byte[1] = (uint8_t)byte2;
    byte2 >>= 8;
    byte[0] = (uint8_t)byte2;
}

#if 1

int file2buf(const char *path, unsigned char *buf, unsigned int buflen)
{
    FILE *fp;
    struct stat st_info;
    int rsize;
    trace("will open file :%s \n", path);

    if (!path || !buf || buflen <= 0)
    {
        trace_err("# Error:  L%d:param invalid error \r\n", __LINE__);
        return RET_INVALID_PARAM;
    }
    fp = fopen(path, "rb");

    if (fp == NULL)
    {
        trace_err("# Error: L%d:open %s file error \r\n", __LINE__, path);
        return RET_EXEC_ERROR;
    }
    else
    {
        if (stat(path, &st_info) < 0)
        {
            trace_err("# Error: L%d:stat %s file error \r\n", __LINE__, path);
            fclose(fp);
            return RET_BUSY_STATE;
        }
    }

    trace("fp_boot size: %d \r\n", st_info.st_size);

    if (st_info.st_size > buflen)
    {
        trace_err("L%d:file size = %d > %d, fail!\n", __LINE__, st_info.st_size, buflen);
        return RET_BUF_TOO_SMALL;
    }
    rsize = fread(buf, sizeof(char), st_info.st_size, fp);
    fclose(fp);
    return rsize;
}

int buf2file(const char *path, unsigned char *buf, unsigned int buflen)
{
    FILE *fp;
    int wsize;

    if (!path || !buf || buflen <= 0)
    {
        trace_err("# Error:  L%d:param invalid error \r\n", __LINE__);
        return RET_INVALID_PARAM;
    }

    fp = fopen(path, "wb+");

    if (!fp)
    {
        trace_err("# Error:  L%d:%s open fail \r\n", __LINE__, path);
        return RET_EXEC_ERROR;
    }

    wsize = fwrite(buf, sizeof(char), buflen, fp);

    if (wsize != buflen)
    {
        trace_err("L%d: %s write failed %d  %dB fail\r\n", __LINE__, path, wsize, buflen);
        return RET_LEN_ERROR;
    }

    trace("# file:%s write SUC.", path);

    fclose(fp);

    return wsize;
}
#endif

#ifndef _MCU_UPDATETOOL_CPUARCH_ // windows
// 打开串口
HANDLE OpenSerial(const char *com, // 串口名称，如COM1，COM2
                  int baud,        // 波特率：常用取值：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800
                  int byteSize,    // 数位大小：可取值7、8；
                  int parity,      // 校验方式：可取值NOPARITY、ODDPARITY、EVENPARITY、MARKPARITY、SPACEPARITY
                  int stopBits)    // 停止位：ONESTOPBIT、ONE5STOPBITS、TWOSTOPBITS；
{
    DCB dcb;
    BOOL b = FALSE;
    COMMTIMEOUTS CommTimeouts;
    HANDLE comHandle = INVALID_HANDLE_VALUE;

    // 打开串口
    comHandle = CreateFile(com,                          // 串口名称
                           GENERIC_READ | GENERIC_WRITE, // 可读、可写
                           0,                            // No Sharing
                           NULL,                         // No Security
                           OPEN_EXISTING,                // Open existing port only
                           FILE_ATTRIBUTE_NORMAL,        // Non Overlapped I/O
                           NULL);                        // Null for Comm Devices

    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("CreateFile fail\r\n");
        return comHandle;
    }

    // 设置读写缓存大小
    b = SetupComm(comHandle, BUF_SIZE, BUF_SIZE);
    if (!b)
    {
        LOGE("SetupComm fail\r\n");
    }

    // 设定读写超时
    CommTimeouts.ReadIntervalTimeout = MAXDWORD;   // 读间隔超时
    CommTimeouts.ReadTotalTimeoutMultiplier = 0;   // 读时间系数
    CommTimeouts.ReadTotalTimeoutConstant = 0;     // 读时间常量
    CommTimeouts.WriteTotalTimeoutMultiplier = 1;  // 写时间系数
    CommTimeouts.WriteTotalTimeoutConstant = 1;    // 写时间常量
    b = SetCommTimeouts(comHandle, &CommTimeouts); // 设置超时
    if (!b)
    {
        LOGE("SetCommTimeouts fail\r\n");
    }

    // 设置串口状态属性
    GetCommState(comHandle, &dcb);     // 获取当前
    dcb.BaudRate = baud;               // 波特率
    dcb.ByteSize = byteSize;           // 每个字节有位数
    dcb.Parity = parity;               // 无奇偶校验位
    dcb.StopBits = stopBits;           // 一个停止位
    b = SetCommState(comHandle, &dcb); // 设置
    if (!b)
    {
        LOGE("SetCommState fail\r\n");
    }

    return comHandle;
}
#endif /// #ifndef _MCU_UPDATETOOL_CPUARCH_ // windows

#define UART_PORT_MAX 30
#if (_MCU_UPDATETOOL_CPUARCH_ == _MCU_UPDATETOOL_CPUARCH_FREEX86)
static const char *uart_port_strlist[UART_PORT_MAX] = {
    "/dev/ttyu0", "/dev/ttyu1", "/dev/ttyu2"};
#else
static const char *uart_port_strlist[UART_PORT_MAX] = {
    "/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyUSB2", "/dev/ttyUSB3", "/dev/ttyUSB4", "/dev/ttyUSB5", "/dev/ttyUSB6", "/dev/ttyUSB7", "/dev/ttyUSB8", "/dev/ttyUSB9",
    "/dev/ttyAMA0", "/dev/ttyAMA1", "/dev/ttyAMA2", "/dev/ttysAMA3", "/dev/ttyAMA4", "/dev/ttyAMA5", "/dev/ttyAMA6", "/dev/ttyAMA7", "/dev/ttyAMA8", "/dev/ttyAMA9",
    "/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2", "/dev/ttyS3", "/dev/ttyS4", "/dev/ttyS5", "/dev/ttyS6", "/dev/ttyS7", "/dev/ttyS8", "/dev/ttyS9"};
#endif

int fd_serial = -1;
HANDLE serial_open(const char *com, // 串口名称，如COM1，COM2
                   int baud,        // 波特率：常用取值：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800
                   int byteSize,    // 数位大小：可取值7、8；
                   int parity,      // 校验方式：可取值NOPARITY、ODDPARITY、EVENPARITY、MARKPARITY、SPACEPARITY
                   int stopBits)    // 停止位：ONESTOPBIT、ONE5STOPBITS、TWOSTOPBITS；
{
#ifndef _MCU_UPDATETOOL_CPUARCH_ // windows
    return OpenSerial(com, baud, byteSize, parity, stopBits);
#else // linux
    int ret = -1;

    ret = UART0_Open(&fd_serial, (char *)uart_port_strlist[busid]);
    if (ret == -1)
    {
        trace_err("open %s failed: 0x%x", uart_port_strlist[busid], ret);
        return ret;
    }
    ret = UART0_Init(fd_serial, baud, 0, byteSize, stopBits, 'N');
    if (ret == -1)
    {
        printf("config %s failed: baudrate:%s", uart_port_strlist[busid], baud);
        return ret;
    }
#endif
    return fd_serial;
}

static int serial_close(HANDLE comHandle)
{
#ifndef _MCU_UPDATETOOL_CPUARCH_ // windows
    return CloseHandle(comHandle);
#else // linux
    UART0_Close(comHandle);
    return 1;
#endif
}

static int serial_send(HANDLE comHandle, uint8_t *pbuf, int len)
{
    BOOL b = FALSE;
    int reallen = 0;
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("serial send failed, com error!\r\n");
        return -1;
    }
    if (NULL == pbuf)
    {
        LOGE("serial send failed, buffer error!\r\n");
        return -2;
    }
    if (0 == len)
    {
        LOGE("serial send failed, len error!\r\n");
        return -3;
    }

    // 回复
    trace_buf("send frame", pbuf, len);
#ifdef _MCU_UPDATETOOL_CPUARCH_
    reallen = UART0_Send(comHandle, pbuf, len);
#else
    b = WriteFile(comHandle, pbuf, len, &reallen, NULL);
#endif
    if (!b)
    {
        LOGE("send fail\r\n");
    }
    return reallen;
}

typedef struct
{
    uint8_t flag; // 0 无数据 1 有数据
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
        LOGE("serial recv failed, com error!\r\n");
        return -1;
    }
    if (NULL == pbuf)
    {
        LOGE("serial recv failed, buffer error!\r\n");
        return -2;
    }
    if (0 == len)
    {
        LOGE("serial recv failed, len error!\r\n");
        return -3;
    }
#ifdef _MCU_UPDATETOOL_CPUARCH_
    reallen = UART0_Recv(comHandle, pbuf, len);
#else
    b = ReadFile(comHandle, pbuf, len, &reallen, NULL);
#endif
    if (!b)
    {
        LOGE("recv fail\r\n");
    }
    return reallen;
}

static int serial_checkbuf(HANDLE comHandle, uint32_t nlen)
{
    int ret, i;
    int iret = RET_LEN_ERROR;
    ST_SERIAL_FIFO_TEMP *pfifo = &st_serial_fifo;
    // if (pfifo->flag) // 有数据
    //  {
    //      if (pfifo->fifolen > nlen)
    //      {
    //          trace("recv fifo already data %d\n", pfifo->fifolen);
    //          return RET_SUCCESS;
    //      }
    //  }
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
        if (ret > 0) // 读取数据
        {
            if (ret == nlen) // 读取到等额数据
            {
                pfifo->fifolen += nlen;
                trace("recv fifo read data %d\n", pfifo->fifolen);
                iret = RET_SUCCESS;
                break;
            }
            // 读取较少数据
            pfifo->fifolen += ret;
            nlen -= ret;
        }
    }
    return iret;
}

static int serial_fiforecv(HANDLE comHandle, uint8_t *pbuf, int len)
{
    ST_SERIAL_FIFO_TEMP *pfifo = &st_serial_fifo;
    // if (pfifo->flag) // 有数据
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

static int serial_recvframe(HANDLE comHandle, uint8_t *pbuf, int len)
{
    int reallen = 0;
    uint8_t buf[BUF_SIZE] = {0};
    uint16_t index;
    unsigned long starttick = 0;
    uint32_t length = 0;
    int ret;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;

    length = 1;
    index = 0;
    starttick = sys_getcounter();
    memset(buf, 0x00, sizeof(buf));

    serial_fifoclear(0); // 接收数据帧之前，先清空历史缓冲区数据

    while (1)
    {
        if (serial_checkbuf(comHandle, length) == RET_SUCCESS)
        {
            ret = serial_fiforecv(comHandle, buf, length);
            if (ret == length)
            {
                if (buf[index] == DL_LOCAL_STX)
                {
                    trace("-|->Recv: STX\n");
                    index++;
                    break;
                }
            }
        }
        if (sys_getcounter() - starttick > DL_TIME_WAIT)
        {
            trace_err("%d:recv outtime\n", __LINE__);
            return RET_TIMEOUT_ERR;
        }
    }

    length = DL_LOCAL_FIXEDLEN;
    starttick = sys_getcounter();
    while (1)
    {
        if (serial_checkbuf(comHandle, length) == RET_SUCCESS)
        {
            ret = serial_fiforecv(comHandle, buf + index, length);
            if (ret)
            {
                trace_buf("recv_data", buf + index, ret);
            }
            if (ret == length)
            {
                if (buf[0] == DL_LOCAL_STX)
                {
                    trace("-|->Recv: info\n");
                    index += length;
                    break;
                }
            }
        }
        if (sys_getcounter() - starttick > DL_TIME_WAIT)
        {
            trace_buf("recvfifo-finish", st_serial_fifo.fifobuf, st_serial_fifo.fifolen);

            trace_err("%d:recv outtime\n", __LINE__);
            return RET_TIMEOUT_ERR;
        }
    }

    length = pframe->length[0] << 8 | pframe->length[1];
    trace("current frame bin length=%d\n", length);
    starttick = sys_getcounter();
    while (length)
    {
        if (serial_checkbuf(comHandle, length) == RET_SUCCESS)
        {
            ret = serial_fiforecv(comHandle, buf + index, length);
            if (ret == length)
            {
                if (buf[0] == DL_LOCAL_STX)
                {
                    trace("-|->Recv: bin\n");
                    index += length;
                    break;
                }
            }
        }
        if (sys_getcounter() - starttick > DL_TIME_WAIT)
        {
            trace_err("%d:recv outtime\n", __LINE__);
            return RET_TIMEOUT_ERR;
        }
        sys_delay_ms(10);
    }
    reallen = index;

    uint16_t crc16 = 0;
    uint16_t crc16old = 0;
    // uint16_t length;
    //  帧头
    if (pframe->stx != DL_LOCAL_STX)
    {
        crc16 = RET_INVALID_PARAM;
        trace("frame start header lose!\n");
        goto ERR_CHK;
    }
    length = msb_byte2_to_uint16(pframe->length);
    // 帧内容长度是否符合: 总长度-帧校验信息长度，剩下的都是有效bin数据长度，应当和length一致
    if ((index - DL_LOCAL_STXLEN - DL_LOCAL_FIXEDLEN) != length)
    {
        crc16 = RET_LEN_ERROR;
        trace("frame len uncompatibled!\n");
        goto ERR_CHK;
    }
    trace_buf("frame bin:", pframe->bin, length);
    crc16 = calc_crc16(pframe->bin, length, 0);
    crc16old = pframe->crc[0] << 8 | pframe->crc[1];
    if (crc16old != crc16)
    {
        trace("frame check failed %x %x!\n", crc16old, crc16);
        crc16 = RET_EXEC_ERROR;
        goto ERR_CHK;
    }
    if (pframe->recode[0] || pframe->recode[1])
    {
        crc16 = RET_VERIFY_CRCERR;
        trace("frame parse error!\n");
        goto ERR_CHK;
    }
    if (pbuf && len > reallen)
    {
        memcpy(pbuf, buf, reallen);
        trace("obain data %d \n", reallen);
        trace_buf("recv frame", buf, reallen);
    }

    //    LTRACE("->chkOK");
    return reallen;
ERR_CHK:
    //    LTRACE("->chk:%d",crc16);
    trace_err("recv frame err:%d\n", crc16);
    localdl_form_reurncode(pframe, crc16);
    return RET_VERIFY_CRCERR;
}

/*
1. 0xaa 0xff 进入下载模式  终端返回
2. 等待终端发送握手命令 $A   回复$A
3. 获取终端信息
* 下载请求  传输数据  保存文件 结束下载
*/
// 握手
int dl_handshake(int mode, HANDLE comHandle)
{
    uint8_t buf[BUF_SIZE] = {0};
    int ret;
    trace("handshake begin...\n");

    unsigned long starttick = 0;
    starttick = sys_getcounter();
    ret = serial_send(comHandle, INFLASH_WRITE_STR, INFLASH_WRITE_STRLEN);
    ret = serial_send(comHandle, INFLASH_WRITE_B_STR, INFLASH_WRITE_B_STRLEN);
    while (1)
    {
        memset(buf, 0x00, BUF_SIZE);
        // if (serial_checkbuf(comHandle, 2) != 0 ) {
        //     continue;
        // }
        // ret = serial_fiforecv(comHandle, buf, 2);
        ret = serial_recv(comHandle, buf, 2);
        if (ret > 0)
        {
            if (memcmp(buf, INFLASH_WRITE_STR, INFLASH_WRITE_STRLEN) == 0)
            {
                ret = serial_send(comHandle, INFLASH_WRITE_STR, INFLASH_WRITE_STRLEN);
                trace("handshake ok!\n");
                ret = RET_SUCCESS;
                break;
            }
            if (memcmp(buf, INFLASH_WRITE_B_STR, INFLASH_WRITE_B_STRLEN) == 0)
            {
                ret = serial_send(comHandle, INFLASH_WRITE_B_STR, INFLASH_WRITE_B_STRLEN);
                trace("handshake ok!\n");
                ret = RET_SUCCESS;
                break;
            }
        }
        if (sys_getcounter() - starttick > (DL_TIME_WAIT * 2))
        {
            trace_err("%s:handshake outtime\n");
            return RET_TIMEOUT_ERR;
        }
    }
    return ret;
}

// 获取终端信息
int dl_terminalinfo_get(int mode, HANDLE comHandle)
{
    struct LocalDlFrame frame;
    int ret;
    uint16_t crc16 = 0;
    uint32_t length = 0;
    uint8_t retry = 3;
    uint8_t buf[BUF_SIZE] = {0};
    int reallen = 0;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;

send_again:
    memset(&frame, 0x00, sizeof(frame));
    memset(&buf, 0x00, sizeof(buf));

    frame.stx = DL_LOCAL_STX;
    frame.cmd = DLCMD_GETTERMINFO;
    frame.bin[0] = mode & 0xff; // 0:version  1:resource
    length = 1;
    if (length)
        crc16 = calc_crc16(frame.bin, length, 0);
    frame.crc[0] = crc16 >> 8;
    frame.crc[1] = crc16 & 0xff;

    localdl_form_reurncode(&frame, RET_SUCCESS);
    msb_uint16_to_byte2(length, frame.length);
    // 发送 命令
    length += DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN;
    ret = serial_send(comHandle, (uint8_t *)&frame, length);
    if (ret != length)
    {
        trace_err("%s : send length err %d  %d\n", __func__, ret, length);
        return -RET_LEN_ERROR;
    }

    // 等待回复 和 确认重发
    reallen = serial_recvframe(comHandle, buf, sizeof(buf));
    if (reallen <= 0)
    {
        if (retry)
        {
            trace("%s recv failed, retry again\n", __func__);
            retry--;
            goto send_again;
        }
        ret = reallen;
    }
    else
    {
        trace("get terminal %d info suc.\n", mode);
        trace_err("terminal%d:\n%s", mode, pframe->bin);
        ret = RET_SUCCESS;
    }

    return ret;
}

#if 1
// 发送文件下载请求
int dl_fileinfo_sendrequest(int mode, HANDLE comHandle, ST_DL_INFO_HOST *pdlinfo)
{
    struct LocalDlFrame frame;
    int ret;
    uint16_t crc16 = 0;
    uint32_t length = 0;
    uint8_t retry = DL_RETRY_SEND_NUM;
    uint8_t buf[BUF_SIZE] = {0};
    int reallen = 0;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;

send_again:
    memset(&frame, 0x00, sizeof(frame));
    memset(&buf, 0x00, sizeof(buf));

    frame.stx = DL_LOCAL_STX;
    frame.cmd = DLCMD_LOADFILE;
    length = sizeof(ST_DL_INFO);
    memcpy(frame.bin, (uint8_t *)pdlinfo, length);
    // frame.bin[0] = mode & 0xff; // 0:version  1:resource
    if (length)
        crc16 = calc_crc16(frame.bin, length, 0);
    frame.crc[0] = crc16 >> 8;
    frame.crc[1] = crc16 & 0xff;

    localdl_form_reurncode(&frame, RET_SUCCESS);
    msb_uint16_to_byte2(length, frame.length);
    // 发送 命令
    length += DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN;
    ret = serial_send(comHandle, (uint8_t *)&frame, length);
    if (ret != length)
    {
        trace_err("%s : send length err %d  %d\n", __func__, ret, length);
        return -RET_LEN_ERROR;
    }

    // 等待回复 和 确认重发
    reallen = serial_recvframe(comHandle, buf, sizeof(buf)); // 接收 和 校验
    if (reallen <= 0)
    {
        if (retry)
        {
            trace("%s recv failed, retry again\n", __func__);
            retry--;
            goto send_again;
        }
        ret = reallen;
    }
    else
    {
        retry = DL_RETRY_SEND_NUM;
        trace("request download suc.\n");
        ret = RET_SUCCESS;
    }

    return ret;
}

// 发送文件
int dl_fileinfo_senddata(int mode, HANDLE comHandle, ST_DL_INFO_HOST *pdlinfo)
{
    struct LocalDlFrame frame;
    int ret;
    uint16_t crc16 = 0;
    uint32_t length = 0;
    uint8_t retry = DL_RETRY_SEND_NUM;
    uint8_t buf[BUF_SIZE] = {0};
    int reallen = 0;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;
    uint32_t sendindex = 0;

    pdlinfo->offset = 0;
    pdlinfo->framelen = 0;
    while (pdlinfo->datalen != pdlinfo->offset)
    {
        download_info(pdlinfo->offset, pdlinfo->datalen);
        memset(&frame, 0x00, sizeof(frame));
        memset(&buf, 0x00, sizeof(buf));
        pdlinfo->framelen = pdlinfo->datalen - pdlinfo->offset;
        if (pdlinfo->framelen > DL_CFG_PACKLEN)
        {
            pdlinfo->framelen = DL_CFG_PACKLEN;
        }
        frame.stx = DL_LOCAL_STX;
        frame.cmd = DLCMD_TRANSDATA;
        length = pdlinfo->framelen;
        memcpy(frame.bin, pdlinfo->pdata + pdlinfo->offset, length);

        if (length)
            crc16 = calc_crc16(frame.bin, length, 0);
        frame.crc[0] = crc16 >> 8;
        frame.crc[1] = crc16 & 0xff;

        localdl_form_reurncode(&frame, RET_SUCCESS);
        msb_uint16_to_byte2(length, frame.length);
        // 发送 命令
        length += DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN;
        ret = serial_send(comHandle, (uint8_t *)&frame, length);
        if (ret != length)
        {
            trace_err("%s : send length err %d  %d\n", __func__, ret, length);
            return -RET_LEN_ERROR;
        }

        // 等待回复 和 确认重发
        reallen = serial_recvframe(comHandle, buf, sizeof(buf)); // 接收 和 校验
        if (reallen <= 0)
        {
            if (retry)
            {
                trace_info("%s recv failed, retry again\n", __func__);
                retry--;
                continue;
            }
            ret = reallen;
            trace_err("%s recv failed %d, %d:%d exit!\n", __func__, sendindex, pdlinfo->offset, pdlinfo->datalen);
            break;
        }
        else
        {
            retry = DL_RETRY_SEND_NUM;
            trace("transfer file suc.\n");
            ret = RET_SUCCESS;
        }
        pdlinfo->offset += pdlinfo->framelen;
        sendindex++;
    }
    download_info(pdlinfo->offset, pdlinfo->datalen);

    return ret;
}

// 保存文件
int dl_fileinfo_savefile(int mode, HANDLE comHandle, ST_DL_INFO_HOST *pdlinfo)
{
    struct LocalDlFrame frame;
    int ret;
    uint16_t crc16 = 0;
    uint32_t length = 0;
    uint8_t retry = 0; // DL_RETRY_SEND_NUM;
    uint8_t buf[BUF_SIZE] = {0};
    int reallen = 0;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;

send_again:
    memset(&frame, 0x00, sizeof(frame));
    memset(&buf, 0x00, sizeof(buf));

    frame.stx = DL_LOCAL_STX;
    frame.cmd = DLCMD_SAVEFILE;
    length = 0;

    if (length)
        crc16 = calc_crc16(frame.bin, length, 0);
    frame.crc[0] = crc16 >> 8;
    frame.crc[1] = crc16 & 0xff;

    localdl_form_reurncode(&frame, RET_SUCCESS);
    msb_uint16_to_byte2(length, frame.length);
    // 发送 命令
    length += DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN;
    ret = serial_send(comHandle, (uint8_t *)&frame, length);
    if (ret != length)
    {
        trace_err("%s : send length err %d  %d\n", __func__, ret, length);
        return -RET_LEN_ERROR;
    }

    // 等待回复 和 确认重发
    reallen = serial_recvframe(comHandle, buf, sizeof(buf)); // 接收 和 校验
    if (reallen <= 0)
    {
        if (retry)
        {
            trace("%s recv failed, retry again\n", __func__);
            retry--;
            goto send_again;
        }
        ret = reallen;
    }
    else
    {
        retry = DL_RETRY_SEND_NUM;
        trace("save file suc.\n");
        ret = RET_SUCCESS;
    }

    return ret;
}

// 结束退出下载
int dl_fileinfo_finishdl(int mode, HANDLE comHandle, ST_DL_INFO_HOST *pdlinfo)
{
    struct LocalDlFrame frame;
    int ret;
    uint16_t crc16 = 0;
    uint32_t length = 0;
    uint8_t retry = DL_RETRY_SEND_NUM;
    uint8_t buf[BUF_SIZE] = {0};
    int reallen = 0;
    struct LocalDlFrame *pframe = (struct LocalDlFrame *)&buf;

send_again:
    memset(&frame, 0x00, sizeof(frame));
    memset(&buf, 0x00, sizeof(buf));

    frame.stx = DL_LOCAL_STX;
    frame.cmd = DLCMD_FINISH;
    length = 0;

    if (length)
        crc16 = calc_crc16(frame.bin, length, 0);
    frame.crc[0] = crc16 >> 8;
    frame.crc[1] = crc16 & 0xff;

    localdl_form_reurncode(&frame, RET_SUCCESS);
    msb_uint16_to_byte2(length, frame.length);
    // 发送 命令
    length += DL_LOCAL_STXLEN + DL_LOCAL_FIXEDLEN;
    ret = serial_send(comHandle, (uint8_t *)&frame, length);
    if (ret != length)
    {
        trace_err("%s : send length err %d  %d\n", __func__, ret, length);
        return -RET_LEN_ERROR;
    }

    // 等待回复 和 确认重发
    reallen = serial_recvframe(comHandle, buf, sizeof(buf)); // 接收 和 校验
    if (reallen <= 0)
    {
        if (retry)
        {
            trace("%s recv failed, retry again\n", __func__);
            retry--;
            goto send_again;
        }
        ret = reallen;
    }
    else
    {
        retry = DL_RETRY_SEND_NUM;
        trace("finish download suc.\n");
        ret = RET_SUCCESS;
    }

    return ret;
}
#endif

#define CFG_BMP2BIN_DL 1

#define CFG_CHANGE_INDEXBMP 1
#if CFG_CHANGE_INDEXBMP

#ifndef LHALFB
#define LHALFB(x) (unsigned char)((x) & 0x0F) // 获取低半字节
#endif
#ifndef HHALFB
#define HHALFB(x) (unsigned char)(((x) >> 4) & 0x0F) // 获取高半字节
#endif

/*
 * c_asc2hex - [GENERIC] 2个字节ASC码转为1个字节16进制
 *    输入: 2个字节，高位在前，低位在后
 *     例如：'3''4' -> 0x34
 *    输出: 1个字节，16进制
 *    返回值:  0-成功 1-失败
 * @
 */
int c_asc2hex(unsigned char *ucBuffer, unsigned char *outbuf)
{
    unsigned char tmp;
    if (ucBuffer[0] >= '0' && ucBuffer[0] <= '9')
    {
        tmp = (unsigned char)((ucBuffer[0] - '0') << 4);
    }
    else if (ucBuffer[0] >= 'a' && ucBuffer[0] <= 'f')
    {
        tmp = (unsigned char)((ucBuffer[0] - 'a' + 10) << 4);
    }
    else if (ucBuffer[0] >= 'A' && ucBuffer[0] <= 'F')
    {
        tmp = (unsigned char)((ucBuffer[0] - 'A' + 10) << 4);
    }
    else
    {
        return 1;
    }
    if (ucBuffer[1] >= '0' && ucBuffer[1] <= '9')
    {
        tmp |= (unsigned char)(ucBuffer[1] - '0');
    }
    else if (ucBuffer[1] >= 'a' && ucBuffer[1] <= 'f')
    {
        tmp |= ((ucBuffer[1] - 'a') + 10);
    }
    else if (ucBuffer[1] >= 'A' && ucBuffer[1] <= 'F')
    {
        tmp |= ((ucBuffer[1] - 'A') + 10);
    }
    else
    {
        return 1;
    }
    *outbuf = tmp;
    return 0;
} /* -----  end of function c_asc2hex  ----- */

/*
 * c_hex2asc - [GENERIC] 1个字节16进制转为2字节ASC码
 *    输入: 1个字节16进制
 *     例如：0x3B ->  '3''B'
 *    输出: 2个字节ASC码
 * @
 */
int c_hex2asc(unsigned char ucData, unsigned char *pucDataOut)
{
    unsigned char tmp;
    tmp = HHALFB(ucData);
    if (tmp >= 10)
    {
        tmp += ('A' - 10);
    }
    else
    {
        tmp += '0';
    }
    pucDataOut[0] = tmp;
    tmp = LHALFB(ucData);
    if (tmp >= 10)
    {
        tmp += ('A' - 10);
    }
    else
    {
        tmp += '0';
    }
    pucDataOut[1] = tmp;
    return 0;
}

/*
 * c_str2hex - [GENERIC] 字符串转为16进制输出
 *      '1' 'A' -> 0x1A
 *    输入: in_len字节字符串
 *    输出: in_len/2字节16进制
 *    返回: 0-成功  1-失败(非法asc码字符)
 * @
 */
int c_str2hex(uint32_t in_len, const void *inbuf, void *outbuf)
{
    uint32_t i;
    uint8_t *in = (uint8_t *)inbuf;
    uint8_t *out = (uint8_t *)outbuf;

    if (in_len % 2)
    {
        return 1;
    }
    for (i = 0; i < in_len / 2; i++)
    {
        if (c_asc2hex(&in[2 * i], &out[i]))
        {
            return 1;
        }
    }
    return 0;
}

/*
 * c_hex2str - [GENERIC] 16进制数组转为字符串输出
 *        0x1A-> '1' 'A'
 *    输入: in_len字节16进制数组
 *    输出: in_len*2字节字符串
 *    返回: 0-成功
 * @
 */
int c_hex2str(uint32_t in_len, const void *inbuf, void *outbuf)
{
    uint32_t i;
    uint8_t *in = (uint8_t *)inbuf;
    uint8_t *out = (uint8_t *)outbuf;
    for (i = 0; i < in_len; i++)
    {
        c_hex2asc(in[i], &out[2 * i]);
    }
    return 0;
}

int index_changefile_str2asc(char *path, char *pw, char *ph)
{
    int fdbin;
    struct stat buf;
    uint8_t *poutfile = NULL;
    uint8_t *pinfile = NULL;
    unsigned long long outlen;
    int ret;
    if (NULL == path)
    {
        trace_err("file path err!");
        return -1;
    }
    // 打开文件
    fdbin = open(path, O_RDONLY);
    if (fdbin < 0)
    {
        trace_err("%s file open fail,res = %d.\r\n", path, fdbin);
        return -1;
    }

    fstat(fdbin, &buf);
    trace_info("%s file size:%ld\n ", path, buf.st_size);

    pinfile = (unsigned char *)malloc(buf.st_size);
    outlen = buf.st_size << 2 + sizeof(BMP_FILE_DESC);
    poutfile = (unsigned char *)malloc(outlen);

    if (NULL == pinfile)
    {
        trace_err("malloc size = %ld  fail!\n", buf.st_size);
        return -1;
    }
    if (NULL == poutfile)
    {
        my_free(pinfile);
        trace_err("malloc size = %ld  fail!\n", buf.st_size);
        return -1;
    }
    memset(pinfile, 0x00, buf.st_size);
    memset(poutfile, 0x00, outlen);

    // 读取内容
    ret = read(fdbin, pinfile, buf.st_size);
    if (ret < 0)
    {
        trace_err("read bin fail,res = %d.\r\n", ret);
        my_free(pinfile);
        return -1;
    }

    // 关闭文件
    close(fdbin);

    BMP_FILE_DESC *pbmp_info = (BMP_FILE_DESC *)poutfile;

    pbmp_info->width = strtoul(pw, 0, 0);
    pbmp_info->heigth = strtoul(ph, 0, 0);
    pbmp_info->datalen = buf.st_size >> 1;
    trace_info("w=%d\th=%d\tlen=%d\n", pbmp_info->width, pbmp_info->heigth, pbmp_info->datalen);

    trace_err_buf("change1:", poutfile, sizeof(BMP_FILE_DESC));

    ret = c_str2hex(buf.st_size, pinfile, &poutfile[sizeof(BMP_FILE_DESC)]);
    trace_info("change file is %d\n", ret);
    if (ret)
    {
        trace_err("change file failed!\n");
        return -1;
    }
    trace_err_buf("change2:", poutfile, sizeof(BMP_FILE_DESC));

    // 解析传入的文件路径 在相同路径下建立 新的名称文件
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    uint8_t newpath[1024];

    _splitpath(path, drive, dir, fname, ext);
    trace_info("parse %s -> %s\t%s\t%s\t%s\n", path, drive, dir, fname, ext);

    memset(newpath, 0x00, sizeof(newpath));

    sprintf(newpath, "%s%s%s.bin", drive, dir, fname);
    trace_info("newpath=%s\n", newpath);

    // 整合BMP头部信息  并计算CRC
    if (strlen(fname) > sizeof(pbmp_info->bmp_desc))
    {
        memcpy(pbmp_info->bmp_desc, fname, 30);
    }
    else
    {
        memcpy(pbmp_info->bmp_desc, fname, strlen(fname));
    }
    pbmp_info->bmp_desc[31] = 0x00;
    pbmp_info->bmp_index = BMP_COMPANY_LOGO_INDEX;

    pbmp_info->crc32 = Calc_crc32((const char *)pbmp_info, sizeof(BMP_FILE_DESC) - 4);
    trace_err_buf("change3:", poutfile, sizeof(BMP_FILE_DESC));

    buf2file(newpath, poutfile, (buf.st_size >> 1) + sizeof(BMP_FILE_DESC));

    my_free(pinfile);
    my_free(poutfile);
    return 0;
}

#endif

#if CFG_BMP2BIN_DL
static char c_bmp2binpath[256];

#if 1

// ��?????
typedef struct __attribute__((packed)) BMP_FILE_HEAD
{
    unsigned short bfType;      // ??????????????0x424D??????????BM
    unsigned int bfSize;        // ?????��???????
    unsigned short bfReserved1; // ??????
    unsigned short bfReserved2; // ??????
    unsigned int bfOffBits;     // ??????????????????????????
} BMP_FILE_HEAD;

// ��?????
typedef struct __attribute__((packed)) BMP_INFO_HEAD
{
    unsigned int biSize;         // ?????????????40???
    int biWidth;                 // ???????
    int biHeight;                // ???????
    unsigned short biPlanes;     // ??????1
    unsigned short biBitCount;   // ?????????????��??????????? 1??????????,4??16 ????,8??256 ???,24????????????��?.bmp ?????? 32 ��??????????????
    unsigned int biCompression;  // ???��???????????��???? BI_RGB??BI_RLE8??BI_RLE4??BI_BITFIELDS???????��Windows???????????????????BI_RGB????????????
    unsigned int biSizeImage;    // ???????��??????????????
    int biXPelsPerMeter;         // ???????��?????????
    int biYPelsPerMeter;         // ???????��?????????
    unsigned int biClrUsed;      // ????????????????????????????????????????????? 2 ?? biBitCount ?��?
    unsigned int biClrImportant; // ???????????????????????????????????????��?????????????
} BMP_INFO_HEAD;

typedef enum
{
    BITMAP_MODE_RL = 1,
    BITMAP_MODE_RM = 2,
    BITMAP_MODE_CL = 3,
    BITMAP_MODE_CM = 4,
    BITMAP_MODE_RCL = 5,
    BITMAP_MODE_RCM = 6,
    BITMAP_MODE_CRL = 7,
    BITMAP_MODE_CRM = 8,
    BITMAP_MODE_INVALID,
} bitmap_mode_e;

typedef enum
{
    FMT_BITMAP = 0,
    FMT_WEB = 1,
    FMT_RGB565 = 2,
    FMT_BGR565 = 3,
    FMT_ARGB1555 = 4,
    FMT_INVALID,
} fmt_e;

typedef struct
{
    fmt_e fmt;
    char fmt_str[12];
    void (*color_convert)(uint8_t *in, uint8_t *out, int h, int v);
} fmt_s;

FILE *fpwb = NULL; // ?????????????
FILE *fpwc = NULL; // ???C????
FILE *fpr = NULL;
BMP_FILE_HEAD BFH;
BMP_INFO_HEAD BIH;
unsigned char *bmp_data = NULL;
unsigned char *rgb888_data = NULL;
unsigned char *out_data = NULL;
uint32_t out_data_size = 0;

const fmt_s *aim_fmt = NULL;

int32_t bitmap_mode = BITMAP_MODE_CRL; // ???
int32_t reverse_color = 1;             // ???
int32_t luminance = 128;               // ????

void rgb888_to_bitmap(uint8_t *in, uint8_t *out, int h, int v);

const fmt_s format_preset[] = {
    {FMT_BITMAP, "bitmap", rgb888_to_bitmap},
};

void rgb888_to_bitmap(uint8_t *in, uint8_t *out, int h, int v)
{
    int32_t x = 0, y = 0;
    int32_t avg = 0;

    int32_t he = 0;
    int32_t ve = 0;

    switch (bitmap_mode)
    {
    case BITMAP_MODE_RL:
    case BITMAP_MODE_RM:
    case BITMAP_MODE_RCL:
    case BITMAP_MODE_RCM:
        he = (h + 7) >> 3;
        ve = v;
        break;
    case BITMAP_MODE_CL:
    case BITMAP_MODE_CM:
    case BITMAP_MODE_CRL:
    case BITMAP_MODE_CRM:
        he = h;
        ve = (v + 7) >> 3;
        break;
    default:
        break;
    }

    for (y = 0; y < v; y++)
    {
        for (x = 0; x < h; x++)
        {
            avg = in[y * h * 3 + x * 3] + in[y * h * 3 + x * 3 + 1] + in[y * h * 3 + x * 3 + 2];
            if ((reverse_color == 0 && avg < luminance) ||
                (reverse_color == 1 && avg > luminance))
            {
                continue;
            }

            switch (bitmap_mode)
            {
            case BITMAP_MODE_RL:
                out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x01 << (x & 0x07));
                break;
            case BITMAP_MODE_RM:
                out[he * y + (x >> 3)] = out[he * y + (x >> 3)] | (0x80 >> (x & 0x07));
                break;
            case BITMAP_MODE_CL:
                out[ve * x + (y >> 3)] = out[ve * x + (y >> 3)] | (0x01 << (y & 0x07));
                break;
            case BITMAP_MODE_CM:
                out[ve * x + (y >> 3)] = out[ve * x + (y >> 3)] | (0x80 >> (y & 0x07));
                break;
            case BITMAP_MODE_RCL:
                out[y + v * (x >> 3)] = out[y + v * (x >> 3)] | (0x01 << (x & 0x07));
                break;
            case BITMAP_MODE_RCM:
                out[y + v * (x >> 3)] = out[y + v * (x >> 3)] | (0x80 >> (x & 0x07));
                break;
            case BITMAP_MODE_CRL:
                out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x01 << (y & 0x07));
                break;
            case BITMAP_MODE_CRM:
                out[x + h * (y >> 3)] = out[x + h * (y >> 3)] | (0x80 >> (y & 0x07));
                break;
            default:
                break;
            }
        }
    }
}

void bmp_to_rgb888(uint8_t *in, uint8_t *out, int h, int v)
{
    int32_t i = 0, j = 0;
    unsigned int offset = 0;
    unsigned int line_size = 0;

    // a=14,b=5
    //??????a/b=2
    //??????(a+b-1)/b=3
    //  (((width*biBitCount+7)/8+3)/4*4
    //  (((width*biBitCount)+31)/32)*4
    //  (((width*biBitCount)+31)>>5)<<2
    //  ((width*3)+3)/4*4 // 24bit
    //  line_size = (((h * 24) + 31) >> 5) << 2;
    line_size = ((h * 3 + 3) >> 2) << 2;

    for (i = 0; i < v; i++)
    {
        for (j = 0; j < h; j++)
        {
            offset = (v - i - 1) * line_size + j * 3;

            out[i * h * 3 + j * 3] = in[offset + 2];
            out[i * h * 3 + j * 3 + 1] = in[offset + 1];
            out[i * h * 3 + j * 3 + 2] = in[offset];
        }
    }
}

int convert(char *filename)
{
    if ((fpr = fopen(filename, "rb")) == NULL)
    {
        printf("\nopen %s error, or convert finish\n", filename);
        return 1;
    }
    printf("\rconverting file %s ", filename);
    fflush(stdout);

    fseek(fpr, BFH.bfOffBits, SEEK_SET);
    fread(bmp_data, BIH.biSizeImage, 1, fpr);
    fclose(fpr);

    bmp_to_rgb888(bmp_data, rgb888_data, BIH.biWidth, BIH.biHeight);
    // dbg_rgb888_dump_ppm("dump.ppm", rgb888_data, BIH.biWidth, BIH.biHeight);
    memset(out_data, 0, out_data_size);
    aim_fmt->color_convert(rgb888_data, out_data, BIH.biWidth, BIH.biHeight);

    // fwrite(out_data, 1, out_data_size, fpwb);

    // bin2array_convert(&fpwc, (void *)out_data, out_data_size, sizeof(unsigned char));

    return 0;
}

int bmp_read_head(FILE *fp)
{
    fseek(fp, 0, SEEK_SET);

    fread(&BFH, sizeof(BFH), 1, fp); // ��ȡBMP�ļ�ͷ
    fread(&BIH, sizeof(BIH), 1, fp); // ��ȡBMP��Ϣͷ��40�ֽڣ�ֱ���ýṹ���?

    trace("\nBMP file head\n");
    trace("bfType = %x\n", BFH.bfType);
    trace("bfSize = %d\n", BFH.bfSize);
    trace("bfReserved1 = %d\n", BFH.bfReserved1);
    trace("bfReserved2 = %d\n", BFH.bfReserved2);
    trace("bfOffBits = %d\n", BFH.bfOffBits);

    trace("\nBMP info head\n");
    trace("biSize = %d\n", BIH.biSize);
    trace("biWidth = %d\n", BIH.biWidth);
    trace("biHeight = %d\n", BIH.biHeight);
    trace("biPlanes = %d\n", BIH.biPlanes);
    trace("biBitCount = %d\n", BIH.biBitCount);
    trace("biCompression = %d\n", BIH.biCompression);
    trace("biSizeImage = %d\n", BIH.biSizeImage);
    trace("biXPelsPerMeter = %d\n", BIH.biXPelsPerMeter);
    trace("biYPelsPerMeter = %d\n", BIH.biYPelsPerMeter);
    trace("biClrUsed = %d\n", BIH.biClrUsed);
    trace("biClrImportant = %d\n", BIH.biClrImportant);

    // if((BFH.bfType != 0x424D) || (BIH.biClrImportant != 0))
    if ((BFH.bfType != 0x4D42))
    {
        trace_err("\nnot bmp file\n");
        return 1;
    }

    if (BIH.biBitCount != 24 || ((BIH.biClrImportant != 0) && (BIH.biClrImportant != 16777216)))
    {
        trace_err("\nnot 24 bit bmp file\n");
        return 2;
    }

    return 0;
}

int bmp2bin_convert(char *path)
{
    char *filename = path;
    int ret = 0;
    fpr = fopen(filename, "rb");
    if (fpr == NULL)
    {
        printf("open file %s error \n", filename);
        return -1;
    }
    ret = bmp_read_head(fpr);
    if (ret)
    {
        return -2;
    }
    fclose(fpr);

    luminance *= 3;
    aim_fmt = &format_preset[0];
    out_data_size = BIH.biWidth * ((BIH.biHeight + 7) >> 3);
    out_data = (unsigned char *)malloc(out_data_size);
    bmp_data = (unsigned char *)malloc(BIH.biSizeImage);
    rgb888_data = (unsigned char *)malloc(BIH.biWidth * BIH.biHeight * 3);

    // if((fpwb = fopen(".\\img.bin", "wb")) == NULL)
    // {
    //     printf("write file \".\\img.bin\" error \n");
    //     return -3;
    // }

    convert(filename);

    // fclose(fpwb);
    free(bmp_data);
    bmp_data = NULL;
    free(rgb888_data);
    rgb888_data = NULL;
    // free(out_data);
    // out_data = NULL;

    return 0;
}

#endif

char *index_changefile_bmp2bin(char *path, char *pw, char *ph)
{
    int fdbin;
    struct stat buf;
    uint8_t *poutfile = NULL;
    uint8_t *pinfile = NULL;
    unsigned long long outlen;
    int ret;
    if (NULL == path)
    {
        trace_err("file path err!");
        return NULL;
    }

    if (bmp2bin_convert(path) < 0)
    {
        trace_err("bmp2bin parse failed!\n");
        return NULL;
    }

    pinfile = out_data;
    outlen = out_data_size << 2 + sizeof(BMP_FILE_DESC);
    poutfile = (unsigned char *)malloc(outlen);

    if (NULL == pinfile)
    {
        trace_err("malloc size = %ld  fail!\n", out_data_size);
        return NULL;
    }
    if (NULL == poutfile)
    {
        my_free(pinfile);
        trace_err("malloc size = %ld  fail!\n", out_data_size);
        return NULL;
    }
    memset(poutfile, 0x00, outlen);
    memcpy(poutfile + sizeof(BMP_FILE_DESC), pinfile, out_data_size);

    BMP_FILE_DESC *pbmp_info = (BMP_FILE_DESC *)poutfile;
    if (pw != NULL && ph != NULL)
    {
        pbmp_info->width = strtoul(pw, 0, 0);
        pbmp_info->heigth = strtoul(ph, 0, 0);
    }
    if (pbmp_info->width != BIH.biWidth || pbmp_info->heigth != BIH.biHeight)
    {
        pbmp_info->width = BIH.biWidth;
        pbmp_info->heigth = BIH.biHeight;
    }
    pbmp_info->datalen = out_data_size;
    trace_info("w=%d\th=%d\tlen=%d\n", pbmp_info->width, pbmp_info->heigth, pbmp_info->datalen);

    trace_err_buf("change1:", poutfile, sizeof(BMP_FILE_DESC));

    // 解析传入的文件路径 在相同路径下建立 新的名称文件
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];

    uint8_t newpath[1024];

    _splitpath(path, drive, dir, fname, ext);
    trace_info("parse %s -> %s\t%s\t%s\t%s\n", path, drive, dir, fname, ext);

    memset(newpath, 0x00, sizeof(newpath));

    sprintf(newpath, "%s%s%s.bin", drive, dir, fname);
    memset(c_bmp2binpath, 0x00, sizeof(c_bmp2binpath));
    memcpy(c_bmp2binpath, newpath, strlen(newpath));
    trace_info("newpath=%s\n", newpath);

    // 整合BMP头部信息  并计算CRC
    if (strlen(fname) > sizeof(pbmp_info->bmp_desc))
    {
        memcpy(pbmp_info->bmp_desc, fname, 30);
    }
    else
    {
        memcpy(pbmp_info->bmp_desc, fname, strlen(fname));
    }
    pbmp_info->bmp_desc[31] = 0x00;
    pbmp_info->bmp_index = BMP_COMPANY_LOGO_INDEX;

    pbmp_info->crc32 = Calc_crc32((const char *)pbmp_info, sizeof(BMP_FILE_DESC) - 4);
    trace_err_buf("change2:", poutfile, sizeof(BMP_FILE_DESC));

    buf2file(newpath, poutfile, (out_data_size) + sizeof(BMP_FILE_DESC));

    // my_free(pinfile);
    free(out_data);
    out_data = NULL;
    my_free(poutfile);
    return c_bmp2binpath;
}
#endif

void usage(const char *app)
{
    printf("\nUsage:\tttyUSB0-9\tttyAMA0-9\tttyS0-9\n", app);
    printf("\nUsage:\t%s -h\n", app);
    printf("\tprintf help informations\n");
    printf("\n");
    printf("\nUsage:\t%s -v\n", app);
    printf("\tprintf version informations\n");
    printf("\n");
    printf("\nUsage:\t%s -l<v><r> <bus>\n", app);
    printf("\tprintf current mcu file informations\n");
    printf("\n");
    printf("\nUsage:\t%s -n <bus> <newfile>\n", app);
    printf("\tIt will use <newfile> to update mcu app.\n");
    printf("\n");
    printf("\nUsage:\t%s -f <bus> <newfile>\n", app);
    printf("\tIt will use <newfile> to update mcu fontlib.\n");
    printf("\n");
    printf("\nUsage:\t%s -r <bus> <newfile> <num_id>\n", app);
    printf("\tIt will use <newfile> to update mcu bmp resource, num_id=[0-%d].\n", BMP_NUM_MAX_);
    printf("\n");
    printf("\nUsage:\t%s -n <bus> <newfile> -boot <passwd>\n", app);
    printf("\tIt will use <newfile> to update mcu boot.\n");
    printf("\tThe <passwd> contaxt to XSPEED,please.\n");
    printf("\n");
    printf("\nUsage:\t%s -g <bus> <newfile> <passwd>\n", app);
    printf("\tIt will use <newfile> to update mcu logo.\n");
    printf("\n");
    printf("\nUsage:\t%s -c <bmp.txt> bmp_width bmp_heigth\n", app);
    printf("\tIt will convert <bmp.txt> to <bmp-resource> packet file(at the same path).\n");
    printf("\n");
    printf("\nUsage:\t%s -b <bus> <xx.bmp> <num_id> [bmp_width] [bmp_heigth]\n", app);
    printf("\tIt will convert <xx.bmp> to <bmp-resource> packet file(at the same path) and download to mcu.\n");
    printf("\n");
}

void app_version_prt(char **argv)
{
    trace_info("%s version:%s\n", argv[0], "3.0.4");
    trace_info("build date:%s %s\n", __DATE__, __TIME__);
}

int dl_filedl_process(uint8_t opcode, uint8_t *path, uint8_t num_id, HANDLE comHandle)
{
    int ret = 0;
    ST_DL_INFO_HOST *pdlinfo = NULL;
    uint32_t crc32 = 0;

    /* parse input file  */
    bin_info_t bininfo;
    uint32_t poslen;
    uint8_t *p = NULL, *pdl = NULL;
    uint32_t validlen;
    int fdbin;
    struct stat buf;
    uint8_t resfilebuf[RES_FILE_CRC_MAXSIZE];
    uint32_t forcelen;

    if (NULL == path)
    {
        trace_err("file path err!");
        return -1;
    }
    // 打开文件
    fdbin = open(path, O_RDONLY);
    if (fdbin < 0)
    {
        trace_err("%s file open fail,res = %d.\r\n", path, fdbin);
        return -1;
    }

    fstat(fdbin, &buf);
    trace_info("%s file size:%ld\n ", path, buf.st_size);

    pbinfile = (unsigned char *)malloc(buf.st_size);

    if (NULL == pbinfile)
    {
        trace_err("malloc size = %ld  fail!\n", buf.st_size);
        return -1;
    }
    memset(pbinfile, 0x00, buf.st_size);

    // 读取内容
    ret = read(fdbin, pbinfile, buf.st_size);
    if (ret < 0)
    {
        trace_err("read bin fail,res = %d.\r\n", ret);
        my_free(pbinfile);
        return -1;
    }

    // 关闭文件
    close(fdbin);

    ///  文件解析
    p = pbinfile;

    if (FILE_TYPE_BOOT == opcode || FILE_TYPE_APP == opcode) // boot app
    {
#if 1
        memset((uint8_t *)&bininfo, 0x00, sizeof(bininfo));

        memcpy((uint8_t *)&poslen, &p[BIN_INFO_OFFSET], sizeof(poslen));

        trace_info("Now get file poslen = %08X \n", poslen);

        switch (opcode)
        {
        case FILE_TYPE_APP:
            // case IS_OPRCODE_UPDATEAPP:
            // case IS_OPRCODE_UPDATEAPP_FORCE:
            {
                forcelen = LEN_APP;
                if (poslen > SA_APP && poslen < (SA_APP + LEN_APP)) // just app
                {
                    poslen -= SA_APP;
                    validlen = buf.st_size;
                    pdl = pbinfile;
                    memcpy((uint8_t *)&bininfo, &p[poslen], sizeof(bininfo));
                }
                else if (buf.st_size > LEN_BOOT) // have boot+app
                {
                    memcpy((uint8_t *)&poslen, &p[LEN_BOOT + BIN_INFO_OFFSET], sizeof(poslen));
                    trace_info("The new get file poslen = %08X \n", poslen);
                    if (poslen > SA_APP && poslen < (SA_APP + LEN_APP)) // find app
                    {
                        poslen -= SA_APP;
                        validlen = buf.st_size - LEN_BOOT;
                        pdl = &pbinfile[LEN_BOOT];
                        memcpy((uint8_t *)&bininfo, &p[LEN_BOOT + poslen], sizeof(bininfo));
                    }
                }
            }
            break;
        case FILE_TYPE_BOOT:
            // case IS_OPRCODE_UPDATEBOOT:
            // case IS_OPRCODE_UPDATEBOOT_FORCE:
            {
                forcelen = LEN_BOOT;
                if (poslen > SA_BOOT && poslen < (SA_BOOT + LEN_BOOT)) // have find boot
                {
                    poslen -= SA_BOOT;
                    validlen = (buf.st_size > LEN_BOOT) ? (LEN_BOOT) : buf.st_size;
                    pdl = pbinfile;
                    memcpy((uint8_t *)&bininfo, &p[poslen], sizeof(bininfo));

                    if (bininfo.filesize != 0 && bininfo.filesize < LEN_BOOT)
                    {
                        validlen = bininfo.filesize;
                    }
                }
            }
            break;
        default:
            trace_err("input file invalid!\n");
            return -1;
        }

        if (NULL == pdl)
        {
            trace_err("\tERROR:Input file <%s> check failed!\n", path);
            my_free(pbinfile);
            return -1;
        }

        trace_info("FILE bin info:%s %08X %s %s\n",
                   bininfo.Name, bininfo.Ver, bininfo.BuildTime, bininfo.Descript);
        trace_info("magicnum:%08X %08X, file info:%dB\n",
                   bininfo.MagicStart, bininfo.MagicEnd, bininfo.filesize);

        if ((bininfo.MagicStart != BIN_MAGICNUM_START) || (bininfo.MagicEnd != BIN_MAGICNUM_END))
        {
            trace_err("magic num error!\n");
            my_free(pbinfile);
            return -3;
        }

        if (validlen != bininfo.filesize)
        {
            trace_err("filesize not compatible %d %d \n", validlen, bininfo.filesize);
            my_free(pbinfile);
            return -4;
        }
#endif
    }
    else if (FILE_TYPE_FONT == opcode)
    { // fontlib can direct download
        validlen = buf.st_size;
        pdl = pbinfile;
    }
    else if (FILE_TYPE_LOGO == opcode)
    {
        // 记录当前图片信息
        BMP_FILE_DESC *pbmp_info = (BMP_FILE_DESC *)pbinfile;
        pbmp_info->bmp_index = BMP_COMPANY_LOGO_INDEX;
        pbmp_info->crc32 = Calc_crc32((const char *)pbmp_info, sizeof(BMP_FILE_DESC) - 4);

        validlen = buf.st_size;
        pdl = pbinfile;
        if (FILE_TYPE_LOGO == opcode)
        {
            trace_err_buf("output bmp-logo info:", pdl, buf.st_size);
            trace_err("\n");
        }
    }
    else if (FILE_TYPE_RESBMP == opcode)
    {
        // 记录当前图片信息
        BMP_FILE_DESC *pbmp_info = (BMP_FILE_DESC *)pbinfile;
        pbmp_info->bmp_index = num_id;
        pbmp_info->crc32 = Calc_crc32((const char *)pbmp_info, sizeof(BMP_FILE_DESC) - 4);
        validlen = buf.st_size;
        pdl = pbinfile;
    }
    else
    {
        trace_err("input file or correspond parameter unsupport!\n");
        return -1;
    }

    ///  文件解析 end

    pdlinfo = &gdlinfo;

    /* 记录需要发送的信息 */
    memset((uint8_t *)&gdlinfo, 0x00, sizeof(gdlinfo));
    pdlinfo->datalen = validlen; // buf.st_size;
    pdlinfo->pdata = pdl;        // pbinfile;
    pdlinfo->crcmode = DIGEST_MODE_CRC32;
    pdlinfo->filetype = opcode;

    pdlinfo->rfu[0] = num_id; // recoed bmp num
    /* add crc32 */
    // if (pdlinfo->filetype == FILE_TYPE_BOOT || pdlinfo->filetype == FILE_TYPE_APP)
    // {
    //     crc32 = Calc_crc32(pdlinfo->pdata, gdlinfo.datalen);
    // }
    // else
    {
        memcpy(resfilebuf, pdlinfo->pdata, RES_FILE_CRC_MAXSIZE_HALF); // 头尾各取 RES_FILE_CRC_MAXSIZE_HALF 字节作为校验
        memcpy(&resfilebuf[RES_FILE_CRC_MAXSIZE_HALF], pdlinfo->pdata + pdlinfo->datalen - RES_FILE_CRC_MAXSIZE_HALF,
               RES_FILE_CRC_MAXSIZE_HALF);
        crc32 = Calc_crc32(resfilebuf, RES_FILE_CRC_MAXSIZE);
        trace_err_buf("digest info:", resfilebuf, RES_FILE_CRC_MAXSIZE);
        trace_err("result code:%x\n", crc32);
    }
    memcpy(gdlinfo.appdigest, (unsigned char *)&crc32, sizeof(crc32));

    trace_info("\tbegin to request download...\n");

    // 发送文件下载请求
    if ((ret = dl_fileinfo_sendrequest(0, comHandle, pdlinfo)))
    {
        trace_err("The device request download failed %d!\n", ret);
        return RET_EXEC_ERROR;
    }
    trace_info("\tbegin to download data...\n");
    // 发送文件
    if ((ret = dl_fileinfo_senddata(0, comHandle, pdlinfo)))
    {
        trace_err("The device download file failed %d!\n", ret);
        return RET_EXEC_ERROR;
    }
    trace_info("\tData send complete!\n\tbegin to save file...\n");
    // 保存文件
    if ((ret = dl_fileinfo_savefile(0, comHandle, pdlinfo)))
    {
        trace_err("The device save file failed %d!\n", ret);
        return RET_EXEC_ERROR;
    }
    trace_info("\tfile saved.\n");
    // 结束下载
    if ((ret = dl_fileinfo_finishdl(0, comHandle, pdlinfo)))
    {
        trace_err("The device finish download failed %d!\n", ret);
        return RET_EXEC_ERROR;
    }

    trace_err("\tfile <%s> download suc.\nExit download!\n", path);

    return ret;
}

static int app_dlboot_verify(char *opcode, char *passwd)
{
    if (strcmp(opcode, "-boot"))
    {
        return -1;
    }
    if (strcmp(passwd, "xspeed123"))
    {
        return -2;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    BOOL b = FALSE;
    DWORD wRLen = 0;
    DWORD wWLen = 0;
    char buf[BUF_SIZE] = {0};
    HANDLE comHandle = INVALID_HANDLE_VALUE; // 串口句柄
    uint8_t retry = 1;

    // 新增的
    busid = 1;
    uint8_t isoprcode = 0;    // 记录操作  getinfo? update?
    uint8_t isupdatetype = 0; // 升级类型 boot? app?
    char *newfilepath = NULL;
    uint8_t listmode = 0;
    uint8_t num_id = 0;
    uint8_t isfiletype = 0xff;

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

    if (argc < 2 || argc > 7)
    {
        usage(argv[0]);
        return -1;
    }
    trace_line();

    switch (argv[1][1])
    {
#if CFG_CHANGE_INDEXBMP
    case 'c': // printf("\nUsage:\t%s -c <newfile> bmp_width bmp_heigth\n", app);
    {
        if (argc < 5)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        newfilepath = argv[2];
        index_changefile_str2asc(newfilepath, argv[3], argv[4]);
    }
        return 0;
#endif
#if CFG_BMP2BIN_DL
    case 'b':
    {
        if (argc < 5)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_UPDATERES;
        trace_line();
        newfilepath = argv[3];
        trace_line();
        if (argv[1][2] == 'f')
        {
            isoprcode += 1;
        }
        num_id = strtoul(argv[4], 0, 0);
        num_id = num_id % BMP_NUM_MAX_;
        isfiletype = FILE_TYPE_RESBMP;
        // 导入bmp文件，weigh height 生成bin文件(默认0xfe)，输出新资源bin文件路径
        newfilepath = index_changefile_bmp2bin(argv[3], argv[5], argv[6]);
        if (newfilepath == NULL)
        {
            trace_err("bmp convert to binary file failed!\n");
            return -2;
        }
        // 转化成功，则按照 resource文件进行下载处理
    }
    break;
#endif
    case 'l':
    {
        if (argc < 2)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_LISTINFO;
        if (argv[1][2] == 'r')
        {
            // gdlinfo.filetype = IS_OPRCODE_UPDATEAPP_FORCE;
            isoprcode = IS_OPRCODE_LISTRES;
            listmode = 1;
        }
    }
    break;
    case 'n':
    {
        if (argc < 4)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_UPDATEAPP;
        isfiletype = FILE_TYPE_APP;
        trace_line();
        newfilepath = argv[3];
        trace_line();
        if (argv[1][2] == 'f')
        {
            isoprcode += 1;
        }
        if (argc == 4)
        {
            break;
        }
        trace_line();
        if (argc < 6)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        if (app_dlboot_verify(argv[4], argv[5]))
        {
            return -5;
        }
        isoprcode = IS_OPRCODE_UPDATEBOOT;
        isupdatetype = IS_OPRCODE_UPDATEBOOT;
        isfiletype = FILE_TYPE_BOOT;
    }
    break;
    case 'f':
    {
        if (argc < 4)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_UPDATEFONTLIB;
        trace_line();
        newfilepath = argv[3];
        trace_line();
        if (argv[1][2] == 'f')
        {
            isoprcode += 1;
        }
        isfiletype = FILE_TYPE_FONT;
    }
    break;
    case 'r':
    {
        if (argc < 5)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_UPDATERES;
        trace_line();
        newfilepath = argv[3];
        trace_line();
        if (argv[1][2] == 'f')
        {
            isoprcode += 1;
        }
        num_id = strtoul(argv[4], 0, 0);
        num_id = num_id % BMP_NUM_MAX_;
        isfiletype = FILE_TYPE_RESBMP;
    }
    break;
    case 'g':
    {
        if (argc < 5)
        {
            trace_err("\tinput invalid param.\n");
            return -1;
        }
        trace_line();
        busid = strtoul(argv[2], 0, 0);
        isoprcode = IS_OPRCODE_UPDATELOGO;
        trace_line();
        newfilepath = argv[3];
        trace_line();
        if (argv[1][2] == 'f')
        {
            isoprcode += 1;
        }
        if (strcmp(argv[4], "xspeed123"))
        {
            // trace_err("passwd err!\n");
            // return -2;
        }
        isfiletype = FILE_TYPE_LOGO;
    }
    break;
    case 'v':
        app_version_prt(argv);
        return 0;
    case 'h':
    default:
        app_version_prt(argv);
        usage(argv[0]);
        return 0;
    }

    // 打开串口
    if (busid >= UART_PORT_MAX || busid < 0)
    {
        trace_err("input busid error,please input valid id,is [0-2]\n");
        return RET_INVALID_PARAM;
    }
    serial_fifoclear(0);
    const char *com = CFG_DEBUG_COMID;

#if 1 // 强制切换波特率115200
    uint8_t force_bdr[] = {0xAA, 0xF0, 0x01};
    uint8_t force_rcv[1024];
    uint8_t force_flag = 0;
    int fori;
    comHandle = serial_open(com, CBR_9600, 8, NOPARITY, ONESTOPBIT);
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("OpenSerial com%d fail!\r\n", busid);
        return -1;
    }
    memset(force_rcv, 0x00, sizeof(force_rcv));
    for (fori = 0; fori < 3; fori++)
    {
        serial_send(comHandle, force_bdr, sizeof(force_bdr));
        sys_delay_ms(30);
    }

    if (serial_recv(comHandle, force_rcv, sizeof(force_bdr)) > APP_CHANGEBDR_COMPLET_LEN)
    {
        if (strstr(force_rcv, APP_CHANGEBDR_COMPLET))
        {
            force_flag = 1;
        }
    }

    // 关闭串口
    b = serial_close(comHandle);
    if (!b)
    {
        LOGE("CloseHandle fail\r\n");
    }
    serial_checkbuf(comHandle, (BUF_SIZE << 2));
    serial_fifoclear(0);
#if 0
    if (!force_flag)
    {
        comHandle = serial_open(com, CBR_115200, 8, NOPARITY, ONESTOPBIT);
        if (INVALID_HANDLE_VALUE == comHandle) {
            LOGE("OpenSerial com%d fail!\r\n", busid);
            return -1;
        }

        //关闭串口
        b = serial_close(comHandle);
        if (!b) {
            LOGE("CloseHandle fail\r\n");
        }
    }
#endif
    serial_checkbuf(comHandle, (BUF_SIZE << 2));
    serial_fifoclear(0);
#endif

    comHandle = serial_open(com, CBR_115200, 8, NOPARITY, ONESTOPBIT);
    if (INVALID_HANDLE_VALUE == comHandle)
    {
        LOGE("OpenSerial com%d fail!\r\n", busid);
        return -1;
    }
    LOGD("Open com%d Successfully!\r\n", busid);
    unsigned char entrydl = DL_HANDSHAKE_CMD1;
    serial_send(comHandle, &entrydl, 1);
    entrydl = DL_HANDSHAKE_CMD2;
    serial_send(comHandle, &entrydl, 1);
    sys_delay_ms(100);

#if 0
    //循环接收消息，收到消息后将消息内容打印并回复I_RECEIVE, 如果收到EXIT_STR就回复EXIT_STR并退出循环
	while (1) {
		wRLen = 0;
        //读串口消息
		b = ReadFile(comHandle, buf, sizeof(buf)-1, &wRLen, NULL);
		if (b && 0 < wRLen) {//读成功并且数据大小大于0
            buf[wRLen] = '\0';
            LOGD("[RECV]%s\r\n", buf);//打印收到的数据
            if (0 == strncmp(buf, EXIT_STR, strlen(EXIT_STR))) {
                //回复
                b = WriteFile(comHandle, TEXT(I_EXIT), strlen(I_EXIT), &wWLen, NULL);
                if (!b) {
                    LOGE("WriteFile fail\r\n");
                }
                break;
            }
            //回复
			b = WriteFile(comHandle, TEXT(I_RECEIVE), strlen(I_RECEIVE), &wWLen, NULL);
            if (!b) {
                LOGE("WriteFile fail\r\n");
            }
		}
	}
#else
    // 握手
    unsigned long starttick = 0;
    int ret;
    starttick = sys_getcounter();
    retry = 1;
    while (retry--)
    {
        if (sys_getcounter() - starttick > (DL_TIME_WAIT * 10))
        {
            trace_err("%s:handshake outtime\n");
            ret = RET_TIMEOUT_ERR;
            goto exit_finish;
        }
        if (RET_SUCCESS == dl_handshake(0, comHandle))
        {
            trace_line();
            break;
        }
        sys_delay_ms(10);
    }
    trace_line();
    serial_fifoclear(0);
    trace_line();
    serial_checkbuf(comHandle, (BUF_SIZE << 2));
    trace_line();
    serial_fifoclear(0);
    trace_line();

    // 操作  获取设备终端信息
    if (IS_OPRCODE_LISTINFO == isoprcode || IS_OPRCODE_LISTRES == isoprcode)
    {
        ret = dl_terminalinfo_get(listmode, comHandle);
        // 结束下载
        if ((ret = dl_fileinfo_finishdl(0, comHandle, NULL)))
        {
            trace_err("The device exit download failed %d!\n", ret);
            ret = RET_EXEC_ERROR;
        }

        goto exit_finish;
    }

    // 操作 下载文件
    starttick = sys_getcounter();
    ret = dl_filedl_process(isfiletype, newfilepath, num_id, comHandle);
    if (pbinfile)
    {
        my_free(pbinfile);
    }
    trace_info("download file use time: %ld\n", (sys_getcounter() - starttick));

#endif
exit_finish:

#if 1 // 强制切换波特率115200

    force_bdr[0] = 0xAA;
    force_bdr[1] = 0xF0;
    force_bdr[2] = 0x00;
    memset(force_rcv, 0x00, sizeof(force_rcv));
    for (fori = 0; fori < 3; fori++)
    {
        serial_send(comHandle, force_bdr, sizeof(force_bdr));
        sys_delay_ms(30);
    }

    if (serial_recv(comHandle, force_rcv, sizeof(force_bdr)) > APP_CHANGEBDR_COMPLET_LEN)
    {
        if (strstr(force_rcv, APP_CHANGEBDR_COMPLET))
        {
            force_flag = 1;
        }
    }

    serial_checkbuf(comHandle, (BUF_SIZE << 2));
    serial_fifoclear(0);

    serial_checkbuf(comHandle, (BUF_SIZE << 2));
    serial_fifoclear(0);
#endif

    // 关闭串口
    b = serial_close(comHandle);
    if (!b)
    {
        LOGE("CloseHandle fail\r\n");
    }

    LOGD("Program Exit.\r\n");
    return ret;
}
