/*
 * clcam, video.c
 * Copyright (C) 2011
 *  Shaun Landis
 *
 * Please see the file COPYING for license information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include "clcam.h"
#include "save.h"
#include "filters.h"

#define CLEAR(x) memset(&(x), 0, sizeof (x))
#define CLIP(x) ((x) >= 0xFF ? 0xFF : ( (x) <= 0x00 ? 0x00 : (x)))

void YUV422toRGB888(Capture *capture) {
  int line, column;
  unsigned char *py, *pu, *pv;
  unsigned char *src = (unsigned char *)capture->memory;
  unsigned char *tmp = capture->image;

  /* In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr. 
     Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels. */
  py = src;
  pu = src + 1;
  pv = src + 3;

  for (line = 0; line < capture->height; ++line) {
    for (column = 0; column < capture->width; ++column) {
      *tmp++ = CLIP((double)*py + 1.402*((double)*pv-128.0));
      *tmp++ = CLIP((double)*py - 0.344*((double)*pu-128.0) - 0.714*((double)*pv-128.0));      
      *tmp++ = CLIP((double)*py + 1.772*((double)*pu-128.0));

      py += 2;

      if ((column & 1)==1) {
        pu += 4;
        pv += 4;
      }
    }
  }
}

void errno_exit(const char* s) {
  fprintf(stderr, "%s error %d, %s\n", s, errno, strerror (errno));
  exit(EXIT_FAILURE);
}

int xioctl(int fd, int request, void *argp) {
  int r;

  do r = ioctl(fd, request, argp);
  while (r < 0 && EINTR == errno);

  return r;
}

void imageProcess(Capture *capture) {
  capture->size = capture->width * capture->height * 3 * sizeof(char);
  capture->image = malloc(capture->size);

  YUV422toRGB888(capture);

  /*
   * From here on, the RGB image will reside in capture->image. Now
   * we can pass this through a filter chain, or to any of the image
   * saving functions
   */

  walk_filters(capture);

  if (capture->text)
    (void)create_stamp(capture);
  
  if (capture->savetype == PPM) {
    capture->save.format = RAW;
    ppm_save(capture);
#ifdef  ENABLE_JPEG
  } else if (capture->savetype == JPEG) {
    capture->save.optimize = 0;
    jpeg_save(capture);
#endif
#ifdef  ENABLE_PNG
  } else if (capture->savetype == PNG) {
    capture->save.interlace = PNG_INTERLACE_NONE;
    capture->save.compression = 6;
    png_save(capture);
#endif
  }
}

int frameRead(Capture *capture) {
  struct v4l2_buffer buffer;
  CLEAR(buffer);

  buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory = V4L2_MEMORY_MMAP;

  if (xioctl(capture->fd, VIDIOC_DQBUF, &buffer) < 0) {
    switch (errno) {
      case EAGAIN:
        return 0;

      case EIO:
        break;

      default:
        errno_exit("VIDIOC_DQBUF");
    }
  }
  
  imageProcess(capture);

  return 1;
}

void captureStop(Capture *capture) {
  enum v4l2_buf_type type;

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (xioctl(capture->fd, VIDIOC_STREAMOFF, &type) < 0)
    errno_exit("VIDIOC_STREAMOFF");
}

void captureStart(Capture *capture) {
  struct v4l2_buffer buffer;
  enum v4l2_buf_type type;

  CLEAR (buffer);

  buffer.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory      = V4L2_MEMORY_MMAP;
  buffer.index       = 0;

  if (xioctl(capture->fd, VIDIOC_QBUF, &buffer) < 0)
    errno_exit("VIDIOC_QBUF");
                
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (xioctl(capture->fd, VIDIOC_STREAMON, &type) < 0)
    errno_exit("VIDIOC_STREAMON");
}

void deviceUninit(Capture *capture) {
  if (munmap(capture->memory, capture->length) < 0)
    errno_exit("munmap");

  free(capture->image);
}

