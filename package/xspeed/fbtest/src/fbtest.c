#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

int log_level = 3;
char *logfile = NULL;

struct fb_dev
{
    int fb;       // for frame buffer
    void *fb_mem; // frame buffer mmap
    unsigned int fb_width, fb_height, fb_line_len, fb_size, fb_bpp, xoffset, yoffset;
};
static struct fb_dev fbdev;

static void draw(uint16_t color)
{
    int i, j;
    if (fbdev.fb_bpp == 16)
    {
        uint16_t *p16 = (uint16_t *)fbdev.fb_mem;
        for (i = 0; i < fbdev.fb_height; i++, p16 += fbdev.fb_line_len / 2)
        {
            for (j = 0; j < fbdev.fb_width; j++)
                p16[j] = color;
        }
    }
    else if (fbdev.fb_bpp == 32)
    {
        uint32_t *p32 = (uint32_t *)fbdev.fb_mem;
        for (i = 0; i < fbdev.fb_height; i++, p32 += fbdev.fb_line_len / 4)
        {
            for (j = 0; j < fbdev.fb_width; j++)
                p32[j] = color;
        }
    }
}

static void draw_bits(unsigned char red, unsigned char green, unsigned char blue, int Gradient)
{
    int x = 0, y = 0;
    long location = 0;

    for (y = 0; y < fbdev.fb_height; y++)
    {
        for (x = 0; x < fbdev.fb_width; x++)
        {
            location = (x + fbdev.xoffset) * (fbdev.fb_bpp / 8) +
                       (y + fbdev.yoffset) * fbdev.fb_width * (fbdev.fb_bpp / 8);
            if (fbdev.fb_bpp == 32)
            {
                if (Gradient)
                {
                    red = (x * 255) / fbdev.fb_width;
                    green = (y * 255) / fbdev.fb_height;
                    blue = 255 - ((x * 255) / fbdev.fb_width);
                }
                *((char *)fbdev.fb_mem + location) = blue;      // 蓝色通道
                *((char *)fbdev.fb_mem + location + 1) = green; // 绿色通道
                *((char *)fbdev.fb_mem + location + 2) = red;   // 红色通道
                *((char *)fbdev.fb_mem + location + 3) = 0;     // 透明度通道
            }
            else
            {
                // 其他情况后续添加
            }
        }
    }
}

static void usage()
{
    printf(
        "usage: fb-test [option] \n"
        "   -h: this message\n"
        "   -m: display mode.\n"
        "       default 0.\n"
        "       0: auto display rgb.\n"
        "       1: control increase range one(0x00000000-->0x00000001).\n"
        "       2: control increase range double(0x00000000-->0x00000002).\n"
        "   -d: switch play on fb1 or fb0.\n"
        "       default fb0.\n"
        "   -D: Draw_mode.\n"
        "       default 0.\n"
        "       0: draw.\n"
        "       1: draw_bits.\n"
        "       2: draw_bits quickly.\n"
        "   -b: bits per pixel (16 or 32).\n"
        "       default 32.\n");
}

int framebuffer_open(const char *fbfilename)
{
    int fb;
    struct fb_var_screeninfo fb_vinfo;
    struct fb_fix_screeninfo fb_finfo;
    char realname[80];
    strcpy(realname, "/dev/");
    strcat(realname, fbfilename);

    fb = open(realname, O_RDWR);
    if (fb < 0)
    {
        printf("device %s open failed\n", realname);
        return -1;
    }

    if (ioctl(fb, FBIOGET_VSCREENINFO, &fb_vinfo))
    {
        printf("Can't get VSCREENINFO: %s\n", strerror(errno));
        close(fb);
        return -1;
    }

    if (ioctl(fb, FBIOGET_FSCREENINFO, &fb_finfo))
    {
        printf("Can't get FSCREENINFO: %s\n", strerror(errno));
        return -1;
    }

    fbdev.fb_bpp = fb_vinfo.bits_per_pixel;

    fbdev.fb_width = fb_vinfo.xres;
    fbdev.fb_height = fb_vinfo.yres;

    fbdev.xoffset = fb_vinfo.xoffset;
    fbdev.yoffset = fb_vinfo.yoffset;

    fbdev.fb_line_len = fb_finfo.line_length;
    fbdev.fb_size = fb_finfo.smem_len;

    printf("frame buffer: %d(%d)x%d,  %dbpp, 0x%xbyte\n",
           fbdev.fb_width, fbdev.fb_line_len, fbdev.fb_height, fbdev.fb_bpp, fbdev.fb_size);

    if (fbdev.fb_bpp != 16 && fbdev.fb_bpp != 32)
    {
        printf("frame buffer must be 16bpp or 32bpp mode\n");
        return -1;
    }

    fbdev.fb_mem = mmap(NULL, fbdev.fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fbdev.fb_mem == NULL || (int)fbdev.fb_mem == -1)
    {
        fbdev.fb_mem = NULL;
        printf("mmap failed\n");
        close(fb);
        return -1;
    }

    fbdev.fb = fb;
    memset(fbdev.fb_mem, 0x0, fbdev.fb_size);

    return 0;
}

