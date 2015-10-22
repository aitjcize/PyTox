/**
 * @file   av.h
 * @author Wei-Ning Huang (AZ) <aitjcize@gmail.com>
 *
 * Copyright (C) 2013 - 2014  Wei-Ning Huang (AZ) <aitjcize@gmail.com>
 * All Rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __AV_H__
#define __AV_H__

#include <Python.h>
#include <tox/toxav.h>

#include <vpx/vpx_image.h>
typedef enum {
    av_TypeAudio = 192,
    av_TypeVideo
} ToxAvCallType;

typedef struct _ToxAvCSettings {
    ToxAvCallType call_type;

    uint32_t video_bitrate; /* In kbits/s */
    uint16_t max_video_width; /* In px */
    uint16_t max_video_height; /* In px */

    uint32_t audio_bitrate; /* In bits/s */
    uint16_t audio_frame_duration; /* In ms */
    uint32_t audio_sample_rate; /* In Hz */
    uint32_t audio_channels;
} ToxAvCSettings;

extern const ToxAvCSettings av_DefaultSettings;

/* ToxAV definition */
typedef struct {
    PyObject_HEAD
    PyObject *core;
    ToxAV *av;
    vpx_image_t* in_image;
    uint32_t i_w, i_h;
    unsigned char* out_image;
    uint32_t o_w, o_h;
    ToxAvCSettings cs;
} ToxAVCore;

void ToxAVCore_install_dict(void);

#endif /* __AV_H__ */
