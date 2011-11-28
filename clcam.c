/*
 * clcam, clcam.c
 * Copyright (C) 2003-2011
 *  Shaun Landis
 *  Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <linux/videodev2.h>

#include "clcam.h"
#include "save.h"
#include "video.h"
#include "filters.h"

char version[] = VERSION;

static void setDefaults(Capture *capture) {
  capture->savetype = JPEG;
  capture->saveext = "jpg";
  capture->fd = -1;
  capture->device = "/dev/video0";
  capture->filename = "capture.jpg";
  capture->save.smoothness = 0;
  capture->save.quality = 85;

  capture->width = 640;
  capture->height = 480;
  capture->depth = 3;
  capture->size = capture->width * capture->height * capture->depth;
  capture->text_fg = 0xFFFFFF;
  capture->text_bg = 0xFFFFFFFF;

  capture->swapcolors = 0;

  capture->brightness = 0;
  capture->gamma = 0;
  capture->noise = 0;
  capture->f_index = 0;
  capture->rotate = 0;
  capture->c_index = 0;
}

void print_usage() {
  fprintf(stderr, "clcam - console webcam capture\n");
  fprintf(stderr, "Copyright (C) 2003-2011\n");
  fprintf(stderr, "\tShaun Landis <slandis@plague.org>\n");
  fprintf(stderr, "\tRomesh Kumbhani <kumbhani@overworked.org>\n");
  fprintf(stderr, "usage:\n" );
#ifdef  ENABLE_FILTERS
  fprintf(stderr, "  -F --help-filters\t\t\tdisplay filter help\n");
#endif
  fprintf(stderr, "  -C --help-controls\t\tdisplay hardware control help\n");
  fprintf(stderr, "  -H --help\t\t\tdisplay this help screen\n");
#ifdef  DEBUG
  fprintf(stderr, "  -i --info <hw|values>\t\tdisplay hardware info or default values\n");
#endif
  fprintf(stderr, "  -v --version\t\t\tdisplay version and exit\n");
  fprintf(stderr, "  -c --controls [control1=val,control2=val]\n");
  fprintf(stderr, "  -f --filters [filter1[=val],filter2[=val]...]\n");
  fprintf(stderr, "  -o --output  <file name>\tfilename to output image\n");
  fprintf(stderr, "  -p --type    <type>\t\ttype of image to dump (JPEG,PNG,PPM)\n");
  fprintf(stderr, "  -q --quality <num>\t\tsets quality to <num>\n");
  fprintf(stderr, "  -t --text \"<text>\"\t\toverlays <text> onto the image\n");
  fprintf(stderr, "  -d --video   <video dev>\tgrab frames from <video dev>\n");
  fprintf(stderr, "  -x --width   <num>\t\tsets the image width\n");
  fprintf(stderr, "  -y --height  <num>\t\tsets the image height\n");
  fprintf(stderr, "     --foreground <#>\t\tsets foreground color for <text>\n");
  fprintf(stderr, "     --background <#>\t\tsets background color for <text>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Valid image types are: PNG, JPEG and PPM\n");
  fprintf(stderr, "All switches are optional\n");
}

#ifdef  ENABLE_FILTERS
void print_filters() {
  fprintf(stderr, "The following image filters are available:\n");
  fprintf(stderr, "  brightness <#>\tadjusts the image brightness (0-255)\n");
  fprintf(stderr, "  despeckle\tdespeckle filter\n");
  fprintf(stderr, "  noise <#>\tadds noise to the image (0-255)\n");
  fprintf(stderr, "  flip\t\tflips the image vertically\n");
  fprintf(stderr, "  gamma <#>\tadjusts the image gamma (0-255)\n");
  fprintf(stderr, "  highpass\tsharpness image filter\n");
  fprintf(stderr, "  lowpass\tsmoothing image filter\n");
  fprintf(stderr, "  mirror\tmirrors the image horizontally\n");
  fprintf(stderr, "  negative\ttransforms the image to a negative of the original\n");
  fprintf(stderr, "  laplace\tperforms laplace edge detection on the image\n");
  fprintf(stderr, "  predator\tpsuedo \"infrared\" as in the movie 'Predator'\n");
  fprintf(stderr, "  sobel\t\tperforms sobel edge detection on the image\n");
  fprintf(stderr, "  greyscale\tconvert rgb image to greyscale using the luminosity method\n");
  fprintf(stderr, "\nREMEMBER: Filters are applied in the order in which you specify them!\n");
}
#endif  /* ENABLE_FILTERS */


