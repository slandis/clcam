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
  capture->greyscale = 0;

  capture->brightness = 0;
  capture->gamma = 0;
  capture->sobel = 0;
  capture->laplace = 0;
  capture->noise = 0;
  capture->negative = 0;
  capture->flip = 0;
  capture->mirror = 0;
  capture->lowpass = 0;
  capture->highpass = 0;
  capture->f_index = 0;
  capture->rotate = 0;
  capture->rotate_angle = 0;
}

void print_usage() {
  fprintf(stderr, "clcam - console webcam capture\n");
  fprintf(stderr, "Copyright (C) 2003-2011\n");
  fprintf(stderr, "\tShaun Landis <slandis@plague.org>\n");
  fprintf(stderr, "\tRomesh Kumbhani <kumbhani@overworked.org>\n");
  fprintf(stderr, "usage:\n" );
  #ifdef  ENABLE_FILTERS
  fprintf(stderr, "  -F --filters\t\t\tdisplay filter help\n");
#endif
  fprintf(stderr, "  -H --help\t\t\tdisplay this help screen\n");
#ifdef  DEBUG
  fprintf(stderr, "  -I --info <hw|values>\t\tdisplay hardware info or default values\n");
#endif
  fprintf(stderr, "  -V --version\t\t\tdisplay version and exit\n");
  fprintf(stderr, "  -i --type    <type>\t\ttype of image to dump (JPEG,PNG,PPM)\n");
  fprintf(stderr, "  -o --output  <file name>\tfilename to output image\n");
  fprintf(stderr, "  -q --quality <num>\t\tsets quality to <num>\n");
  fprintf(stderr, "  -t --text \"<text>\"\t\toverlays <text> onto the image\n");
  fprintf(stderr, "  -d --video   <video dev>\tgrab frames from <video dev>\n");
  fprintf(stderr, "  -x --width   <num>\t\tsets the image width\n");
  fprintf(stderr, "  -y --height  <num>\t\tsets the image height\n");
  fprintf(stderr, "  -z --greyscale\t\tsaves image as greyscale\n");
  fprintf(stderr, "     --foreground <#>\t\tsets foreground color for <text>\n");
  fprintf(stderr, "     --background <#>\t\tsets background color for <text>\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Valid image types are: PNG, JPEG and PPM\n");
  fprintf(stderr, "All switches are optional\n");
}

#ifdef  ENABLE_FILTERS
void print_filters() {
  fprintf(stderr, "The following image filters are available:\n");
  fprintf(stderr, "  -b --brightness <#>\tadjusts the image brightness (0-255)\n");
  fprintf(stderr, "  -d --despeckle\tdespeckle filter\n");
  fprintf(stderr, "  -e --noise <#>\tadds amount of noise in the image (0-255)\n");
  fprintf(stderr, "  -f --flip\t\tflips the image vertically\n");
  fprintf(stderr, "  -g --gamma <#>\tadjusts the image gamma (0-255)\n");
  fprintf(stderr, "  -h --highpass\t\tsharpness image filter\n");
  fprintf(stderr, "  -l --lowpass\t\tsmoothing image filter\n");
  fprintf(stderr, "  -m --mirror\t\tmirrors the image horizontally\n");
  fprintf(stderr, "  -n --negative\t\ttransforms the image to a negative of the original\n");
  fprintf(stderr, "  -p --laplace\t\tperforms laplace edge detection on the image\n");
  fprintf(stderr, "  -r --rotate <#>\trotate image by specified number of degrees (0-255)\n");
  fprintf(stderr, "  -s --sobel\t\tperforms sobel edge detection on the image\n");
  fprintf(stderr, "\nREMEMBER: Filters are applied in the order in which you specify them!\n");
}
#endif  /* ENABLE_FILTERS */


#ifdef  ENABLE_FILTERS
 #ifdef DEBUG
  #define OPT_STRING "FHI:Vb:e:fg:hi:klmno:pq:r:st:d:x:y:zY:Z:"
 #else
  #define OPT_STRING "FHVb:e:fg:hi:klmno:pq:r:st:d:x:y:zY:Z:"
 #endif /* DEBUG */
#else
 #ifdef DEBUG
  #define OPT_STRING "HIVi:o:q:t:d:x:y:zY:Z:"
 #else
  #define OPT_STRING "HVi:o:q:t:d:x:y:zY:Z:"
 #endif /* DEBUG */
#endif  /* ENABLE_FILTERS */