void framebuffer_close()
{
    if (fbdev.fb_mem)
    {
        munmap(fbdev.fb_mem, fbdev.fb_size);
        fbdev.fb_mem = NULL;
    }

    if (fbdev.fb)
    {
        close(fbdev.fb);
        fbdev.fb = 0;
    }
}

int main(int argc, char *argv[])
{
    int i, c;
    char fbfilename[16] = "fb0";
    int display_mode = 0;
    int Draw_mode = 0;
    int bpp = 32; // 默认使用32位色深度

    while ((c = getopt(argc, argv, "d:m:b:D:")) != -1)
    {
        switch (c)
        {
        case 'd':
            strcpy(fbfilename, optarg);
            break;
        case 'm':
            display_mode = atoi(optarg);
            break;
        case 'b':
            bpp = atoi(optarg);
            break;
        case 'D':
            Draw_mode = atoi(optarg);
            break;
        case '?':
        case 'h':
            usage();
            return 0;
        }
        if (display_mode < 0 || display_mode > 2 || (bpp != 16 && bpp != 32))
        {
            usage();
            return 0;
        }
    }

    if (framebuffer_open(fbfilename))
    {
        printf("Failed to open framebuffer: %s\n", fbfilename);
        return -1;
    }

    if (Draw_mode == 1)
    {
        for (size_t i = 0; i <= 255; i++)
        {
            draw_bits(i, 0, 0, 0);
            usleep(25000);
        }
        for (size_t i = 0; i <= 255; i++)
        {
            draw_bits(0, i, 0, 0);
            usleep(25000);
        }
        for (size_t i = 0; i <= 255; i++)
        {
            draw_bits(0, 0, i, 0);
            usleep(25000);
        }
        draw_bits(0, 0, 0, 1);
        goto out;
    }
    else if (Draw_mode == 2)
    {
        for (size_t i = 0; i <= 5; i++)
        {
            draw_bits(255, 0, 0, 0);
            usleep(500000);
            draw_bits(0, 255, 0, 0);
            usleep(500000);
            draw_bits(0, 0, 255, 0);
            usleep(500000);
        }
        draw_bits(0, 0, 0, 1);
        goto out;
    }

    if (bpp == 16)
    {
        // blue (16位)
        for (i = 0; i <= 0x1F; i++)
        { // 16位颜色深度的最大值是0x1F
            printf("%d:color=0x%x\n", i, i);
            draw(i);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
        // green
        for (i = 0; i <= 0x3F; i++)
        { // 16位颜色深度的最大值是0x3F
            printf("%d:color=0x%x\n", i, i);
            draw(i << 5);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
        // red
        for (i = 0; i <= 0x1F; i++)
        { // 16位颜色深度的最大值是0x1F
            printf("%d:color=0x%x\n", i, i);
            draw(i << 11);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
    }
    else
    {
        // blue (32位)
        for (i = 0; i <= 0xff; i++)
        {
            printf("%d:color=0x%x\n", i, i);
            draw(i);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
        // green
        for (i = 0; i <= 0xffff; i += 0x100)
        {
            printf("%d:color=0x%x\n", i, i);
            draw(i);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
        // red
        for (i = 0; i <= 0xffffff; i += 0x10000)
        {
            printf("%d:color=0x%x\n", i, i);
            draw(i);
            if (display_mode == 1)
                getchar();
            else
                usleep(25000);
        }
    }

out:
    framebuffer_close();
    return 0;
}
