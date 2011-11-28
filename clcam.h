/*
 * clcam, clcam.h
 * Copyright (C) 2003-2011
 *  Shaun Landis
 *  Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

#define DEBUG 1

#ifdef  ENABLE_PNG
#include <png.h>
#endif

typedef int bool;
enum bool { false, true };

enum {
  FILTER_BRIGHTNESS = 0,
  FILTER_NOISE,
  FILTER_FLIP,
  FILTER_GAMMA,
  FILTER_HIGHPASS,
  FILTER_DESPECKLE,
  FILTER_LOWPASS,
  FILTER_MIRROR,
  FILTER_NEGATIVE,
  FILTER_LAPLACE,
  FILTER_PREDATOR,
  FILTER_SOBEL,
  FILTER_GREYSCALE
};

enum {
  HW_BRIGHTNESS = 0,
  HW_CONTRAST,
  HW_SATURATION,
  HW_HUE,
  HW_UNUSED,
  HW_AUDIO_VOLUME,
  HW_AUDIO_BALANCE,
  HW_AUDIO_BASS,
  HW_AUDIO_TREBLE,
  HW_AUDIO_MUTE,
  HW_AUDIO_LOUDNESS,
  HW_BLACK_LEVEL,
  HW_AUTO_WHITE_BALANCE,
  HW_DO_WHITE_BALANCE,
  HW_RED_BALANCE,
  HW_BLUE_BALANCE,
  HW_GAMMA,
  HW_EXPOSURE,
  HW_AUTOGAIN,
  HW_GAIN,
  HW_HFLIP,
  HW_VFLIP,
  HW_HCENTER,
  HW_VCENTER,
  HW_LINE_FREQUENCY,
  HW_HUE_AUTO,
  HW_WB_TEMPERATURE,
  HW_SHARPNESS,
  HW_BACKLIGHT_COMP
};

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
  
  int           c_index;
  char          *filename;
  int           width;
  int           height;
  int           size;
  int           depth;
  Save          save;
  int           savetype;
  char          *saveext;

  void          *memory;
  uint          length;

  unsigned char *image;

  int           greyscale;
  int           swapcolors;

  long          text_fg;
  long          text_bg;
  char          text_str[57];
  int           text;

#ifdef  ENABLE_FILTERS
  /* Filter flags */
  int           brightness;
  int           gamma;
  int           noise;
  int           f_index;
  int           rotate;
#endif
} Capture;

char *get_option_name(int);