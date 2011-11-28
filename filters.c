/*
 * clcam, filters.c
 * Copyright (C) 2003-2011
 *	Shaun Landis
 *	Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */
 
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/types.h>
#include <signal.h>
#include <png.h>
#include <linux/videodev2.h>

#include "clcam.h"
#include "filters.h"

/*
 * This really isn't a filter..
 * Swap rgb positions incase things are
 * a bit backwards.
 */
void swap_rgb24(unsigned char *mem, int n) {
  unsigned char  c;
  unsigned char *p = mem;
  int   i = n ;

#ifdef	DEBUG
  fprintf(stderr, "Performing swap_rgb24()...");
#endif

  while (--i) {
    c = p[0];
    p[0] = p[2];
    p[2] = c;
    p += 3;
  }

#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Walk through the filter chain, calling
 * each filter function in the order it was
 * specified on the command line.
 */
#ifdef ENABLE_FILTERS
void walk_filters(Capture *capture) {
  int i;
  
  for (i = 0; i < capture->f_index; i++)
    f_chain[i](capture);
}

/*
 * Brightness adjustment, from 0 - 255
 */
void filter_brightness(Capture *capture) {
  int i;
  unsigned char *imageOut = NULL;
  
  imageOut = malloc(capture->size * sizeof(unsigned char));

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_brightness()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }
  
  memcpy(imageOut, capture->image, capture->size);

  for (i = 0; i <= capture->width * capture->height * 3; i++)
    if (imageOut[i] + capture->brightness >= 255)
      imageOut[i] = 255;
    else
     imageOut[i] += capture->brightness;

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Gamma adjustment, from 0 - 255
 */
void filter_gamma(Capture *capture) {
  unsigned int a;
  unsigned long int b = 0;
  unsigned char *imageOut = NULL;

  imageOut = malloc(capture->size * sizeof(unsigned char));
  
#ifdef	DEBUG
  fprintf(stderr, "Performing filter_gamma()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }
  
  memcpy(imageOut, capture->image, capture->size);
  
  for (a = 0; a < 256; a++)
    gamma_table[a] = (unsigned char)(pow((double)a/255.0f, (float)(capture->gamma/255)) * 255.0f);
    
  for (b = 0; b < capture->width * capture->height * 3; b++)
    *(imageOut + b) = gamma_table[(*(imageOut + b))];

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Basic sobel filter - broken?
 */
/*
void filter_sobel(Capture *capture) {
  int i;
  int grad;
  int deltaX, deltaY;
  int width;
  int temp;

  unsigned char *imageOut = NULL, *imageInG = NULL, *imageOutG = NULL;
  imageOut = malloc(capture->size * sizeof(unsigned char));

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

#ifdef  DEBUG
  fprintf(stderr, "Performing filter_sobel()...");
#endif

  width = capture->width*3;
  imageInG  = malloc((capture->size / 3) * sizeof(unsigned char));
  imageOutG = malloc((capture->size / 3) * sizeof(unsigned char));

  for (i = 0; i < capture->size; i = i + 3) {
    imageInG[i/3] = (capture->image[i]*0.3 + capture->image[i+1]*0.59 + capture->image[i+2]*0.11);
  }

  for (i = width/3; i < (capture->height-1) * width/3; i++) {
    deltaX = 2 * imageInG[i+1] + imageInG[i-width+1] + imageInG[i+width+1] - 2*imageInG[i-1] - imageInG[i-width-1] - imageInG[i+width-1];
    deltaY = imageInG[i-width-1] + 2*imageInG[i-width] + imageInG[i-width+1] - imageInG[i+width-1] - 2*imageInG[i+width] - imageInG[i+width+1];
    grad = (abs(deltaX) + abs(deltaY)) / 3;

    if (grad > 255)
      grad = 255;

    imageOutG[i] = 255-(unsigned char) grad;
  }
  for (i = 0; i < capture->size; i++) {
    // imageOut[i] = (imageIn[i] + imageOutG[i/3])/2;  <-- original

    temp = (int)(1.0*(float)capture->image[i] + 0.5*((float)imageOutG[i/3] - 128));
    if (temp > 255) temp = 255;
    if (temp < 0) temp = 0;
    imageOut[i] = (unsigned char)temp;
  }

  free(imageInG);
  free(imageOutG);

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);

#ifdef  DEBUG
  fprintf(stderr, "done.\n");
#endif
}
/**/

/**/
void filter_sobel(Capture *capture) {
  int i;
  int grad;
  int deltaX, deltaY;
  int width, height = capture->height;

  unsigned char *imageIn = capture->image, *imageOut = NULL;
  imageOut = malloc(capture->size * sizeof(unsigned char));
  
  if (capture->greyscale)
    width = capture->width;
  else
    width = capture->width*3;

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_sobel()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

  for (i = width; i < (height-1) * width; i++) {
    deltaX = 2 * imageIn[i+1] + imageIn[i-width+1] + imageIn[i+width+1] - 2*imageIn[i-1] - imageIn[i-width-1] - imageIn[i+width-1];
    deltaY = imageIn[i-width-1] + 2*imageIn[i-width] + imageIn[i-width+1] - imageIn[i+width-1] - 2*imageIn[i+width] - imageIn[i+width+1];
    grad = (abs(deltaX) + abs(deltaY)) / 3;

    if (grad > 255)
      grad = 255;

    imageOut[i] = (unsigned char) grad;
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}
/**/

/*
 * Laplace filter
 */
void filter_laplace(Capture *capture) {
  int i;
  int delta;

  unsigned char *imageOut = malloc(capture->size);

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_laplace()...");
#endif

  for (i = capture->width; i < (capture->height-1) * capture->width; i++) {
    delta  = abs(4 * capture->image[i] - capture->image[i-1] - capture->image[i+1] - capture->image[i-capture->width] - capture->image[i+capture->width]);

    if (delta > 255)
      imageOut[i] = 255;
    else
      imageOut[i] = (unsigned char)delta;
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Noise filter, value from 0 to 255
 */
void filter_noise(Capture *capture) {
  int i;
  int width = capture->width;
  int height = capture->height;
  int numberOfPixels = (int)(capture->size * (capture->noise/255));
  unsigned char *imageOut = malloc(capture->size);

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_noise()...");
#endif

  memcpy(imageOut, capture->image, capture->size);

  for (i = 0; i < numberOfPixels ; i += 2) {
    imageOut[rand() % capture->size]= 255;
    imageOut[rand() % capture->size]= 0;
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Negative filter
 */
void filter_negative(Capture *capture) {
  int i;
  unsigned char *imageOut = malloc(capture->size);

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_negative()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

  memcpy(imageOut, capture->image, capture->size);

  for (i = 0; i < capture->size; i++) {
    imageOut[i] = 255 - capture->image[i];
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);

#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif  
}

void filter_flip(Capture *capture) {
  int r, c;
  int height, width;
  char tmp;
  unsigned char *imageOut = malloc(capture->size);

#ifdef  DEBUG
  fprintf(stderr, "Performing filter_flip()...");
#endif
  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }
  
  width = capture->width * 3;
  height = capture->height;

  for (r = 0; r < height / 2; r++)
    for (c = 0; c < width; c++) {
      imageOut[c + width * r] = capture->image[width * (height - r - 1) + c];
      imageOut[width * (height - r - 1) + c] = capture->image[c + width * r];
    }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);

#ifdef  DEBUG
  fprintf(stderr, "done.\n");
#endif
}

void filter_mirror(Capture *capture) {
  int r, c;
  int height, width;
  unsigned char *imageOut = malloc(capture->size);
  
  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_mirror()...");
#endif

  width = capture->width * 3;
  height = capture->height;

  for (r = 0; r < capture->size; r += width)
    for (c = 0; c < width / 3; c++){
      if (capture->greyscale) {
        imageOut[c*2 + r + 0] = capture->image[(width - c*2 - 0) + r];
        imageOut[c*2 + r + 1] = capture->image[(width - c*2 - 1) + r];
        imageOut[c*2 + r + 2] = capture->image[(width - c*2 - 2) + r];
      } else {
        imageOut[c*3 + r + 0] = capture->image[(width - c*3 - 0) + r];
        imageOut[c*3 + r + 1] = capture->image[(width - c*3 - 1) + r];
        imageOut[c*3 + r + 2] = capture->image[(width - c*3 - 2) + r];
      }
    }

/*
  for (r = 0; r < height/2; r++)
    for (c = 0; c < width; c++) {
      imageOut[c+width*r] = imageIn[width*(height - r - 1) + c];
      imageOut[width*(height - r - 1) + c] = imageIn[c+width*r];
    }
*/
#ifdef  DEBUG
  fprintf(stderr, "Copying image to buffer\n");
#endif
  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

void filter_lowpass(Capture *capture) {
  int r, c;
  int width;
  unsigned char *imageOut = NULL;
  imageOut = malloc(capture->size * sizeof(unsigned char));

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_lowpass()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

  width = capture->width*3;

/*
  for (r = 0; r < capture->size; r += width)
    for (c = 0; c < width; c++)
      imageOut[c + r] = (imageIn[r-1 + c-1] + imageIn[r-1 + c] + imageIn[r-1 + c+1] + imageIn[r + c -1] + imageIn[r + c] + imageIn[r + c +1] + imageIn[r + 1 + c - 1] + imageIn[ r + 1 + c] + imageIn[r + 1 + c + 1]) / 9;
*/

  for (r = 0; r < capture->size; r += width){    
    for (c = 0; c < width; c++)
      if ((r==0)||(r==capture->size)||(c < 3)||(c > width-3))
        imageOut[c+r] = capture->image[c+r];
      else
        imageOut[c + r] = (capture->image[(r-3) + (c-3)] + capture->image[(r-3) + (c+0)] + capture->image[(r-3) + (c+3)] +
        	capture->image[(r+0) + (c-3)] + capture->image[(r+0) + (c+0)] + capture->image[(r+0) + (c+3)] + capture->image[(r+3) + (c-3)] + capture->image[(r+3) + (c+0)] + capture->image[(r+3) + (c+3)]) / 9;
  }
  
  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

void filter_highpass(Capture *capture) {
  int r, c;
  int width = capture->width;
  unsigned char *imageOut = malloc(capture->size);

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_highpass()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

  for (r = 0; r < capture->size; r += width)
    for (c = 0; c < width; c++)
      imageOut[c + r]= ( - capture->image[r-1 + c-1] - capture->image[r -1 + c] - capture->image[r - 1 + c + 1] -
      		capture->image[r + c -1] + 8 *( capture->image[r + c]) - capture->image[r + c +1] - capture->image[r + 1 + c - 1] -
      		capture->image[ r + 1 + c] - capture->image[r + 1 + c + 1]) /9;

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Fake the "infrared" look from the Predator movie
 */
void filter_predator(Capture *capture) {
  unsigned char *imageOut = malloc(capture->size);
  int x;

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_predator()...");
#endif

  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }
  
  memcpy(imageOut, capture->image, capture->size);

  while (x < capture->size) {
    if (imageOut[x] > imageOut[x + 1]) {
      if (imageOut[x] > imageOut[x + 2])
        imageOut[x + 1] = imageOut[x + 2] = 0;

      imageOut[x] = 255;
    } else if (imageOut[x + 1] > imageOut[x + 2]) {
      if (imageOut[x + 1] > imageOut[x])
        imageOut[x] = imageOut[x + 2] = 0;

      imageOut[x + 1] = 255;
    } else if (imageOut[x + 2] > imageOut[x]) {
      if (imageOut[x + 2] > imageOut[x + 1])
        imageOut[x + 1] = imageOut[x] = 0;

      imageOut[x + 2] = 255;
    } else {
      imageOut[x] = imageOut[x + 1] = imageOut[x + 2] = 0;
    }

    x += 3;
  }
    
  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * the light-check threshold.  Higher numbers remove more lights but blur the
 * image more.  30 is good for indoor lighting.
 */
#define NO_LIGHTS 30

/*
 * macros to make the code a little more readable, p=previous, n=next
 */
#define RED capture->image[i*3]
#define GREEN capture->image[i*3+1]
#define BLUE capture->image[i*3+2]
#define pRED capture->image[i*3-3]
#define pGREEN capture->image[i*3-2]
#define pBLUE capture->image[i*3-1]
#define nRED capture->image[i*3+3]
#define nGREEN capture->image[i*3+4]
#define nBLUE capture->image[i*3+5]

/*
 * Despeckle filter
 */
void filter_despeckle(Capture *capture) {
  long i;
  unsigned char *imageOut = malloc(capture->size);

#ifdef	DEBUG
  fprintf(stderr, "Performing filter_despeckle()...");
#endif
  
  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }

  for (i = 0; i < capture->size; i++) {
    if (i % capture->width == 0 || i % capture->width == capture->width - 1)
      memcpy(&imageOut[i*3], &capture->image[i*3], 3);
    else {
      if (RED - (GREEN+BLUE)/2 > NO_LIGHTS + ((pRED - (pGREEN+pBLUE)/2) + (nRED - (nGREEN+nBLUE)/2)))
        imageOut[i*3] = (pRED+nRED)/2;
      else
        imageOut[i*3] = RED;

      if (GREEN - (RED+BLUE)/2 > NO_LIGHTS + ((pGREEN - (pRED+pBLUE)/2) + (nGREEN - (nRED+nBLUE)/2)))
        imageOut[i*3+1] = (pGREEN+nGREEN)/2;
      else
        imageOut[i*3+1] = GREEN;

      if (BLUE - (GREEN+RED)/2 > NO_LIGHTS + ((pBLUE - (pGREEN+pRED)/2) + (nBLUE - (nGREEN+nRED)/2)))
        imageOut[i*3+2] = (pBLUE+nBLUE)/2;
      else
        imageOut[i*3+2] = BLUE;
    }
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Rotate image filter - NOT WORKING
 */
void filter_rotate(Capture *capture) {
  int x,y, newheight, newwidth, rp_x, rp_y;
  float im_r, im_theta, relx,rely,xyrad,xyor,xrot,yrot;
  unsigned char *imageIn = capture->image, *imageOut = NULL;
  float theta;
#ifdef	DEBUG
  fprintf(stderr, "Performing rotate_image()...");
#endif

  rp_x = capture->width/2;
  rp_y = capture->height/2;
  
  theta = capture->rotate*PI/180.0; // Convert theta into radians

  im_r     = sqrt((capture->height * capture->height) + (capture->width * capture->width));
  im_theta = atan2(capture->height, capture->width) + theta;
  newheight = ceil(im_r * sin (im_theta));
  newwidth  = ceil(im_r * cos (im_theta));
  
  imageOut = malloc(newheight * (newwidth * 3));
  memset(&imageOut, 0, capture->size);
  
  if (!imageOut) {
    fprintf(stderr,"imageOut alloc err\n");
    return;
  }
  
  for (x = 0; x < capture->width; x++){
    for (y = 0; y < capture->height; y++){
      relx = x - rp_x;
      rely = y - rp_y;
      xyrad = sqrt((relx * relx) + (rely * rely));
      
      if (relx == 0) {
        if (rely > 0) {
          xyor = PI / 2;
        } else {
          xyor = 3 * PI / 2;
        }
      } else {
        xyor = atan2(rely, relx);
      }
      
      xrot = xyrad * cos(xyor + theta) + rp_x;
      yrot = -xyrad * sin(xyor + theta) + rp_y;
      
      if ((((int)xrot * capture->width + (int)yrot + 0) > 0) && (((int)xrot * capture->width + (int)yrot + 2) < (newheight * newwidth * 3))){
        imageOut[(int)xrot*capture->width + (int)yrot + 0] = capture->image[x*3 + 0];
        imageOut[(int)xrot*capture->width + (int)yrot + 1] = capture->image[x*3 + 1];
        imageOut[(int)xrot*capture->width + (int)yrot + 2] = capture->image[x*3 + 2];
      }
    }
  }
  
  memcpy(capture->image, imageOut, newheight * (newwidth * 3));
  free(imageOut);
  
#ifdef	DEBUG
  fprintf(stderr, "done.\n");
#endif
}

/*
 * Default is to use the luminosity method. We could also use the average or
 * lightness methods here... they are listed below for reference.
 *
 * lightness
 *  newRGB = (max(RGB) + min(RGB)) / 2
 *
 * average
 *  newRGB = (R+G+B) / 2
 */
void filter_greyscale(Capture *capture) {
  /*
   * This is the formula The GIMP uses to convert to
   * greyscale, so I'll use it here...
   * newRGB = 0.21R + 0.71G + 0.07B
   */
  int i;
  unsigned char *imageOut = NULL;
  imageOut = malloc(capture->size);

  for (i = 0; i < capture->size; i += 3) {
    int l = (0.21 * capture->image[i + 0] + 0.71 * capture->image[i+1] + 0.07 * capture->image[i+2]);
    imageOut[i + 0] = l;
    imageOut[i + 1] = l;
    imageOut[i + 2] = l;
  }

  memcpy(capture->image, imageOut, capture->size);
  free(imageOut);
}
#endif	/* ENABLE_FILTERS */