#ifdef  ENABLE_FILTERS
 #ifdef DEBUG
  #define OPT_STRING "c:Cd:f:Fhio:p:q:s:t:u:vx:y:F:B:"
 #else
  #define OPT_STRING "c:Cd:f:Fho:p:q:s:t:u:vx:y:F:B:"
 #endif /* DEBUG */
#else
 #ifdef DEBUG
  #define OPT_STRING "c:Cd:hio:p:q:s:t:u:vx:y:F:B:"
 #else
  #define OPT_STRING "c:Cd:ho:p:q:s:t:u:vx:F:B:"
 #endif /* DEBUG */
#endif  /* ENABLE_FILTERS */

static struct option long_options[] = {
  { "controls",     required_argument,  NULL, 'c' },
  { "help-controls", no_argument,       NULL, 'C' },
  { "device",       required_argument,  NULL, 'd' },
#ifdef  ENABLE_FILTERS
  { "filters",      required_argument,  NULL, 'f' },
  { "help-filters", no_argument,        NULL, 'F' },
#endif
  { "help",         no_argument,        NULL, 'h' },
#ifdef  DEBUG
  { "info",         no_argument,        NULL, 'i' },
#endif
  { "output",       required_argument,  NULL, 'o' },
  { "type",         required_argument,  NULL, 'p' },
  { "quality",      required_argument,  NULL, 'q' },
  { "text",         required_argument,  NULL, 's' },
  { "foreground",   required_argument,  NULL, 't' },
  { "background",   required_argument,  NULL, 'u' },
  { "version",      no_argument,        NULL, 'v' },
  { "width",        required_argument,  NULL, 'x' },
  { "height",       required_argument,  NULL, 'y' },
  { 0, 0, 0, 0 }
};

char *const filter_opts[] = {
  "brightness",
  "noise",
  "flip",
  "gamma",
  "highpass",
  "despeckle",
  "lowpass",
  "mirror",
  "negative",
  "laplace",
  "predator",
  "sobel",
  "greyscale",
  NULL
};

char *const control_opts[] = {
  "brightness",
  "contrast",
  "saturation",
  "hue",
  NULL,
  "audio-volume",
  "audio-balance",
  "audio-bass",
  "audio-treble",
  "audio-mute",
  "audio-loudness",
  "black-level",
  "auto-white-balance",
  "do-white-balance",
  "red-balance",
  "blue-balance",
  "gamma",
  "exposure",
  "autogain",
  "gain",
  "horizontal-flip",
  "vertical-flip",
  NULL,
  NULL,
  "line-frequency",
  "hue-auto",
  "white-balance-temperature",
  "sharpness",
  "backlight-compensation",
  NULL
};

char *get_option_name(int id) {
  return control_opts[id - V4L2_CID_BASE];
}

/*
 * Hey boss, it's a main()!
 */
