/*
 * clcam, save.c
 * Copyright (C) 2003-2011
 *  Shaun Landis
 *  Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/videodev2.h>

#ifdef  ENABLE_JPEG
#include <jpeglib.h>
#endif

#include "clcam.h"
#include "save.h"

#ifdef  ENABLE_PNG
void png_save(Capture *capture) {
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 k, height, width;
  png_bytep row_pointers[capture->height];
          
  /* open the file */
  if(!strcmp(capture->filename, "-"))
    fp = stdout;
  else if(capture->filename != NULL)
    fp = fopen(capture->filename, "wb");
                        
  if (fp == NULL){
    fprintf(stderr, "Error - Could not open file");
    return;
  }

  /* Create and initialize the png_struct with the desired error handler
   * functions.  If you want to use the default stderr and longjump method,
   * you can supply NULL for the last three parameters.  We also check that
   * the library version is compatible with the one used at compile time,
   * in case we are using dynamically linked libraries.  REQUIRED.
   */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
                   
  if (png_ptr == NULL) {
    fprintf(stderr, "Error - could not create PNG");
    fclose(fp);
    return;
  }

  /* Allocate/initialize the image information data.  REQUIRED */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(fp);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return;
  }

  /* Set error handling.  REQUIRED if you aren't supplying your own
   * error hadnling functions in the png_create_write_struct() call.
   */
  if (setjmp(png_ptr->jmpbuf)) {
    /* If we get here, we had a problem reading the file */
    fclose(fp);
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return;
  }

  /* One of the following I/O initialization functions is REQUIRED */
  /* set up the output control if you are using standard C streams */
  png_init_io(png_ptr, fp);

  /* Set the image information here.  Width and height are up to 2^31,
   * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
   * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
   * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
   * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
   * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
   * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
   */

  if(capture->greyscale)
    png_set_IHDR(png_ptr, info_ptr, capture->width, capture->height, 8, PNG_COLOR_TYPE_GRAY, capture->save.interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  else
    png_set_IHDR(png_ptr, info_ptr, capture->width, capture->height, 8, PNG_COLOR_TYPE_RGB, capture->save.interlace, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  png_set_compression_level(png_ptr, capture->save.compression);

  /* set the palette if there is one.  REQUIRED for indexed-color images */
  //palette = (png_colorp)png_malloc(png_ptr, 256 * sizeof (png_color));
  /* ... set palette colors ... */
  //png_set_PLTE(png_ptr, info_ptr, palette, 256);

  /* optional significant bit chunk */
  /* if we are dealing with a grayscale image then */
  //sig_bit.gray = true_bit_depth;
  /* otherwise, if we are dealing with a color image then */
  //sig_bit.red = true_red_bit_depth;
  //sig_bit.green = true_green_bit_depth;
  //sig_bit.blue = true_blue_bit_depth;
  /* if the image has an alpha channel then */
  //sig_bit.alpha = true_alpha_bit_depth;
  //png_set_sBIT(png_ptr, info_ptr, sig_bit);

  /* Optional gamma chunk is strongly suggested if you have any guess
   * as to the correct gamma of the image.
   */
  //png_set_gAMA(png_ptr, info_ptr, gamma);

  /* Optionally write comments into the image */
  /*
  text_ptr[0].key = "Title";
  text_ptr[0].text = "Mona Lisa";
  text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
  text_ptr[1].key = "Author";
  text_ptr[1].text = "Leonardo DaVinci";
  text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
  text_ptr[2].key = "Description";
  text_ptr[2].text = "<long text>";
  text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
  png_set_text(png_ptr, info_ptr, text_ptr, 3);
   */
  /* other optional chunks like cHRM, bKGD, tRNS, tIME, oFFs, pHYs, */
  /* note that if sRGB is present the cHRM chunk must be ignored
   * on read and must be written in accordance with the sRGB profile */

  /* Write the file header information.  REQUIRED */
  png_write_info(png_ptr, info_ptr);
  
  /* Once we write out the header, the compression type on the text
   * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
   * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
   * at the end.
   */

  /* set up the transformations you want.  Note that these are
   * all optional.  Only call them if you want them.
   */
                        
  /* invert monocrome pixels */
  //png_set_invert_mono(png_ptr);

  /* Shift the pixels up to a legal bit depth and fill in
   * as appropriate to correctly scale the image.
   */
  //png_set_shift(png_ptr, &sig_bit);

  /* pack pixels into bytes */
  //png_set_packing(png_ptr);

  /* swap location of alpha bytes from ARGB to RGBA */
  //png_set_swap_alpha(png_ptr);
    
  /* Get rid of filler (OR ALPHA) bytes, pack XRGB/RGBX/ARGB/RGBA into
   * RGB (4 channels -> 3 channels). The second parameter is not used.
   */
  //png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);

  /* flip BGR pixels to RGB */
  //png_set_bgr(png_ptr);

  /* swap bytes of 16-bit files to most significant byte first */
  //png_set_swap(png_ptr);

  /* swap bits of 1, 2, 4 bit packed pixel formats */
  //png_set_packswap(png_ptr);

  /* turn on interlace handling if you are not using png_write_image() */
  /*
  if (interlacing)
    number_passes = png_set_interlace_handling(png_ptr);
  else
    number_passes = 1;
  */
  /* The easiest way to write the image (you may have a different memory
   * layout, however, so choose what fits your needs best).  You need to
   * use the first method if you aren't handling interlacing yourself.
   */
  /*
  png_uint_32 k, height, width;
  png_byte image[height][width];
  png_bytep row_pointers[height];
  */

  if (capture->greyscale) {
    for (k = 0; k < capture->height; k++)
      row_pointers[k] = capture->image + k * capture->width;
  } else {
    for (k = 0; k < capture->height; k++)
      row_pointers[k] = capture->image + k * capture->width * 3;
  }

  /* One of the following output methods is REQUIRED */
  png_write_image(png_ptr, row_pointers);

  /* You can write optional chunks like tEXt, zTXt, and tIME at the end
   * as well.
   */

  /* It is REQUIRED to call this to finish writing the rest of the file */
  png_write_end(png_ptr, info_ptr);

  /* if you malloced the palette, free it here */
  free(info_ptr->palette);

  /* if you allocated any text comments, free them here */

  /* clean up after the write, and free any memory allocated */
  png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

  /* close the file */
  fclose(fp);

  /* that's it */

  return;
}
#endif  /* ENABLE_PNG */

void ppm_save(Capture *capture) {
  FILE *fp;
  unsigned char buff[3];
  int i;
      
  if(!strcmp(capture->filename, "-"))
    fp = stdout;
  else if(capture->filename != NULL)
    fp = fopen(capture->filename, "wb");
                  
  if (fp == NULL){
    fprintf(stderr, "Could not open file");
    return;
  }

  /*  Raw  */
  if (capture->save.format == RAW){
    fprintf(fp, "P6\n%d %d\n%d\n", capture->width, capture->height, 255);
  
    if (capture->greyscale){
      for (i = 0; i < capture->width * capture->height; i++) {
        buff[0] = capture->image[i];
        buff[1] = capture->image[i];
        buff[2] = capture->image[i];
        fwrite (buff, 1, 3, fp);
      }
    } else {
      for (i = 0; i < capture->width * capture->height * 3; i++) {
        fputc(capture->image[i], fp);
      }
    }
  } else if (capture->save.format == ASCII) {
    fprintf(fp, "P3\n%d %d\n%d\n", capture->width, capture->height, 255);
    
    if (capture->greyscale) {
      for (i = 0; i < capture->width * capture->height; i++) {
        fprintf(fp, "%03d %03d %03d\n", capture->image[i], capture->image[i], capture->image[i]);
      }
    } else
      for (i = 0; i < capture->width * capture->height * 3; i = i + 3) {
        fprintf(fp, "%03d %03d %03d\n", capture->image[i], capture->image[i+1], capture->image[i+2]);
      }
  }
  
  fclose(fp);
}

#ifdef  ENABLE_JPEG
void jpeg_save(Capture *capture) {
  FILE *fp;
//  int ls=0, x, y;
//  int i;
//  int lines, pixels_per_line;
//  unsigned char buff[3];
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  JSAMPROW row_pointer[1];     // pointer to JSAMPLE row(s)

  if(!strcmp(capture->filename, "-"))
    fp = stdout;
  else if(capture->filename != NULL)
    fp = fopen(capture->filename, "wb");

  if (fp == NULL){
    fprintf(stderr, "Could not open file");
    return;
  }

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, fp);

  cinfo.image_width = capture->width;
  cinfo.image_height = capture->height;

  if (capture->greyscale) {
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;
  } else {
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
  }

//  cinfo.smoothing_factor = capture->save.smoothness;

//  if (capture->save.smoothness) {
//    cinfo.optimize_coding = TRUE;
//  }

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, capture->save.quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = &capture->image[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
    jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  fclose(fp);
  return;
}
#endif  /* ENABLE_JPEG */
