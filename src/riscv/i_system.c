/*
 * i_system.c
 *
 * System support code
 *
 * Copyright (C) 1993-1996 by id Software, Inc.
 * Copyright (C) 2021 Sylvain Munaut
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <wchar.h>

#include "../../../common/sdl_syscalls.h"
#include "../doomdef.h"
#include "doomstat.h"

#include "../d_event.h"
#include "d_main.h"
#include "g_game.h"
#include "i_sound.h"
#include "i_video.h"
#include "m_misc.h"

#include "i_system.h"

/* Video controller, used as a time base */
/* Normally running at 70 Hz, although in 640x480 compat
 * mode, it's 60 Hz so our tick is 15% too slow ... */

/* Video Ticks tracking */
static uint16_t vt_last = 0;
static uint32_t vt_base = 0;

void I_Init(void) {
    // vt_last = video_state[0] & 0xffff;
}

byte *I_ZoneBase(int *size) {
    /* Give 6M to DOOM */
    *size = 6 * 1024 * 1024;
    return (byte *) malloc(*size);
}

int I_GetTime(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);

    /* TIC_RATE is 35 in theory */
    uint16_t vt_now = ((((long long) tv.tv_sec) * 1000) + (tv.tv_usec / 1000)) / TICRATE;

    if (vt_now < vt_last)
        vt_base += 65536;
    vt_last = vt_now;

    return (vt_base + vt_now);
}

static void I_GetRemoteEvents(void) {
    register event_t *a0 asm("a0") = events;
    register int *a1 asm("a1") = &eventhead;
    register int a2 asm("a2") = eventtail;
    register long syscall_id asm("a7") = SDL_PULL_EVENTS;
    asm volatile("ecall" : "+r"(a0), "+r"(a1): "r"(a2), "r"(syscall_id) : "memory");
}

void I_StartFrame(void) {
    /* Nothing to do */
}

void I_StartTic(void) {
    I_GetRemoteEvents();
}

ticcmd_t *I_BaseTiccmd(void) {
    static ticcmd_t emptycmd;
    return &emptycmd;
}

void I_Quit(void) {
    D_QuitNetGame();
    M_SaveDefaults();
    I_ShutdownGraphics();
    exit(0);
}

byte *I_AllocLow(int length) {
    byte *mem;
    mem = (byte *) malloc(length);
    memset(mem, 0, length);
    return mem;
}

void I_Tactile(int on, int off, int total) {
    // UNUSED.
    on = off = total = 0;
}

void I_Error(char *error, ...) {
    va_list argptr;

    // Message first.
    va_start(argptr, error);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, error, argptr);
    fprintf(stderr, "\n");
    va_end(argptr);

    fflush(stderr);

    // Shutdown. Here might be other errors.
    if (demorecording)
        G_CheckDemoStatus();

    D_QuitNetGame();
    I_ShutdownGraphics();

    exit(-1);
}
