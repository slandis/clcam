/*
 * clcam, video.h
 * Copyright (C) 2011
 *	Shaun Landis
 *
 * Please see the file COPYING for license information
 */

void device_open(Capture *);
void device_init(Capture *);
void capture_start(Capture *);
void capture_stop(Capture *);
void device_uninit(Capture *);
void device_close(Capture *);

void enum_controls(Capture *);
void walk_controls(Capture *);
void reset_controls(Capture *);

struct v4l2_control c_chain[32];
