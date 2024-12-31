#ifndef __LCM_API_DEMO_H_
#define __LCM_API_DEMO_H_

#ifdef _MCU_UPDATETOOL_CPUARCH_
#include "drv_uart.h"
#endif

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

#define trace printf
#define trace_err printf
#define trace_info printf

// 返回值状态码定义
#define RET_UPDATE_PARAMETER_2 2
#define RET_UPDATE_PARAMETER_1 1
#define RET_SUCCESS 0
#define RET_INVALID_PARAM -1
#define RET_NOT_READY -2
#define RET_EXEC_ERROR -3
#define RET_BUF_TOO_SMALL -4
#define RET_LEN_ERROR -5
#define RET_BUSY_STATE -6
#define RET_TIMEOUT_ERR -7
#define RET_VERIFY_CRCERR -8
#define RET_NOT_FOUND -9

enum
{
    E_DISP_PRODUCTINFO = 0x00, // 显示产品信息 aa 00 : 显示产品信息
    E_DISP_RESET = 0x01,       // 复位 aa 01 : 复位
    E_DISP_CLS = 0x10,         // 清屏 aa 10 : 清屏
    E_DISP_OPENLCD = 0x11,     // 开显示 aa 11 : 开显示
    E_DISP_CLOSELCD = 0x12,    // 关显示 aa 12 ：关显示
    E_DISP_LIGHTNESS = 0x13,   // 亮度 aa 13 n : 亮度
    E_DISP_LATTICE = 0x14,     // 对比度 aa 14 n : 对比度
    E_DISP_SETCURSOR = 0x20,   // 设置光标位置 aa 20 x y : 设置光标
    E_DISP_INVERT = 0x22,      // 反显 aa 22 1/0 : 开关反显
    E_DISP_ZFJJ = 0x21,        // 字符间距 aa 21 n : 字符间距
    E_DISP_STRING = 0x23,      // 字符串显示 aa 20 x y aa 23 str 00: 指定位置显示字符串
    E_DISP_AUTOLINE = 0x24,    // 字符串自动换行
    E_DISP_ROLLTIME = 0x25,    // 滚动时间
    E_DISP_CLEANTIME = 0x26,   // 滚动时间
    E_DISP_STRING_2 = 0x27,
    E_DISP_BMPNUM = 0x30,   // 根据 序号 显示BMP aa 30 n>>8 n : 显示指定序号图片 全图显示
    E_DISP_BLOCKPIC = 0x31, // 以块方式显示图片 aa 31 n>>8 n x y px py w h : 块方式显示图片

    E_SPECFUNC_EXITTHREAD = 0x99,  // aa 99 退出线程
    E_SPECFUNC_CHANGEBDR = 0xF0,   // aa f0 0/1 修改波特率为 9600/115200
    E_SPECFUNC_ENTRYDLMODE = 0xFE, // aa fe 设备进入下载模式
};

// 获得项数
#ifndef BUFFER_DIM
#define BUFFER_DIM(x) (sizeof(x) / sizeof(x[0]))
#endif

#endif // __LCM_API_DEMO_H_
