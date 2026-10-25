/* SPDX-License-Identifier: MIT */
#include "frame_io.h"
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static int line_read(FILE *stream, char *line, size_t size)
{
    size_t len = 0;
    int ch;
    while ((ch = fgetc(stream)) != EOF && ch != '\n') {
        if (ch == '\0' || len == size - 1) return -1;
        line[len++] = (char)ch;
    }
    if (ferror(stream)) return -1;
    if (ch == EOF && !len) return 0;
    if (len && line[len - 1] == '\r') --len;
    line[len] = '\0';
    return 1;
}

static void header(char *text, size_t capacity)
{
    size_t used = (size_t)snprintf(text, capacity, "frame,background");
    for (unsigned i = 0; i < DEMO_SAMPLE_COUNT; ++i)
        used += (size_t)snprintf(text + used, capacity - used, ",s%u", i);
}

int frame_write_header(FILE *stream)
{
    char text[256];
    header(text, sizeof text);
    return fprintf(stream, "%s\n", text) >= 0;
}

int frame_read_header(FILE *stream)
{
    char expected[256], line[4096];
    header(expected, sizeof expected);
    return line_read(stream, line, sizeof line) == 1 && strcmp(expected, line) == 0;
}

int frame_write(FILE *stream, unsigned id, const demo_frame *frame)
{
    if (fprintf(stream, "%u,%.17g", id, frame->background) < 0) return 0;
    for (unsigned i = 0; i < DEMO_SAMPLE_COUNT; ++i)
        if (fprintf(stream, ",%.17g", frame->samples[i]) < 0) return 0;
    return fputc('\n', stream) != EOF;
}

int frame_read(FILE *stream, unsigned *id, demo_frame *frame)
{
    char line[4096], *end;
    int status = line_read(stream, line, sizeof line);
    if (status != 1) return status;
    if (line[0] < '0' || line[0] > '9') return -1;
    errno = 0;
    unsigned long number = strtoul(line, &end, 10);
    if (errno || number > UINT_MAX || *end != ',') return -1;
    char *cursor = end + 1;
    demo_frame candidate;
    for (unsigned i = 0; i <= DEMO_SAMPLE_COUNT; ++i) {
        errno = 0;
        double value = strtod(cursor, &end);
        if (end == cursor || errno || !isfinite(value)) return -1;
        if ((i < DEMO_SAMPLE_COUNT && *end != ',') ||
            (i == DEMO_SAMPLE_COUNT && *end != '\0')) return -1;
        if (i == 0) candidate.background = value;
        else candidate.samples[i - 1] = value;
        cursor = end + 1;
    }
    *id = (unsigned)number;
    *frame = candidate;
    return 1;
}
