/**
 * @file   av.c
 * @author Wei-Ning Huang (AZ) <aitjcize@gmail.com>
 *
 * Copyright (C) 2013 - 2014  Wei-Ning Huang (AZ) <aitjcize@gmail.com>
 * All Rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include <Python.h>
#include <tox/toxav.h>

#include "av.h"
#include "core.h"
#include "util.h"

extern PyObject* ToxOpError;

#define AUDIO_FRAME_SIZE (av_DefaultSettings.audio_sample_rate * av_DefaultSettings.audio_frame_duration / 1000)

#define CALLBACK_DEF(name)                                          \
  void ToxAV_callback_##name(void* agent, int32_t idx, void* self)  \
  {                                                                 \
    PyObject_CallMethod((PyObject*)self, #name, "i", idx);          \
  }

CALLBACK_DEF(on_invite);
CALLBACK_DEF(on_start);
CALLBACK_DEF(on_cancel);
CALLBACK_DEF(on_reject);
CALLBACK_DEF(on_end);
CALLBACK_DEF(on_ringing);
CALLBACK_DEF(on_starting);
CALLBACK_DEF(on_ending);
CALLBACK_DEF(on_request_timeout);
CALLBACK_DEF(on_peer_timeout);
CALLBACK_DEF(on_media_change);

static void i420_to_rgb(vpx_image_t *img, unsigned char *out)
{
    const int w = img->d_w;
    const int w2 = w / 2;
    const int pstride = w * 3;
    const int h = img->d_h;
    const int h2 = h / 2;

    const int strideY = img->stride[0];
    const int strideU = img->stride[1];
    const int strideV = img->stride[2];
    int posy, posx;

    for (posy = 0; posy < h2; posy++) {
        unsigned char *dst = out + pstride * (posy * 2);
        unsigned char *dst2 = out + pstride * (posy * 2 + 1);
        const unsigned char *srcY = img->planes[0] + strideY * posy * 2;
        const unsigned char *srcY2 = img->planes[0] + strideY * (posy * 2 + 1);
        const unsigned char *srcU = img->planes[1] + strideU * posy;
        const unsigned char *srcV = img->planes[2] + strideV * posy;

        for (posx = 0; posx < w2; posx++) {
            unsigned char Y, U, V;
            short R, G, B;
            short iR, iG, iB;

            U = *(srcU++);
            V = *(srcV++);
            iR = (351 * (V - 128)) / 256;
            iG = - (179 * (V - 128)) / 256 - (86 * (U - 128)) / 256;
            iB = (444 * (U - 128)) / 256;

            Y = *(srcY++);
            R = Y + iR ;
            G = Y + iG ;
            B = Y + iB ;
            R = (R < 0 ? 0 : (R > 255 ? 255 : R));
            G = (G < 0 ? 0 : (G > 255 ? 255 : G));
            B = (B < 0 ? 0 : (B > 255 ? 255 : B));
            *(dst++) = R;
            *(dst++) = G;
            *(dst++) = B;

            Y = *(srcY2++);
            R = Y + iR ;
            G = Y + iG ;
            B = Y + iB ;
            R = (R < 0 ? 0 : (R > 255 ? 255 : R));
            G = (G < 0 ? 0 : (G > 255 ? 255 : G));
            B = (B < 0 ? 0 : (B > 255 ? 255 : B));
            *(dst2++) = R;
            *(dst2++) = G;
            *(dst2++) = B;

            Y = *(srcY++) ;
            R = Y + iR ;
            G = Y + iG ;
            B = Y + iB ;
            R = (R < 0 ? 0 : (R > 255 ? 255 : R));
            G = (G < 0 ? 0 : (G > 255 ? 255 : G));
            B = (B < 0 ? 0 : (B > 255 ? 255 : B));
            *(dst++) = R;
            *(dst++) = G;
            *(dst++) = B;

            Y = *(srcY2++);
            R = Y + iR ;
            G = Y + iG ;
            B = Y + iB ;
            R = (R < 0 ? 0 : (R > 255 ? 255 : R));
            G = (G < 0 ? 0 : (G > 255 ? 255 : G));
            B = (B < 0 ? 0 : (B > 255 ? 255 : B));
            *(dst2++) = R;
            *(dst2++) = G;
            *(dst2++) = B;
        }
    }
}

static void rgb_to_i420(unsigned char* rgb, vpx_image_t *img)
{
  int upos = 0;
  int vpos = 0;
  int x = 0, i = 0;
  int line = 0;

  for (line = 0; line < img->d_h; ++line) {
    if (!(line % 2)) {
      for (x = 0; x < img->d_w; x += 2) {
        uint8_t r = rgb[3 * i];
        uint8_t g = rgb[3 * i + 1];
        uint8_t b = rgb[3 * i + 2];

        img->planes[VPX_PLANE_Y][i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
        img->planes[VPX_PLANE_U][upos++] = ((-38*r + -74*g + 112*b) >> 8) + 128;
        img->planes[VPX_PLANE_V][vpos++] = ((112*r + -94*g + -18*b) >> 8) + 128;

        r = rgb[3 * i];
        g = rgb[3 * i + 1];
        b = rgb[3 * i + 2];

        img->planes[VPX_PLANE_Y][i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
      }
    } else {
      for (x = 0; x < img->d_w; x += 1) {
        uint8_t r = rgb[3 * i];
        uint8_t g = rgb[3 * i + 1];
        uint8_t b = rgb[3 * i + 2];

        img->planes[VPX_PLANE_Y][i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
      }
    }
  }
}

void ToxAV_audio_recv_callback(ToxAv* av, int32_t call_idx, int16_t* data,
    int size, void* self)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject_CallMethod((PyObject*)self, "on_audio_data", "iis#", call_idx, size,
      (char*)data, size << 1);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }

  PyGILState_Release(gstate);
}

void ToxAV_video_recv_callback(ToxAv* av, int32_t call_idx, vpx_image_t* image,
    void* user)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  ToxAV* self = (ToxAV*)user;

  if (image == NULL) {
    return;
  }

  if (self->out_image && (self->o_w != image->d_w || self->o_h != image->d_h)) {
    free(self->out_image);
    self->out_image = NULL;
  }

  const int buf_size = image->d_w * image->d_h * 3;

  if (self->out_image == NULL) {
    self->o_w = image->d_w;
    self->o_h = image->d_h;
    self->out_image = malloc(buf_size);
  }

  i420_to_rgb(image, self->out_image);

  vpx_img_free(image);

  PyObject_CallMethod((PyObject*)self, "on_video_data", "iiis#", call_idx,
      self->o_w, self->o_h, self->out_image);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }

  PyGILState_Release(gstate);
}


static int init_helper(ToxAV* self, PyObject* args)
{
  PyEval_InitThreads();

  if (self->av) {
    toxav_kill(self->av);
    self->av = NULL;
  }

  PyObject* core = NULL;
  int max_calls;

  if (!PyArg_ParseTuple(args, "Oi", &core, &max_calls)) {
    PyErr_SetString(PyExc_TypeError, "must associate with a core instance");
    return -1;
  }

  self->core = core;
  Py_INCREF(self->core);

  self->av = toxav_new(((ToxCore*)self->core)->tox, max_calls);

  self->cs = av_DefaultSettings;
  self->cs.max_video_width = self->cs.max_video_height = 0;

#define REG_CALLBACK(id, name) \
  toxav_register_callstate_callback(self->av, ToxAV_callback_##name, id, self)

  REG_CALLBACK(av_OnInvite, on_invite);
  REG_CALLBACK(av_OnStart, on_start);
  REG_CALLBACK(av_OnCancel, on_cancel);
  REG_CALLBACK(av_OnReject, on_reject);
  REG_CALLBACK(av_OnEnd, on_end);

  REG_CALLBACK(av_OnRinging, on_ringing);
  REG_CALLBACK(av_OnStarting, on_starting);
  REG_CALLBACK(av_OnEnding, on_ending);

  REG_CALLBACK(av_OnRequestTimeout, on_request_timeout);
  REG_CALLBACK(av_OnPeerTimeout, on_peer_timeout);
  REG_CALLBACK(av_OnMediaChange, on_media_change);

#undef REG_CALLBACK

  toxav_register_audio_recv_callback(self->av, ToxAV_audio_recv_callback, self);
  toxav_register_video_recv_callback(self->av, ToxAV_video_recv_callback, self);

  if (self->in_image == NULL) {
    self->in_image = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, 640, 480, 1);
  }

  if (self->av == NULL) {
    PyErr_SetString(ToxOpError, "failed to allocate toxav");
    return -1;
  }

  return 0;
}

static PyObject*
ToxAV_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
  ToxAV* self = (ToxAV*)type->tp_alloc(type, 0);
  self->av = NULL;
  self->in_image = NULL;
  self->out_image = NULL;

  if (init_helper(self, args) == -1) {
    return NULL;
  }

  return (PyObject*)self;
}

static int
ToxAV_init(ToxAV* self, PyObject* args, PyObject* kwds)
{
  // since __init__ in Python is optional(superclass need to call it
  // explicitly), we need to initialize self->tox in ToxAV_new instead of
  // init. If ToxAV_init is called, we re-initialize self->tox and pass
  // the new ipv6enabled setting.
  return init_helper(self, args);
}

static int
ToxAV_dealloc(ToxAV* self)
{
  if (self->av) {
    Py_DECREF(self->core);
    toxav_kill(self->av);
    self->av = NULL;
    vpx_img_free(self->in_image);
    if (self->out_image) {
      free(self->out_image);
    }
  }
  return 0;
}

static void
ToxAV_set_Error(int ret)
{
  const char* msg = NULL;
  switch(ret) {
  case ErrorInternal:
    msg = "Internal error";
    break;
  case ErrorAlreadyInCall:
    msg = "Already has an active call";
    break;
  case ErrorNoCall:
    msg = "Trying to perform call action while not in a call";
    break;
  case ErrorInvalidState:
    msg = "Trying to perform call action while in invalid stat";
    break;
  case ErrorNoRtpSession:
    msg = "Trying to perform rtp action on invalid session";
    break;
  case ErrorAudioPacketLost:
    msg = "Indicating packet loss";
    break;
  case ErrorStartingAudioRtp:
    msg = "Error in toxav_prepare_transmission()";
    break;
  case ErrorStartingVideoRtp:
    msg = "Error in toxav_prepare_transmission()";
    break;
  case ErrorTerminatingAudioRtp:
    msg = "Returned in toxav_kill_transmission()";
    break;
  case ErrorTerminatingVideoRtp:
    msg = "Returned in toxav_kill_transmission()";
    break;
  case ErrorPacketTooLarge:
    msg = "Buffer exceeds size while encoding";
    break;
  case ErrorInvalidCodecState:
    msg = "Codec state not initialized";
    break;
  default:
    msg = "Unknown error";
  }

  PyErr_SetString(ToxOpError, msg);
}

static PyObject*
ToxAV_call(ToxAV* self, PyObject* args)
{
  int peer_id = 0;
  int seconds = 0;

  if (!PyArg_ParseTuple(args, "iii", &peer_id, &self->cs.call_type, &seconds)) {
    return NULL;
  }

  int32_t call_idx = 0;
  int ret = toxav_call(self->av, &call_idx, peer_id, &self->cs, seconds);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(call_idx);
}

static PyObject*
ToxAV_hangup(ToxAV* self, PyObject* args)
{
  int idx = 0;

  if (!PyArg_ParseTuple(args, "i", &idx)) {
    return NULL;
  }

  int ret = toxav_hangup(self->av, idx);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_answer(ToxAV* self, PyObject* args)
{
  int32_t idx = 0;

  if (!PyArg_ParseTuple(args, "ii", &idx, &self->cs.call_type)) {
    return NULL;
  }

  int ret = toxav_answer(self->av, idx, &self->cs);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_reject(ToxAV* self, PyObject* args)
{
  int idx = 0;
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "is", &idx, &res)) {
    return NULL;
  }

  int ret = toxav_reject(self->av, idx, res);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_cancel(ToxAV* self, PyObject* args)
{
  int idx = 0, peer_id = 0;
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "iis", &idx, &peer_id, &res)) {
    return NULL;
  }

  int ret = toxav_cancel(self->av, idx, peer_id, res);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_change_settings(ToxAV* self, PyObject* args)
{
  int idx = 0;
  PyObject* settings = NULL;

  if (!PyArg_ParseTuple(args, "iO", &idx, &settings)) {
    return NULL;
  }

  if (!PyDict_Check(settings)) {
    PyErr_SetString(PyExc_TypeError, "settings should be a dictionary");
    return NULL;
  }

#define CHECK_AND_UPDATE(d, name)                                    \
  PyObject* key_##name = PYSTRING_FromString(#name);                 \
  if (PyDict_Contains(d, key_##name)) {                              \
    self->cs.name = PyLong_AsLong(PyDict_GetItemString(d, #name));   \
  }                                                                  \
  Py_DECREF(key_##name);

  CHECK_AND_UPDATE(settings, call_type);
  CHECK_AND_UPDATE(settings, video_bitrate);
  CHECK_AND_UPDATE(settings, max_video_width);
  CHECK_AND_UPDATE(settings, max_video_height);
  CHECK_AND_UPDATE(settings, audio_bitrate);
  CHECK_AND_UPDATE(settings, audio_frame_duration);
  CHECK_AND_UPDATE(settings, audio_sample_rate);
  CHECK_AND_UPDATE(settings, audio_channels);

  int ret = toxav_change_settings(self->av, idx, &self->cs);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_stop_call(ToxAV* self, PyObject* args)
{
  int idx = 0;

  if (!PyArg_ParseTuple(args, "i", &idx)) {
    return NULL;
  }

  int ret = toxav_stop_call(self->av, idx);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_prepare_transmission(ToxAV* self, PyObject* args)
{
  int idx = 0, support_video = 0;
  uint32_t jbuf_size = 0, VAD_threshold = 0;

  if (!PyArg_ParseTuple(args, "iIIi", &idx, &jbuf_size, &VAD_threshold,
        &support_video)) {
    return NULL;
  }

  int ret = toxav_prepare_transmission(self->av, idx, jbuf_size, VAD_threshold,
      support_video);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_kill_transmission(ToxAV* self, PyObject* args)
{
  int idx = 0;

  if (!PyArg_ParseTuple(args, "i", &idx)) {
    return NULL;
  }

  int ret = toxav_kill_transmission(self->av, idx);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_send_video(ToxAV* self, PyObject* args)
{
  int idx = 0, len = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "is#", &idx, &data, &len)) {
    return NULL;
  }

  rgb_to_i420((unsigned char*)data, self->in_image);

  uint8_t encoded_payload[RTP_PAYLOAD_SIZE];
  int32_t payload_size = toxav_prepare_video_frame(self->av, idx,
      encoded_payload, RTP_PAYLOAD_SIZE, self->in_image);
  int ret = toxav_send_video(self->av, idx, encoded_payload, payload_size);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_send_audio(ToxAV* self, PyObject* args)
{
  int idx = 0, frame_size = 0, len = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "iis#", &idx, &frame_size, &data, &len)) {
    return NULL;
  }

  uint8_t encoded_payload[RTP_PAYLOAD_SIZE];
  int32_t payload_size = toxav_prepare_audio_frame(self->av, idx,
      encoded_payload, RTP_PAYLOAD_SIZE, (int16_t*)data, frame_size);

  int ret = toxav_send_audio(self->av, idx, encoded_payload, payload_size);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_get_peer_csettings(ToxAV* self, PyObject* args)
{
  int idx = 0, peer = 0;

  if (!PyArg_ParseTuple(args, "ii", &idx, &peer)) {
    return NULL;
  }

  ToxAvCSettings cs;
  int ret = toxav_get_peer_csettings(self->av, idx, peer, &cs);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

#define SET_VALUE(d, attr)                                         \
  PyObject* attr = PyLong_FromLong(cs.attr);                       \
  PyDict_SetItemString(d, #attr, attr);                            \
  Py_DECREF(attr);

  PyObject* d = PyDict_New();

  SET_VALUE(d, call_type);

  SET_VALUE(d, video_bitrate);
  SET_VALUE(d, max_video_width);
  SET_VALUE(d, max_video_height);

  SET_VALUE(d, audio_bitrate);
  SET_VALUE(d, audio_frame_duration);
  SET_VALUE(d, audio_sample_rate);
  SET_VALUE(d, audio_channels);

#undef SET_VALUE

  return d;
}

static PyObject*
ToxAV_get_peer_id(ToxAV* self, PyObject* args)
{
  int idx = 0, peer = 0;

  if (!PyArg_ParseTuple(args, "ii", &idx, &peer)) {
    return NULL;
  }

  int ret = toxav_get_peer_id(self->av, idx, peer);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxAV_get_call_state(ToxAV* self, PyObject* args)
{
  int idx = 0;

  if (!PyArg_ParseTuple(args, "i", &idx)) {
    return NULL;
  }

  int ret = toxav_get_call_state(self->av, idx);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyBool_FromLong(ret);
}

static PyObject*
ToxAV_capability_supported(ToxAV* self, PyObject* args)
{
  int idx = 0, cap = 0;

  if (!PyArg_ParseTuple(args, "ii", &idx, &cap)) {
    return NULL;
  }

  int ret = toxav_capability_supported(self->av, idx, cap);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyBool_FromLong(ret);
}

static PyObject*
ToxAV_get_tox(ToxAV* self, PyObject* args)
{
  Py_INCREF(self->core);
  return self->core;
}

static PyObject*
ToxAV_callback_stub(ToxAV* self, PyObject* args)
{
  Py_RETURN_NONE;
}

#define METHOD_DEF(name)                                   \
  {                                                        \
    #name, (PyCFunction)ToxAV_callback_stub, METH_VARARGS, \
    #name "\n"                                             \
    #name " handler. Default implementation does nothing." \
  }

PyMethodDef ToxAV_methods[] = {
  METHOD_DEF(on_invite),
  METHOD_DEF(on_start),
  METHOD_DEF(on_cancel),
  METHOD_DEF(on_reject),
  METHOD_DEF(on_end),
  METHOD_DEF(on_ringing),
  METHOD_DEF(on_starting),
  METHOD_DEF(on_ending),
  METHOD_DEF(on_error),
  METHOD_DEF(on_request_timeout),
  METHOD_DEF(on_peer_timeout),
  {
    "call", (PyCFunction)ToxAV_call, METH_VARARGS,
    "call(friend_id, call_type, seconds)\n"
    "Call a friend with *friend_id*, with *seconds* ringing timeout.\n\n"
    "*call_type* can be one of following value:\n\n"
    "+------------------+-----------------------+\n"
    "| control_type     | description           |\n"
    "+==================+=======================+\n"
    "| ToxAV.TypeAudio  | audio only call       |\n"
    "+------------------+-----------------------+\n"
    "| ToxAV.TypeVideo  | audio and video call  |\n"
    "+------------------+-----------------------+\n"
  },
  {
    "hangup", (PyCFunction)ToxAV_hangup, METH_NOARGS,
    "hangup()\n"
    "Hangup active call."
  },
  {
    "answer", (PyCFunction)ToxAV_answer, METH_VARARGS,
    "answer(idx, call_type)\n"
    "Answer incomming call.\n\n"
    ".. seealso ::\n"
    "    :meth:`.call`"
  },
  {
    "reject", (PyCFunction)ToxAV_reject, METH_VARARGS,
    "reject(idx)\n"
    "Reject incomming call."
  },
  {
    "cancel", (PyCFunction)ToxAV_cancel, METH_VARARGS,
    "cancel(idx)\n"
    "Cancel outgoing request."
  },
  {
    "change_settings", (PyCFunction)ToxAV_change_settings, METH_VARARGS,
    "change_settings(idx)\n"
    "Notify peer that we are changing call settings. *settings* is a"
    "dictionary of new settings like the one returned by "
    "get_peer_csettings\n\n"
    ".. seealso ::\n"
    "    :meth:`.get_peer_csettings`"
  },
  {
    "stop_call", (PyCFunction)ToxAV_stop_call, METH_NOARGS,
    "stop_call(idx)\n"
    "Terminate transmission. Note that transmission will be terminated "
    "without informing remote peer."
  },
  {
    "prepare_transmission", (PyCFunction)ToxAV_prepare_transmission,
    METH_VARARGS,
    "prepare_transmission(idx, jbuf_size, VAD_threshold, support_video)\n"
    "Must be call before any RTP transmission occurs. *support_video* is "
    "either True or False."
  },
  {
    "kill_transmission", (PyCFunction)ToxAV_kill_transmission,
    METH_NOARGS,
    "kill_transmission(idx)\n"
    "Call this at the end of the transmission."
  },
  {
    "on_video_data", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
    "on_video_data(idx, width, height, data)\n"
    "Receive decoded video packet. Default implementation does nothing."
  },
  {
    "on_audio_data", (PyCFunction)ToxAV_callback_stub, METH_VARARGS,
    "on_audio_data(idx, size, data)\n"
    "Receive decoded audio packet. Default implementation does nothing."
  },
  {
    "send_video", (PyCFunction)ToxAV_send_video, METH_VARARGS,
    "send_video(idx, data)\n"
    "Encode and send video packet. *data* should be a str or buffer"
    "containing a image in RGB888 format."
  },
  {
    "send_audio", (PyCFunction)ToxAV_send_audio, METH_VARARGS,
    "send_audio(idx, frame_size, data)\n"
    "Encode and send video packet. *data* should be a str or buffer"
    "containing singal channel 16 bit signed PCM audio data."
  },
  {
    "get_peer_csettings",
    (PyCFunction)ToxAV_get_peer_csettings, METH_VARARGS,
    "get_peer_csettings(idx, peer_num)\n"
    "Get peer transmission type. It can either be audio or video."
    "*peer_num* is always 0 for now."
  },
  {
    "get_peer_id", (PyCFunction)ToxAV_get_peer_id, METH_VARARGS,
    "get_peer_id(peer_num)\n"
    "Get *friend_number* of peer participating in conversation. *peer_num* "
    "is always 0 for now."

  },
  {
    "capability_supported", (PyCFunction)ToxAV_capability_supported,
    METH_VARARGS,
    "capability_supported(capability)\n"
    "Query if certain capability is supported.\n\n"
    "*capability* can be one of following value:\n\n"
    "+----------------------+-----------------+\n"
    "| control_type         | description     |\n"
    "+======================+=================+\n"
    "| ToxAV.AudioEncoding  | audio encoding  |\n"
    "+----------------------+-----------------+\n"
    "| ToxAV.AudioDecoding  | audio decoding  |\n"
    "+----------------------+-----------------+\n"
    "| ToxAV.VideoEncoding  | video encoding  |\n"
    "+----------------------+-----------------+\n"
    "| ToxAV.VideoEncoding  | video decoding  |\n"
    "+----------------------+-----------------+\n"
  },
  {
    "get_tox", (PyCFunction)ToxAV_get_tox,
    METH_VARARGS,
    "get_tox()\n"
    "Get the Tox object associated with this ToxAV instance."
  },
};


PyTypeObject ToxAVType = {
#if PY_MAJOR_VERSION >= 3
  PyVarObject_HEAD_INIT(NULL, 0)
#else
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
#endif
  "ToxAV",                   /*tp_name*/
  sizeof(ToxAV),           /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)ToxAV_dealloc, /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
  "ToxAV object",            /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  ToxAV_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)ToxAV_init,      /* tp_init */
  0,                         /* tp_alloc */
  ToxAV_new,                 /* tp_new */
};

void ToxAV_install_dict()
{
#define SET(name)                                            \
  PyObject* obj_##name = PyLong_FromLong(name);              \
  PyDict_SetItemString(dict, #name, obj_##name);             \
  Py_DECREF(obj_##name);

#define SET2(name)                                           \
  PyObject* obj_##name = PyLong_FromLong(av_##name);         \
  PyDict_SetItemString(dict, #name, obj_##name);             \
  Py_DECREF(obj_##name);

  PyObject* dict = PyDict_New();
  SET(TypeAudio);
  SET(TypeVideo);
  SET(AudioEncoding);
  SET(AudioDecoding);
  SET(VideoEncoding);
  SET(VideoDecoding);

  SET2(CallNonExistant);
  SET2(CallInviting);
  SET2(CallStarting);
  SET2(CallActive);
  SET2(CallHold);
  SET2(CallHanged_up);

  SET2(jbufdc);
  SET2(VADd);

#undef SET2

#undef SET

  ToxAVType.tp_dict = dict;
}
