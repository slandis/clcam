/*
 * clcam, stamp.c
 * Copyright (C) 2003-2011
 *  Shaun Landis
 *
 * Please see the file COPYING for license information
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <linux/videodev2.h>

#include "clcam.h"	/* For Capture structure */
#include "stamp.h"

void create_stamp(Capture *capture) {
  time_t t;
  struct tm  *tm;
  char line[57], *imagePtr;
  int i, x, y, f, p, len;
  int fg[3], bg[3];

#ifdef  DEBUG
   fprintf(stderr, "Performing create_stamp()...\n");
#endif

  /*
   * Initialize our time structures in
   * case the stamp includes any time
   * formatting, then format it all out.
   */
  time(&t);
  tm = localtime(&t);
  len = strftime(line, 57, capture->text_str, tm);
  
  /*
   * Extract RGB values from the hex codes
   * for both foreground and background.
   */
  fg[0] = (capture->text_fg >> 16) & 0xFF;
  fg[1] = (capture->text_fg >> 8) & 0xFF;
  fg[2] = capture->text_fg & 0xFF;
  
  bg[0] = (capture->text_bg >> 16) & 0xFF;
  bg[1] = (capture->text_bg >> 8) & 0xFF;
  bg[2] = capture->text_bg & 0xFF;
  
#ifdef	DEBUG
  fprintf(stderr,"Adding stamp: %s\n",line);
#endif

  /*
   * Draw the filled background box
   */
  for (y = 0; y < CHAR_HEIGHT+1; y++) {
    imagePtr = capture->image + 3 * capture->width * (capture->height-CHAR_HEIGHT+y);

    for (x = 0; x < capture->width; x++) {
      imagePtr[0] = 0;
      imagePtr[1] = 0;
      imagePtr[2] = 0;
      
      imagePtr += 3;
    }
  }

  /*
   * Now we overlay the text on top of
   * the background box, which very well
   * could be "transparent".
   */
  for (y = 0; y < CHAR_HEIGHT; y++) {
    imagePtr = capture->image + 3 * capture->width * (capture->height-CHAR_HEIGHT+y);

    for (x = 0; x < len; x++) {
      f = fontdata_6x11[line[x] * CHAR_HEIGHT + y];

      for (i = CHAR_WIDTH-1; i >= 0; i--) {
        if (f & (CHAR_START << i)) {
          imagePtr[0] = fg[0];
          imagePtr[1] = fg[1];
          imagePtr[2] = fg[2];
        } else if (capture->text_bg != 0xFFFFFFFF && bg[0] > -1) {
          imagePtr[0] = bg[0];
          imagePtr[1] = bg[1];
          imagePtr[2] = bg[2];
        }
       
        imagePtr += 3;
      }
    }
  }
  
#ifdef  DEBUG
  fprintf(stderr, "done.\n");
#endif
}
