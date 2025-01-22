#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

struct buffer_object
{
  uint32_t width;
  uint32_t height;
  uint32_t pitch;
  uint32_t handle;
  uint32_t size;
  uint32_t *vaddr;
  uint32_t fb_id;
};

#define BUFFER_COUNT 3
struct buffer_object buf[BUFFER_COUNT];
uint32_t colors[BUFFER_COUNT] = {0xff0000, 0x00ff00, 0x0000ff};

static int modeset_create_fb(int fd, struct buffer_object *bo, uint32_t color)
{
  struct drm_mode_create_dumb create = {};
  struct drm_mode_map_dumb map = {};

  create.width = bo->width;
  create.height = bo->height;
  create.bpp = 32;
  if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0)
  {
    fprintf(stderr, "Failed to create dumb buffer: %s\n", strerror(errno));
    return -1;
  }

  bo->pitch = create.pitch;
  bo->size = create.size;
  bo->handle = create.handle;
  if (drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
                   bo->handle, &bo->fb_id) < 0)
  {
    fprintf(stderr, "Failed to add framebuffer: %s\n", strerror(errno));
    return -1;
  }

  map.handle = create.handle;
  if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) < 0)
  {
    fprintf(stderr, "Failed to map dumb buffer: %s\n", strerror(errno));
    return -1;
  }

  bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, map.offset);
  if (bo->vaddr == MAP_FAILED)
  {
    fprintf(stderr, "Failed to mmap buffer: %s\n", strerror(errno));
    return -1;
  }

  for (uint32_t i = 0; i < (bo->size / 4); i++)
    bo->vaddr[i] = color;

  return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
  struct drm_mode_destroy_dumb destroy = {};

  drmModeRmFB(fd, bo->fb_id);

  munmap(bo->vaddr, bo->size);

  destroy.handle = bo->handle;
  drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}

static void usage()
{
  printf(
      "usage: drmtest [option] \n"
      "   -h: this message\n"
      "   -d: switch play on card0 or card1.\n"
      "       default /dev/dri/card0.\n"
      "   -t: show times.\n"
      "       default 16.\n");
}

int main(int argc, char **argv)
{
  int fd, c, times = 18, color_index = 0;
  drmModeConnector *conn;
  drmModeRes *res;
  uint32_t conn_id;
  uint32_t crtc_id;

  char drmfilename[16] = "/dev/dri/card0";

  while ((c = getopt(argc, argv, "d:t")) != -1)
  {
    switch (c)
    {
    case 'd':
      strcpy(drmfilename, optarg);
      break;
    case 't':
      times = atoi(optarg);
      break;
    case '?':
    case 'h':
      usage();
      return 1;
    }
  }

  fd = open(drmfilename, O_RDWR | O_CLOEXEC);
  if (fd < 0)
  {
    fprintf(stderr, "Failed to open DRM device: %s\n", strerror(errno));
    return -1;
  }

  res = drmModeGetResources(fd);
  crtc_id = res->crtcs[0];
  conn_id = res->connectors[0];

  conn = drmModeGetConnector(fd, conn_id);

  for (int i = 0; i < BUFFER_COUNT; i++)
  {
    buf[i].width = conn->modes[0].hdisplay;
    buf[i].height = conn->modes[0].vdisplay;
  }

  for (int i = 0; i < BUFFER_COUNT; i++)
  {
    if (modeset_create_fb(fd, &buf[i], colors[i]) < 0)
    {
      fprintf(stderr, "Failed to create framebuffer for color %d\n", i);
      goto fail;
    }
  }

  for (int i = 0; i < times; i++)
  {
    if (drmModeSetCrtc(fd, crtc_id, buf[color_index].fb_id, 0, 0, &conn_id, 1, &conn->modes[0]) < 0)
    {
      fprintf(stderr, "Failed to set CRTC: %s\n", strerror(errno));
      break;
    }
    usleep(800000);
    color_index++;
    if (color_index > BUFFER_COUNT - 1)
      color_index = 0;
  }

fail:

  for (int i = 0; i < BUFFER_COUNT; i++)
  {
    modeset_destroy_fb(fd, &buf[i]);
  }
  drmModeFreeConnector(conn);
  drmModeFreeResources(res);
  close(fd);

  return 0;
}
