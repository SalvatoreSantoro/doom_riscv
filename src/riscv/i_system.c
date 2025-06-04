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
#include <time.h>
#include <wchar.h>

#include "../doomdef.h"
#include "SDL_events.h"
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
    struct timespec ts;

    /* TIC_RATE is 35 in theory */
    uint16_t vt_now = clock() / (1000 * TICRATE);

    if (vt_now < vt_last)
        vt_base += 65536;
    vt_last = vt_now;

    return (vt_base + vt_now);
}

static void I_GetRemoteEvents(void) {
    int idx = eventhead;
    int key;
    register SDL_Event *a0 asm("a0") = sdl_evts;
    register int *a1 asm("a1") = &eventhead;
    register int a2 asm("a2") = eventtail;
    register long syscall_id asm("a7") = SDL_PULL_EVENTS;
    asm volatile("ecall" : "+r"(a0), "+r"(a1) : "r"(a2), "r"(syscall_id) : "memory");
    // map events from sdl to internal doom events queue
    while (idx != eventhead) {
        if ((sdl_evts[idx].type == SDL_KEYDOWN) || (sdl_evts[idx].type == SDL_KEYUP)) {
            switch (sdl_evts[idx].key.keysym.scancode) {
            case SDL_SCANCODE_RIGHT:
                key = KEY_RIGHTARROW;
                break;
            case SDL_SCANCODE_LEFT:
                key = KEY_LEFTARROW;
                break;
            case SDL_SCANCODE_UP:
                key = KEY_UPARROW;
                break;
            case SDL_SCANCODE_DOWN:
                key = KEY_DOWNARROW;
                break;
            case SDL_SCANCODE_ESCAPE:
                key = KEY_ESCAPE;
                break;
            case SDL_SCANCODE_RETURN:
                key = KEY_ENTER;
                break;
            case SDL_SCANCODE_TAB:
                key = KEY_TAB;
                break;

            case SDL_SCANCODE_F1:
                key = KEY_F1;
                break;
            case SDL_SCANCODE_F2:
                key = KEY_F2;
                break;
            case SDL_SCANCODE_F3:
                key = KEY_F3;
                break;
            case SDL_SCANCODE_F4:
                key = KEY_F4;
                break;
            case SDL_SCANCODE_F5:
                key = KEY_F5;
                break;
            case SDL_SCANCODE_F6:
                key = KEY_F6;
                break;
            case SDL_SCANCODE_F7:
                key = KEY_F7;
                break;
            case SDL_SCANCODE_F8:
                key = KEY_F8;
                break;
            case SDL_SCANCODE_F9:
                key = KEY_F9;
                break;
            case SDL_SCANCODE_F10:
                key = KEY_F10;
                break;
            case SDL_SCANCODE_F11:
                key = KEY_F11;
                break;
            case SDL_SCANCODE_F12:
                key = KEY_F12;
                break;

            case SDL_SCANCODE_BACKSPACE:
                key = KEY_BACKSPACE;
                break;
            case SDL_SCANCODE_PAUSE:
                key = KEY_PAUSE;
                break;

            case SDL_SCANCODE_EQUALS:
                key = KEY_EQUALS;
                break;
            case SDL_SCANCODE_MINUS:
                key = KEY_MINUS;
                break;

            case SDL_SCANCODE_RSHIFT:
                key = KEY_RSHIFT;
                break;
            case SDL_SCANCODE_RCTRL:
                key = KEY_RCTRL;
                break;
            case SDL_SCANCODE_RALT:
                key = KEY_RALT;
                break;

            case SDL_SCANCODE_LALT:
                key = KEY_LALT;
                break;

            // use the ascii
            default:
                key = sdl_evts[idx].key.keysym.sym;
                break;
            }
        }
        switch (sdl_evts[idx].type) {
        case SDL_KEYDOWN:
            events[idx].type = ev_keydown;
            events[idx].data1 = key;
            break;
        case SDL_KEYUP:
            events[idx].type = ev_keyup;
            events[idx].data1 = key;
            break;
        case SDL_MOUSEBUTTONDOWN:
            events[idx].type = ev_mouse;
            events[idx].data1 = sdl_evts[idx].button.button;
            events[idx].data2 = 0;
            events[idx].data3 = 0;
            break;
        case SDL_MOUSEBUTTONUP:
            events[idx].type = ev_mouse;
            events[idx].data1 = 2;
            events[idx].data2 = 0;
            events[idx].data3 = 0;
            break;
        case SDL_MOUSEMOTION:
            events[idx].type = ev_mouse;
            events[idx].data1 = sdl_evts[idx].button.button;
            events[idx].data2 = sdl_evts[idx].motion.xrel << 2;
            events[idx].data3 = -sdl_evts[idx].motion.yrel << 2;
            break;
        default:
            break;
        }
        idx = ((idx + 1) & (MAXEVENTS - 1));
    }
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
