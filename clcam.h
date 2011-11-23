/*
 * clcam, clcam.h
 * Copyright (C) 2003-2011
 *  Shaun Landis
 *  Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

#ifdef  ENABLE_PNG
#include <png.h>
#endif

typedef struct _save {
  int isinfo;
  int quality;
  int smoothness;
  int optimize;
  int format;
#ifdef  ENABLE_PNG
  png_byte interlace;
#endif
  int compression;
} Save;

typedef struct _capture {
  int           fd;
  char          *device;
  
  char *filename;
  int width;
  int height;
  int size;
  int depth;
  Save save;
  int savetype;
  char *saveext;

  void *memory;
  uint length;

  unsigned char *image;

  int greyscale;
  int swapcolors;

  long text_fg;
  long text_bg;
  char text_str[57];
  int text;

  /* Filter flags */
  int brightness;
  int gamma;
  int sobel;
  int laplace;
  int noise;
  int negative;
  int flip;
  int mirror;
  int lowpass;
  int highpass;
  int f_index;
  int despeckle;
  int rotate;
  float rotate_angle;
} Capture;
