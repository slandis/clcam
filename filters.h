/*
 * clcam, filters.h
 * Copyright (C) 2003-2011
 *	Shaun Landis
 *	Romesh Kumbhani
 *
 * Please see the file COPYING for license information
 */

#ifdef	ENABLE_FILTERS
#define MAX_FILTERS 10

#define PI 3.14159265

void filter_brightness(Capture *);
void filter_gamma(Capture *);
void filter_sobel(Capture *);
void filter_laplace(Capture *);
void filter_noise(Capture *);
void filter_negative(Capture *);
void filter_flip(Capture *);
void filter_mirror(Capture *);
void filter_lowpass(Capture *);
void filter_highpass(Capture *);
void filter_predator(Capture *);
void filter_despeckle(Capture *);
void filter_rotate(Capture *);
void filter_text(Capture *);
void filter_greyscale(Capture *);

void walk_filters(Capture *);

static void add_text(unsigned char *, int, int);

unsigned char gamma_table[256];

typedef void (*pfl)(Capture *);
pfl f_chain[MAX_FILTERS];
#endif	/* ENABLE_FILTERS */