int main(int argc, char **argv) {
  static Capture capture;
  setDefaults(&capture);
  int enumerate = 0;

  for (;;) {
    int index, c = 0;
    char *value; 

    c = getopt_long(argc, argv, OPT_STRING, long_options, &index);

    if (-1 == c)
      break;

    switch (c) {
      case 'c' :
        while ((index = getsubopt(&optarg, control_opts, &value)) != -1) {
          struct v4l2_control control;

          switch(index) {
            case HW_BRIGHTNESS :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_BRIGHTNESS;
              c_chain[capture.c_index++] = control;
              break;
            case HW_CONTRAST :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_CONTRAST;
              c_chain[capture.c_index++] = control;
              break;
            case HW_SATURATION :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_SATURATION;
              c_chain[capture.c_index++] = control;
              break;
            case HW_HUE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_HUE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_VOLUME :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_AUDIO_VOLUME;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_BALANCE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_AUDIO_BALANCE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_BASS :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_AUDIO_BASS;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_TREBLE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_AUDIO_TREBLE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_MUTE :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_AUDIO_MUTE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUDIO_LOUDNESS :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_AUDIO_LOUDNESS;
              c_chain[capture.c_index++] = control;
              break;
            case HW_BLACK_LEVEL :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_BLACK_LEVEL;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUTO_WHITE_BALANCE :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_AUTO_WHITE_BALANCE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_DO_WHITE_BALANCE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_DO_WHITE_BALANCE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_RED_BALANCE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_RED_BALANCE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_BLUE_BALANCE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_BLUE_BALANCE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_GAMMA :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_GAMMA;
              c_chain[capture.c_index++] = control;
              break;
            case HW_EXPOSURE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_EXPOSURE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_AUTOGAIN :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_AUTOGAIN;
              c_chain[capture.c_index++] = control;
              break;
            case HW_GAIN :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_GAIN;
              c_chain[capture.c_index++] = control;
              break;
            case HW_HFLIP :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_HFLIP;
              c_chain[capture.c_index++] = control;
              break;
            case HW_VFLIP :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_VFLIP;
              c_chain[capture.c_index++] = control;
              break;
            case HW_LINE_FREQUENCY :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_POWER_LINE_FREQUENCY;
              c_chain[capture.c_index++] = control;
              break;
            case HW_HUE_AUTO :
              control.value = ((value == NULL) ? false : true);
              control.id = V4L2_CID_HUE_AUTO;
              c_chain[capture.c_index++] = control;
              break;
            case HW_WB_TEMPERATURE :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
              c_chain[capture.c_index++] = control;
              break;
            case HW_SHARPNESS :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_SHARPNESS;
              c_chain[capture.c_index++] = control;
              break;
            case HW_BACKLIGHT_COMP :
              control.value = ((value == NULL) ? 0 : atoi(value));
              control.id = V4L2_CID_BACKLIGHT_COMPENSATION;
              c_chain[capture.c_index++] = control;
              break;
            default :
              break;
          }
        }
        break;
      case 'C' :
        enumerate = 1;
        break;
#ifdef  ENABLE_FILTERS
      case 'f' :
        while ((index = getsubopt(&optarg, filter_opts, &value)) != -1) {
          switch(index) {
            case FILTER_BRIGHTNESS :
              capture.brightness = ((value == NULL) ? 0 : atoi(value));
              f_chain[capture.f_index++] = &filter_brightness;
              break;
            case FILTER_NOISE :
              capture.noise = ((value == NULL) ? 0 : atoi(value));
              f_chain[capture.f_index++] = &filter_noise;
              break;
            case FILTER_FLIP :
              f_chain[capture.f_index++] = &filter_flip;
              break;
            case FILTER_GAMMA :
              capture.gamma = ((value == NULL) ? 0 : atoi(value));
              f_chain[capture.f_index++] = &filter_gamma;
              break;
            case FILTER_HIGHPASS :
              f_chain[capture.f_index++] = &filter_highpass;
              break;
            case FILTER_DESPECKLE :
              f_chain[capture.f_index++] = &filter_despeckle;
              break;
            case FILTER_LOWPASS :
              f_chain[capture.f_index++] = &filter_lowpass;
              break;
            case FILTER_MIRROR :
              f_chain[capture.f_index++] = &filter_mirror;
              break;
            case FILTER_NEGATIVE :
              f_chain[capture.f_index++] = &filter_negative;
              break;
            case FILTER_LAPLACE :
              f_chain[capture.f_index++] = &filter_laplace;
              break;
            case FILTER_PREDATOR :
              f_chain[capture.f_index++] = &filter_predator;
              break;
            case FILTER_SOBEL :
              f_chain[capture.f_index++] = &filter_sobel;
              break;
            case FILTER_GREYSCALE :
              f_chain[capture.f_index++] = &filter_greyscale;
            default :
              break;
          }
        }
        break;
      case 'F' :
        print_filters();
        exit(0);
        break;
#endif
      case 'h' :
        print_usage();
        exit(0);
        break;
        /*
#ifdef  DEBUG
      case 'i' :
        if (!strcmp(optarg, "hw")) {
          print_defaults();
          exit(1);
        } else if (!strcmp(optarg, "values")) {
          open_cam(&);
          print_cam_info(&qc);
          close_cam(&qc);
          exit(1);
        } else {
          print_usage();
          exit(1);
        }
        break;
#endif
*/
      case 'd' :  /* device name */
        capture.device = malloc(strlen(optarg)+1);
        snprintf(capture.device, strlen(optarg)+1, "%s", optarg);
        break;
      case 'o' :  /* output file */
        capture.filename = malloc(strlen(optarg)+1);
        snprintf(capture.filename, strlen(optarg)+1, "%s", optarg);
        break;
      case 'p' :
        if(!strcmp(optarg, "PPM")) {
          capture.savetype = PPM;
          capture.saveext = "ppm";
        } else if(!strcmp(optarg, "PNG")) {
#ifdef  ENABLE_PNG
          capture.savetype = PNG;
          capture.saveext = "png";
#else
          print_options();
          exit(-1);
#endif
        } else if(!strcmp(optarg, "JPEG")) {
#ifdef  ENABLE_JPEG
          capture.savetype = JPEG;
          capture.saveext = "jpg";
#else
          print_options();
          exit(-1);
#endif
        }
        break;
      case 'q' :  /* jpeg quality */
        capture.save.quality = atoi(optarg);
        break;
      case 's' :  /* text stamp */
        bcopy(optarg, capture.text_str, 57);
        capture.text = 1;
        break;
      case 't' :
        sscanf(optarg, "%lx", &capture.text_fg);
        break;
      case 'u' :
        sscanf(optarg, "%lx", &capture.text_bg);
        break;
      case 'v' :
        printf( "clcam version %s\n", version);
        exit(0);
        break;
      case 'x' :
        capture.width = atoi(optarg);
        break;
      case 'y' :
        capture.height = atoi(optarg);
        break;
      case EOF :
        break;
    }
  }

  device_open(&capture);
  device_init(&capture);
  capture_start(&capture);
  
  if (enumerate) {
    enum_controls(&capture);
    exit(1);
  }

  reset_controls(&capture);
  walk_controls(&capture);

  int count = 1;

  while (count-- > 0) {
    for (;;) {
      fd_set fds;
      struct timeval tv;
      int r;

      FD_ZERO(&fds);
      FD_SET(capture.fd, &fds);

      /* Timeout. */
      tv.tv_sec = 3;
      tv.tv_usec = 0;

      r = select(capture.fd + 1, &fds, NULL, NULL, &tv);

      if (-1 == r) {
        if (EINTR == errno)
          continue;

        fprintf(stderr, "Error in select(): %s\n", strerror(errno));
        exit(-1);
      }

      if (0 == r) {
        fprintf (stderr, "select timeout\n");
        exit(EXIT_FAILURE);
      }

      if (frame_read(&capture))
        break;
        
      /* EAGAIN - continue select loop. */
    }
  }

  capture_stop(&capture);
  device_uninit(&capture);
  device_close(&capture);
  exit(EXIT_SUCCESS);

  return 0;
}
