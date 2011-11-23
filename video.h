/*
 * clcam, video.h
 * Copyright (C) 2011
 *	Shaun Landis
 *
 * Please see the file COPYING for license information
 */

void deviceOpen(Capture *);
void deviceInit(Capture *);
void captureStart(Capture *);
void captureStop(Capture *);
void deviceUninit(Capture *);
void deviceClose(Capture *);