void mmapInit(Capture *capture) {
  struct v4l2_requestbuffers rbuffer;
  CLEAR (rbuffer);

  rbuffer.count   = 1;
  rbuffer.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  rbuffer.memory  = V4L2_MEMORY_MMAP;

  if (xioctl(capture->fd, VIDIOC_REQBUFS, &rbuffer) < 0) {
    if (EINVAL == errno) {
      fprintf(stderr, "%s does not support memory mapping\n", capture->device);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_REQBUFS");
    }
  }
 
  if (rbuffer.count < 1) {
    fprintf(stderr, "Insufficient buffer memory on %s\n", capture->device);
    exit(EXIT_FAILURE);
  }

  struct v4l2_buffer buffer;
  CLEAR (buffer);

  buffer.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer.memory      = V4L2_MEMORY_MMAP;
  buffer.index       = 0;

  if (xioctl(capture->fd, VIDIOC_QUERYBUF, &buffer) < 0)
    errno_exit("VIDIOC_QUERYBUF");
  
  capture->length = buffer.length;
  capture->memory = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, capture->fd, buffer.m.offset);

  if (MAP_FAILED == capture->memory)
    errno_exit("mmap");
}

void deviceInit(Capture *capture) {
  struct v4l2_capability cap;
  struct v4l2_cropcap cropcap;
  struct v4l2_crop crop;
  struct v4l2_format fmt;
  unsigned int min;

  if (xioctl(capture->fd, VIDIOC_QUERYCAP, &cap) < 0) {
    if (EINVAL == errno) {
      fprintf(stderr, "%s is no V4L2 device\n", capture->device);
      exit(EXIT_FAILURE);
    } else {
      errno_exit("VIDIOC_QUERYCAP");
    }
  }

  if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    fprintf(stderr, "%s is no video capture device\n", capture->device);
    exit(EXIT_FAILURE);
  }

  if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n", capture->device);
        exit(EXIT_FAILURE);
      }

  /* Select video input, video standard and tune here. */
  CLEAR(cropcap);

  cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (xioctl(capture->fd, VIDIOC_CROPCAP, &cropcap) == 0) {
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c = cropcap.defrect; /* reset to default */

    if (xioctl(capture->fd, VIDIOC_S_CROP, &crop) < 0) {
      switch (errno) {
        case EINVAL:
          break;

        default:
          break;
      }
    }
  }

  CLEAR (fmt);

  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = capture->width; 
  fmt.fmt.pix.height      = capture->height;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

  if (xioctl(capture->fd, VIDIOC_S_FMT, &fmt) < 0)
    errno_exit("VIDIOC_S_FMT");

  /* Note VIDIOC_S_FMT may change width and height. */
  if (capture->width != fmt.fmt.pix.width) {
    capture->width = fmt.fmt.pix.width;
    fprintf(stderr,"Image width set to %i by device %s.\n", capture->width, capture->device);
  }
  if (capture->height != fmt.fmt.pix.height) {
    capture->height = fmt.fmt.pix.height;
    fprintf(stderr,"Image height set to %i by device %s.\n", capture->height, capture->device);
  }

  capture->size = capture->height * capture->width * 3;

  /* Buggy driver paranoia. */
  min = fmt.fmt.pix.width * 2;
  if (fmt.fmt.pix.bytesperline < min)
    fmt.fmt.pix.bytesperline = min;
  min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
  if (fmt.fmt.pix.sizeimage < min)
    fmt.fmt.pix.sizeimage = min;

  mmapInit(capture);
}

void deviceClose(Capture *capture) {
  if (close(capture->fd) < 0)
    errno_exit("close");

  capture->fd = -1;
}

void deviceOpen(Capture *capture) {
  struct stat st;

  if (stat(capture->device, &st) < 0) {
    fprintf(stderr, "Cannot identify '%s': %d, %s\n", capture->device, errno, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (!S_ISCHR (st.st_mode)) {
    fprintf(stderr, "%s is no device\n", capture->device);
    exit(EXIT_FAILURE);
  }

  capture->fd = open(capture->device, O_RDWR | O_NONBLOCK, 0);

  if (capture->fd < 0) {
    fprintf(stderr, "Cannot open '%s': %d, %s\n", capture->device, errno, strerror (errno));
    exit(EXIT_FAILURE);
  }
}
