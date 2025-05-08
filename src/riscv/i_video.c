/*
 * i_video.c
 *
 * Video system support code
 *
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

#include <stdint.h>
#include <string.h>

#include "../doomdef.h"

#include "../../../common/sdl_syscalls.h"
#include "../d_event.h"
#include "i_system.h"
#include "i_video.h"
#include "v_video.h"

void I_InitGraphics(void) {
    /* Don't need to do anything really ... */

    /* Ok, maybe just set gamma default */
    usegamma = 1;

    const char *name = "DOOM";
    register const char *a0 asm("a0") = name;
    register int a1 asm("a1") = SCREENWIDTH;
    register int a2 asm("a2") = SCREENHEIGHT;
    register size_t a3 asm("a3") = MAXEVENTS;
    register long syscall_id asm("a7") = SDL_INIT;

    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(a2), "r"(a3), "r"(syscall_id) : "memory");
}

void I_ShutdownGraphics(void) {
    /* Don't need to do anything really ... */
    register int a0 asm("a0");
    register long syscall_id asm("a7") = SDL_SHUTDOWN;
    asm volatile("ecall" : "+r"(a0) : "r"(syscall_id));
}

void I_SetPalette(byte *palette) {
    uint32_t buffer[256];
    byte r, g, b;

    for (int i = 0; i < 256; i++) {
        r = gammatable[usegamma][*palette++];
        g = gammatable[usegamma][*palette++];
        b = gammatable[usegamma][*palette++];
        buffer[i] = ((uint32_t) r << 16) | ((uint32_t) g << 8) | ((uint32_t) b);
    }

    register uint32_t *a0 asm("a0") = buffer;
    register size_t a1 asm("a1") = 256;
    register long syscall_id asm("a7") = SDL_WRITE_PALETTE;
    asm volatile("ecall" : "+r"(a0) : "r"(a1), "r"(syscall_id) : "memory");
}

void I_UpdateNoBlit(void) {
}

void I_FinishUpdate(void) {
    /* Copy from RAM buffer to frame buffer */
    register long syscall_id asm("a7") = SDL_WRITE_FB;
    register byte *screen asm("a0") = screens[0];

    asm volatile("ecall" : "+r"(screen) : "r"(syscall_id) : "memory");

    /* Very crude FPS measure (time to render 100 frames */
#if 1
    static int frame_cnt = 0;
    static int tick_prev = 0;

    if (++frame_cnt == 100) {
        int tick_now = I_GetTime();
        printf("%d\n", tick_now - tick_prev);
        tick_prev = tick_now;
        frame_cnt = 0;
    }
#endif
}

void I_WaitVBL(int count) {
    /* Buys-Wait for VBL status bit */
    /* static volatile uint32_t* const video_state = (void*)(VID_CTRL_BASE); */
    /* while (!(video_state[0] & (1 << 16))) */
    /*     ; */
}

void I_ReadScreen(byte *scr) {
    /* FIXME: Would have though reading from VID_FB_BASE be better ...
     *        but it seems buggy. Not sure if the problem is in the
     *        gateware
     */
    memcpy(scr, screens[0], SCREENHEIGHT * SCREENWIDTH);
}

#if 0 /* WTF ? Not used ... */
void
I_BeginRead(void)
{
}

void
I_EndRead(void)
{
}
#endif