static struct option long_options[] = {
#ifdef  ENABLE_FILTERS
  { "filters",    no_argument,        NULL,'F' },
#endif
  { "help",       no_argument,        NULL, 'H' },
#ifdef  DEBUG
  { "info",       no_argument,        NULL, 'I' },
#endif
  { "version",    no_argument,        NULL, 'V' },
#ifdef  ENABLE_FILTERS
  { "brightness", required_argument,  NULL, 'b' },
  { "noise",      required_argument,  NULL, 'e' },
  { "flip",       no_argument,        NULL, 'f' },
  { "gamma",      required_argument,  NULL, 'g' },
  { "highpass",   no_argument,        NULL, 'h' },
#endif  /* ENABLE_FILTERS */
  { "type",       required_argument,  NULL, 'i' },
#ifdef  ENABLE_FILTERS
  { "despeckle",  no_argument,        NULL, 'k' },
  { "lowpass",    no_argument,        NULL, 'l' },
  { "mirror",     no_argument,        NULL, 'm' },
  { "negative",   no_argument,        NULL, 'n' },
#endif  /* ENABLE_FILTERS */
  { "output",     required_argument,  NULL, 'o' },
#ifdef ENABLE_FILTERS
  { "laplace",    no_argument,        NULL, 'p' },
#endif
  { "quality",    required_argument,  NULL, 'q' },
#ifdef  ENABLE_FILTERS
  { "rotate",     required_argument,  NULL, 'r' },
  { "sobel",      no_argument,        NULL, 's' },
#endif  /* ENABLE_FILTERS */
  { "text",       required_argument,  NULL, 't' },
  { "device",     required_argument,  NULL, 'd' },
  { "width",      required_argument,  NULL, 'x' },
  { "height",     required_argument,  NULL, 'y' },
  { "greyscale",  no_argument,        NULL, 'z' },
  { "foreground", required_argument,  NULL, 'Y' },
  { "background", required_argument,  NULL, 'Z' },
  { 0, 0, 0, 0 }
};

/*
 * Hey boss, it's a main()!
 */
int main(int argc, char **argv) {
  static Capture capture;
  setDefaults(&capture);

  for (;;) {
    int index, c = 0;
                
    c = getopt_long(argc, argv, OPT_STRING, long_options, &index);

    if (-1 == c)
      break;

    switch (c) {
#ifdef  ENABLE_FILTERS
      case 'F' :
        print_filters();
        exit(1);
        break;
#endif
      case 'H' :
        print_usage();
        exit(0);
        break;
#ifdef  DEBUG
      case 'I' :
        if (!strcmp(optarg, "hw")) {
          print_defaults();
          exit(1);
        } else if (!strcmp(optarg, "values")) {
          open_cam(&qc);
          print_cam_info(&qc);
          close_cam(&qc);
          exit(1);
        } else {
          print_usage();
          exit(1);
        }
        break;
#endif
      case 'V' :
        printf( "clcam version %s\n", version);
        exit(0);
        break;
#ifdef  ENABLE_FILTERS
      case 'b' :  /* brightness */
        capture.brightness = atoi(optarg);
        f_chain[capture.f_index] = &filter_brightness;
        capture.f_index++;
        break;
      case 'd' :  /* device name */
        capture.device = malloc(strlen(optarg)+1);
        snprintf(capture.device, strlen(optarg)+1, "%s", optarg);
        break;
      case 'e' :        /* noise */
        capture.noise = atoi(optarg);
        f_chain[capture.f_index] = & filter_noise;
        capture.f_index++;
        break;
      case 'f' :        /* flip */
        f_chain[capture.f_index] = &filter_flip;
        capture.f_index++;
        break;
      case 'g' :        /* gamma */
        capture.gamma = atoi(optarg);
        f_chain[capture.f_index] = &filter_gamma;
        capture.f_index++;
        break;
       case 'h' :        /* highpass */
         f_chain[capture.f_index] = &filter_highpass;
         capture.f_index++;
         break;
#endif
      case 'i' :
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
#ifdef  ENABLE_FILTERS
      case 'k' :        /* despeckle */
        capture.despeckle = 1;
        f_chain[capture.f_index] = &filter_despeckle;
        capture.f_index++;
        break;
      case 'l' :        /* lowpass */
        f_chain[capture.f_index] = &filter_lowpass;
        capture.f_index++;
        break;
      case 'm' :        /* mirror */
        f_chain[capture.f_index] = &filter_mirror;
        capture.f_index++;
        break;
      case 'n' :        /* negative */
        f_chain[capture.f_index] = &filter_negative;
        capture.f_index++;
        break;
#endif
      case 'o' :  /* output file */
        capture.filename = malloc(strlen(optarg)+1);
        snprintf(capture.filename, strlen(optarg)+1, "%s", optarg);
        break;
#ifdef  ENABLE_FILTERS
      case 'p' :        /* laplace */
        f_chain[capture.f_index] = &filter_laplace;
        capture.f_index++;
        break;
#endif
      case 'q' :  /* jpeg quality */
        capture.save.quality = atoi(optarg);
        break;
#ifdef  ENABLE_FILTERS
      case 'r' :  /* rotate */
        capture.rotate_angle = atoi(optarg);
        f_chain[capture.f_index] = &filter_rotate;
        capture.f_index++;
        break;
      case 's' :        /* sobel */
        f_chain[capture.f_index] = &filter_sobel;
        capture.f_index++;
         break;
#endif
      case 't' :  /* text stamp */
        bcopy(optarg, capture.text_str, 57);
        capture.text = 1;
        break;
      case 'x' :
        capture.width = atoi(optarg);
        break;
      case 'y' :
        capture.height = atoi(optarg);
        break;
      case 'z' :
        capture.greyscale = 1;
        break;
      case 'Y' :
        sscanf(optarg, "%lx", &capture.text_fg);
        break;
      case 'Z' :
        sscanf(optarg, "%lx", &capture.text_bg);
        break;
      case EOF :
        break;
    }
  }

  deviceOpen(&capture);
  deviceInit(&capture);
  captureStart(&capture);
  
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

      if (frameRead(&capture))
        break;
        
      /* EAGAIN - continue select loop. */
    }
  }

  captureStop(&capture);
  deviceUninit(&capture);
  deviceClose(&capture);
  exit(EXIT_SUCCESS);

  return 0;
}
