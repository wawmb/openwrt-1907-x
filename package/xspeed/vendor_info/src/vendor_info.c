/*
 * Copyright 2017 Rockchip Electronics S.LSI Co. LTD
 * Author: Addy Ke <addy.ke@iotwrt.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version
 */

#include <asm/types.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <linux/stddef.h>
#include <errno.h>
#include <time.h>

// #include <rockchip/rockchip_vendor.h>

#define VENDOR_DEV "/dev/vendor_storage"

#define VENDOR_REQ_TAG 0x56524551
#define VENDOR_READ_IO _IOW('v', 0x01, unsigned int)
#define VENDOR_WRITE_IO _IOW('v', 0x02, unsigned int)

#define MAX_WRITE_LEN 1024

#define VENDOR_SN_ID 1
#define VENDOR_WIFI_MAC_ID 2
#define VENDOR_LAN_MAC_ID 3
#define VENDOR_BLUETOOTH_ID 4

#define VENDOR_MANUFACTURE_ID VENDOR_SN_ID
#define VENDOR_BOARD_ID 11
#define VENDOR_PRODUCT_ID 12
#define VENDOR_TEMPORARY_SN_ID 13
#define VENDOR_RESERVED_ID 330
#define VENDOR_ACTCODE_ID 0xa0
struct rockchip_vendor_req
{
    unsigned int tag;
    unsigned short id;
    unsigned short len;
    unsigned char data[MAX_WRITE_LEN];
};

static int getVendorData(unsigned char *data, unsigned short id, unsigned short data_len)
{
    int ret = 0;
    int fd;
    struct rockchip_vendor_req req;

    fd = open(VENDOR_DEV, O_RDWR, 0);
    if (fd < 0)
    {
        fprintf(stderr, "Vendor storage device open failed\n");
        return fd;
    }

    req.tag = VENDOR_REQ_TAG;
    req.id = id;
    req.len = data_len;

    ret = ioctl(fd, VENDOR_READ_IO, &req);
    if (ret < 0)
    {
        fprintf(stderr, "Vendor read failed\n");
        close(fd);
        return ret;
    }

    memcpy(data, req.data, data_len);

    close(fd);
    return ret;
}

static int setVendorData(unsigned char *data, unsigned short id, unsigned short data_len)
{
    int ret = 0;
    int fd;
    struct rockchip_vendor_req req;

    if (data_len > MAX_WRITE_LEN)
    {
        fprintf(stderr, "The maximum length of vendor data should be less than %d\n",
                MAX_WRITE_LEN);
        return -EINVAL;
    }

    fd = open(VENDOR_DEV, O_RDWR, 0);
    if (fd < 0)
    {
        fprintf(stderr, "Vendor storage device open failed\n");
        return fd;
    }

    req.tag = VENDOR_REQ_TAG;
    req.id = id;
    req.len = data_len;
    memcpy(req.data, data, data_len);

    ret = ioctl(fd, VENDOR_WRITE_IO, &req);
    if (ret < 0)
        fprintf(stderr, "Vendor write failed\n");

    close(fd);
    return ret;
}

// npxx sn number
int getManufactureID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_MANUFACTURE_ID, data_len);
}

int setManufactureID(unsigned char *data, unsigned short data_len)
{
    return setVendorData(data, VENDOR_MANUFACTURE_ID, data_len);
}

int getProductID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_PRODUCT_ID, data_len);
}

int setProductID(unsigned char *data, unsigned short data_len)
{
    return setVendorData(data, VENDOR_PRODUCT_ID, data_len);
}

int getMacAddr(unsigned char *data, unsigned short id, unsigned short data_len)
{
    return getVendorData(data, id, data_len);
}

int setMacAddr(unsigned char *data, unsigned short id, unsigned short data_len)
{
    char hex[100] = {0};
    strcpy(hex, data);
    if (data_len % 2 != 0)
    {
        printf("Invalid hexadecimal string\n");
        return -1;
    }
    char ascii[data_len / 2 + 1];
    for (int i = 0; i < data_len; i += 2)
    {
        char byte[3] = {hex[i], hex[i + 1], '\0'};
        ascii[i / 2] = strtol(byte, NULL, 16);
    }
    ascii[data_len / 2] = '\0';
    memcpy(data, ascii, data_len / 2 + 1);
    data_len = data_len / 2;
    printf("Data string:(%s) Ascii string:(%02x)\n", hex, ascii);
    return setVendorData(data, id, data_len);
}

int getActCode(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_ACTCODE_ID, data_len);
}

int setActCode(unsigned char *data, unsigned short data_len)
{
    return setVendorData(data, VENDOR_ACTCODE_ID, data_len);
}

