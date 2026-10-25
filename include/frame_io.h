/* SPDX-License-Identifier: MIT */
#ifndef FRAME_IO_H
#define FRAME_IO_H
#include "demo.h"
#include <stdio.h>

/* Strict unquoted CSV: frame,background,s0,...,s31. 1=success, 0=EOF, -1=error. */
int frame_read_header(FILE *stream);
int frame_read(FILE *stream, unsigned *id, demo_frame *frame);
int frame_write_header(FILE *stream);
int frame_write(FILE *stream, unsigned id, const demo_frame *frame);
#endif
