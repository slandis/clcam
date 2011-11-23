/*
 * clcam, save.h
 * Copyright (C) 2003-2011
 *	Shaun Landis
 *	Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

enum Save_Types {
	PPM 	= 0,
	PNG 	= 1,
	JPEG 	= 2
};

enum Ppm_Types {
	RAW 	= 0,
	ASCII 	= 1
};

#ifdef	ENABLE_JPEG
void jpeg_save(Capture *);
#endif

#ifdef	ENABLE_PNG
void png_save(Capture *);
#endif

void ppm_save(Capture *);