int getReservedID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_RESERVED_ID, data_len);
}

int setReservedID(unsigned char *data, unsigned short data_len)
{
    return setVendorData(data, VENDOR_RESERVED_ID, data_len);
}

int getTemporaryID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_TEMPORARY_SN_ID, data_len);
}

int setTemporaryID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_TEMPORARY_SN_ID, data_len);
}

int getBoardID(unsigned char *data, unsigned short data_len)
{
    return getVendorData(data, VENDOR_BOARD_ID, data_len);
}

int setBoardID(unsigned char *data, unsigned short data_len)
{
    return setVendorData(data, VENDOR_BOARD_ID, data_len);
}

static void usage(char **argv)
{
    printf(
        "Usage: %s get ID len\n\n"
        "ID:\n"
        "1) sn\n"
        "2) product\n"
        "3) mac\n"
        "4) temporary\n"
        "5) config\n"
        "\n",
        argv[0]);
}

static int getData(unsigned char *id, size_t len)
{
    int ret = 0;
    unsigned char buf[1024];

    memset(buf, 0, 1024);
    if (strcmp((const char *)id, "sn") == 0)
    {
        ret = getManufactureID(buf, len);
        if (ret == 0)
            printf("SN=%s\n", buf);
    }
    else if (strcmp((const char *)id, "product") == 0)
    {
        ret = getProductID(buf, len);
        if (ret == 0)
            printf("Product ID=%s\n", buf);
    }
    else if (strcmp((const char *)id, "mac") == 0)
    {
        ret = getMacAddr(buf, VENDOR_LAN_MAC_ID, len);
        if (ret == 0)
        {
            printf("len=%d\n", len);
            printf("Mac Address=%02x:%02x:%02x:%02x:%02x:%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
            if (len > 6)
            {
                printf("Mac1 Address=%02x:%02x:%02x:%02x:%02x:%02x\n", buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
            }
        }
    }
    else if (strcmp((const char *)id, "temporary") == 0)
    {
        ret = getTemporaryID(buf, len);
        if (ret == 0)
            printf("Temporary ID=%s\n", buf);
    }
    else if (strcmp((const char *)id, "config") == 0)
    {
        ret = getBoardID(buf, len);
        if (ret == 0)
            printf("board_config=%s\n", buf);
    }
    else if (strcmp((const char *)id, "reserved") == 0)
    {
        ret = getReservedID(buf, len);
        if (ret == 0)
            printf("%s\n", buf);
    }
    else
        ret = -EINVAL;
    return ret;
}

static int setData(unsigned char *id, char *buf)
{
    int ret = 0;
    unsigned short len = (unsigned short)strlen(buf);
    if (strcmp((const char *)id, "sn") == 0)
    {
        ret = setManufactureID(buf, len);
        if (ret == 0)
            printf("SN=%s\n", buf);
    }
    else if (strcmp((const char *)id, "product") == 0)
    {
        ret = setProductID(buf, len);
        if (ret == 0)
            printf("Product ID=:%s\n", buf);
    }
    else if (strcmp((const char *)id, "mac") == 0)
    {
        ret = setMacAddr(buf, VENDOR_LAN_MAC_ID, len);
        if (ret == 0)
        {
            printf("len=%d\n", len);
            printf("Mac Address=%02x:%02x:%02x:%02x:%02x:%02x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
            if (len > 12)
            {
                printf("Mac1 Address=%02x:%02x:%02x:%02x:%02x:%02x\n", buf[6], buf[7], buf[8], buf[9], buf[10], buf[11]);
            }
        }
    }
    else if (strcmp((const char *)id, "temporary") == 0)
    {
        ret = setTemporaryID(buf, len);
        if (ret == 0)
            printf("Temporary ID=%s\n", buf);
    }
    else if (strcmp((const char *)id, "config") == 0)
    {
        ret = setBoardID(buf, len);
        if (ret == 0)
            printf("board_config=%s\n", buf);
    }
    else if (strcmp((const char *)id, "reserved") == 0)
    {
        ret = setReservedID(buf, len);
        if (ret == 0)
            printf("%s\n", buf);
    }
    else
        ret = -EINVAL;
    return ret;
}

int main(int argc, char **argv)
{
    int ret = 0;
    if (argc < 4)
    {
        usage(argv);
        return -EINVAL;
    }

    if (strcmp(argv[1], "get") == 0)
    {
        ret = getData((unsigned char *)argv[2], atoi(argv[3]));
        return ret;
    }
    else if (strcmp(argv[1], "set") == 0)
    {
        ret = setData((unsigned char *)argv[2], argv[3]);
        return ret;
    }
    else
    {
        usage(argv);
        return -EINVAL;
    }
}
