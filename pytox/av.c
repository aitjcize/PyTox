/**
 * @file   av.c
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

#include <Python.h>
#include <tox/toxav.h>

#include "av.h"
#include "core.h"
#include "util.h"

extern PyObject* ToxOpError;

#define CALLBACK_DEF(name)                                                 \
  void ToxAVCore_callback_##name(void* agent, int32_t call_index, void* self)  \
  {                                                                        \
    PyObject_CallMethod((PyObject*)self, #name, "i", call_index);          \
  }

static void
ToxAVCore_callback_call(ToxAV *toxAV, uint32_t friend_number, bool audio_enabled,
                        bool video_enabled, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_call", "iii",
                        friend_number, audio_enabled, video_enabled);
}

static void i420_to_rgb(const vpx_image_t *img, unsigned char *out)
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

void ToxAVCore_audio_recv_callback(void* agent, int32_t call_idx,
    const int16_t* data, uint16_t size, void* self)
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject_CallMethod((PyObject*)self, "on_audio_data", "iis#", call_idx, size,
      (char*)data, size << 1);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }

  PyGILState_Release(gstate);
}

void ToxAVCore_video_recv_callback(void* agent, int32_t call_idx,
    const vpx_image_t* image, void* user)
{
  ToxAVCore* self = (ToxAVCore*)user;

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

  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject_CallMethod((PyObject*)self, "on_video_data", "iiis#", call_idx,
      self->o_w, self->o_h, self->out_image, buf_size);

  if (PyErr_Occurred()) {
    PyErr_Print();
  }

  PyGILState_Release(gstate);
}


static int init_helper(ToxAVCore *self, PyObject* args)
{
    PyEval_InitThreads();

    if (self->av) {
        toxav_kill(self->av);
        self->av = NULL;
    }

    PyObject* core = NULL;

    if (!PyArg_ParseTuple(args, "O", &core)) {
        PyErr_SetString(PyExc_TypeError, "must associate with a core instance");
        return -1;
    }

    self->core = core;
    Py_INCREF(self->core);

    TOXAV_ERR_NEW err = 0;
    self->av = toxav_new(((ToxCore*)self->core)->tox, &err);

    self->cs = av_DefaultSettings;
    self->cs.max_video_width = self->cs.max_video_height = 0;

    toxav_callback_call(self->av, ToxAVCore_callback_call, self);

#define REG_CALLBACK(id, name)                                          \
    toxav_register_callstate_callback(self->av, ToxAVCore_callback_##name, id, self)

#undef REG_CALLBACK

    toxav_register_audio_callback(self->av, ToxAVCore_audio_recv_callback, self);
    toxav_register_video_callback(self->av, ToxAVCore_video_recv_callback, self);

    if (self->av == NULL) {
        PyErr_Format(ToxOpError, "failed to allocate toxav %d", err);
        return -1;
    }

    return 0;
}

static PyObject*
ToxAVCore_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
    ToxAVCore* self = (ToxAVCore*)type->tp_alloc(type, 0);
    self->av = NULL;
    self->in_image = NULL;
    self->out_image = NULL;
    self->i_w = self->i_h = self->o_w = self->o_h = 0;

    if (init_helper(self, args) == -1) {
        return NULL;
    }

    return (PyObject*)self;
}

static int
ToxAVCore_init(ToxAVCore *self, PyObject* args, PyObject* kwds)
{
    // since __init__ in Python is optional(superclass need to call it
    // explicitly), we need to initialize self->tox in ToxAVCore_new instead of
    // init. If ToxAVCore_init is called, we re-initialize self->tox and pass
    // the new ipv6enabled setting.
    return init_helper(self, args);
}

static int
ToxAVCore_dealloc(ToxAVCore *self)
{
  if (self->av) {
    Py_DECREF(self->core);
    toxav_kill(self->av);
    self->av = NULL;

    if (self->in_image) {
      vpx_img_free(self->in_image);
    }
    if (self->out_image) {
      free(self->out_image);
    }
  }
  return 0;
}

static void
ToxAVCore_set_Error(int ret)
{
  const char* msg = NULL;
  switch(ret) {
  }

  PyErr_SetString(ToxOpError, msg);
}

static PyObject*
ToxAVCore_call(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (!PyArg_ParseTuple(args, "iii", &friend_number, &audio_bit_rate, &video_bit_rate)) {
        return NULL;
    }

    TOXAV_ERR_CALL err = 0;
    bool ret = toxav_call(self->av, friend_number, audio_bit_rate, video_bit_rate, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav call error: %d", err);
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject*
ToxAVCore_hangup(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;

  if (!PyArg_ParseTuple(args, "i", &call_index)) {
    return NULL;
  }

  int ret = toxav_hangup(self->av, call_index);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_answer(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (!PyArg_ParseTuple(args, "iii", &friend_number, &audio_bit_rate, &video_bit_rate)) {
        return NULL;
    }

    TOXAV_ERR_ANSWER err = 0;
    bool ret = toxav_answer(self->av, friend_number, audio_bit_rate, video_bit_rate, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav answer error: %d", err);
        return NULL;
    }

    Py_RETURN_TRUE;
}

static PyObject*
ToxAVCore_reject(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "is", &call_index, &res)) {
    return NULL;
  }

  int ret = toxav_reject(self->av, call_index, res);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_cancel(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, peer_id = 0;
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "iis", &call_index, &peer_id, &res)) {
    return NULL;
  }

  int ret = toxav_cancel(self->av, call_index, peer_id, res);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_change_settings(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;
  PyObject* settings = NULL;

  if (!PyArg_ParseTuple(args, "iO", &call_index, &settings)) {
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

#undef CHECK_AND_UPDATE

  int ret = toxav_change_settings(self->av, call_index, &self->cs);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_stop_call(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;

  if (!PyArg_ParseTuple(args, "i", &call_index)) {
    return NULL;
  }

  int ret = toxav_stop_call(self->av, call_index);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_prepare_transmission(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, support_video = 0;

  if (!PyArg_ParseTuple(args, "ii", &call_index, &support_video)) {
    return NULL;
  }

  int ret = toxav_prepare_transmission(self->av, call_index, support_video);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_kill_transmission(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;

  if (!PyArg_ParseTuple(args, "i", &call_index)) {
    return NULL;
  }

  int ret = toxav_kill_transmission(self->av, call_index);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

#define RTP_PAYLOAD_SIZE 65535

static PyObject*
ToxAVCore_send_video(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, len = 0, w = 0, h = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "iiis#", &call_index, &w, &h, &data, &len)) {
    return NULL;
  }

  if (self->in_image && (self->i_w != w || self->i_h != h)) {
    vpx_img_free(self->in_image);
    self->in_image = NULL;
  }

  if (self->in_image == NULL) {
    self->i_w = w;
    self->i_h = h;
    self->in_image = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, w, h, 1);
  }

  rgb_to_i420((unsigned char*)data, self->in_image);

  uint8_t encoded_payload[RTP_PAYLOAD_SIZE];
  int32_t payload_size = toxav_prepare_video_frame(self->av, call_index,
      encoded_payload, RTP_PAYLOAD_SIZE, self->in_image);

  int ret = toxav_send_video(self->av, call_index, encoded_payload,
      payload_size);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_send_audio(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, frame_size = 0, len = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "iis#", &call_index, &frame_size, &data, &len)) {
    return NULL;
  }

  uint8_t encoded_payload[RTP_PAYLOAD_SIZE];
  int32_t payload_size = toxav_prepare_audio_frame(self->av, call_index,
      encoded_payload, RTP_PAYLOAD_SIZE, (int16_t*)data, frame_size);

  int ret = toxav_send_audio(self->av, call_index, encoded_payload,
      payload_size);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAVCore_get_peer_csettings(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, peer = 0;

  if (!PyArg_ParseTuple(args, "ii", &call_index, &peer)) {
    return NULL;
  }

  ToxAvCSettings cs;
  int ret = toxav_get_peer_csettings(self->av, call_index, peer, &cs);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
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
ToxAVCore_get_peer_id(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, peer = 0;

  if (!PyArg_ParseTuple(args, "ii", &call_index, &peer)) {
    return NULL;
  }

  int ret = toxav_get_peer_id(self->av, call_index, peer);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxAVCore_get_call_state(ToxAVCore *self, PyObject* args)
{
  int call_index = 0;

  if (!PyArg_ParseTuple(args, "i", &call_index)) {
    return NULL;
  }

  int ret = toxav_get_call_state(self->av, call_index);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxAVCore_capability_supported(ToxAVCore *self, PyObject* args)
{
  int call_index = 0, cap = 0;

  if (!PyArg_ParseTuple(args, "ii", &call_index, &cap)) {
    return NULL;
  }

  int ret = toxav_capability_supported(self->av, call_index, cap);
  if (ret < 0) {
    ToxAVCore_set_Error(ret);
    return NULL;
  }

  return PyBool_FromLong(ret);
}

static PyObject*
ToxAVCore_get_tox(ToxAVCore *self, PyObject* args)
{
  Py_INCREF(self->core);
  return self->core;
}

static PyObject*
ToxAVCore_iteration_interval(ToxAVCore *self)
{
    uint32_t interval = toxav_iteration_interval(self->av);
    return PyLong_FromLong(interval);
}

static PyObject*
ToxAVCore_iterate(ToxAVCore *self)
{
    toxav_iterate(self->av);
    Py_RETURN_NONE;
}


static PyObject*
ToxAVCore_callback_stub(ToxAVCore *self, PyObject* args)
{
  Py_RETURN_NONE;
}

#define METHOD_DEF(name)                                   \
  {                                                        \
    #name, (PyCFunction)ToxAVCore_callback_stub, METH_VARARGS, \
    #name "(call_index)\n"                                        \
    #name " handler. Default implementation does nothing." \
  }

PyMethodDef ToxAVCore_methods[] = {
  METHOD_DEF(on_invite),
  METHOD_DEF(on_ringing),
  METHOD_DEF(on_start),
  METHOD_DEF(on_cancel),
  METHOD_DEF(on_reject),
  METHOD_DEF(on_end),
  METHOD_DEF(on_request_timeout),
  METHOD_DEF(on_peer_timeout),
  METHOD_DEF(on_peer_cs_change),
  METHOD_DEF(on_self_cs_change),
  {
    "call", (PyCFunction)ToxAVCore_call, METH_VARARGS,
    "call(friend_number, audio_bit_rate, video_bit_rate)\n"
    "Call a friend with *friend_number*."
    "Returns True if success.\n\n"
  },
  {
    "hangup", (PyCFunction)ToxAVCore_hangup, METH_VARARGS,
    "hangup(call_index)\n"
    "Hangup active call."
  },
  {
    "answer", (PyCFunction)ToxAVCore_answer, METH_VARARGS,
    "answer(call_index, call_type)\n"
    "Answer incomming call.\n\n"
    ".. seealso ::\n"
    "    :meth:`.call`"
  },
  {
    "reject", (PyCFunction)ToxAVCore_reject, METH_VARARGS,
    "reject(call_index)\n"
    "Reject incomming call."
  },
  {
    "cancel", (PyCFunction)ToxAVCore_cancel, METH_VARARGS,
    "cancel(call_index)\n"
    "Cancel outgoing request."
  },
  {
    "change_settings", (PyCFunction)ToxAVCore_change_settings, METH_VARARGS,
    "change_settings(call_index, csettings)\n"
    "Notify peer that we are changing call settings. *settings* is a"
    "dictionary of new settings like the one returned by "
    "get_peer_csettings\n\n"
    ".. seealso ::\n"
    "    :meth:`.get_peer_csettings`"
  },
  {
    "stop_call", (PyCFunction)ToxAVCore_stop_call, METH_VARARGS,
    "stop_call(call_index)\n"
    "Terminate transmission. Note that transmission will be terminated "
    "without informing remote peer."
  },
  {
    "prepare_transmission", (PyCFunction)ToxAVCore_prepare_transmission,
    METH_VARARGS,
    "prepare_transmission(call_index, support_video)\n"
    "Must be call before any RTP transmission occurs. *support_video* is "
    "either True or False."
  },
  {
    "kill_transmission", (PyCFunction)ToxAVCore_kill_transmission, METH_VARARGS,
    "kill_transmission(call_index)\n"
    "Call this at the end of the transmission."
  },
  {
    "on_video_data", (PyCFunction)ToxAVCore_callback_stub, METH_VARARGS,
    "on_video_data(call_index, width, height, data)\n"
    "Receive decoded video packet. Default implementation does nothing."
  },
  {
    "on_audio_data", (PyCFunction)ToxAVCore_callback_stub, METH_VARARGS,
    "on_audio_data(call_index, size, data)\n"
    "Receive decoded audio packet. Default implementation does nothing."
  },
  {
    "send_video", (PyCFunction)ToxAVCore_send_video, METH_VARARGS,
    "send_video(call_index, data)\n"
    "Encode and send video packet. *data* should be a str or buffer"
    "containing a image in RGB888 format."
  },
  {
    "send_audio", (PyCFunction)ToxAVCore_send_audio, METH_VARARGS,
    "send_audio(call_index, frame_size, data)\n"
    "Encode and send video packet. *data* should be a str or buffer"
    "containing singal channel 16 bit signed PCM audio data."
  },
  {
    "get_peer_csettings",
    (PyCFunction)ToxAVCore_get_peer_csettings, METH_VARARGS,
    "get_peer_csettings(call_index, peer_num)\n"
    "Get peer transmission type. It can either be audio or video. "
    "*peer_num* is always 0 for now."
  },
  {
    "get_peer_id", (PyCFunction)ToxAVCore_get_peer_id, METH_VARARGS,
    "get_peer_id(call_index, peer_num)\n"
    "Get *friend_number* of peer participating in conversation. *peer_num* "
    "is always 0 for now."

  },
  {
    "get_call_state", (PyCFunction)ToxAVCore_get_call_state, METH_VARARGS,
    "get_call_state(call_index)\n"
    "Get current call state\n\n"
    "The state returned can be one of following value:\n\n"
    "+------------------------+\n"
    "| state                  |\n"
    "+========================+\n"
    "| ToxAV.CallNonExistent  |\n"
    "+------------------------+\n"
    "| ToxAV.CallInviting     |\n"
    "+------------------------+\n"
    "| ToxAV.CallStarting     |\n"
    "+------------------------+\n"
    "| ToxAV.CallActive       |\n"
    "+------------------------+\n"
    "| ToxAV.CallHold         |\n"
    "+------------------------+\n"
    "| ToxAV.CallHanged_up    |\n"
    "+------------------------+\n"
  },
  {
    "capability_supported", (PyCFunction)ToxAVCore_capability_supported,
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
    "get_tox", (PyCFunction)ToxAVCore_get_tox,
    METH_VARARGS,
    "get_tox()\n"
    "Get the Tox object associated with this ToxAV instance."
  },
  {
      "iteration_interval", (PyCFunction)ToxAVCore_iteration_interval,
      METH_VARARGS,
      "iteration_interval()\n"
      "Returns the interval in milliseconds when the next toxav_iterate call shouldbe."
  },
  {
      "iterate", (PyCFunction)ToxAVCore_iterate,
      METH_VARARGS,
      "iterate()\n"
      "Main loop for the session."
  },
};


PyTypeObject ToxAVCoreType = {
#if PY_MAJOR_VERSION >= 3
  PyVarObject_HEAD_INIT(NULL, 0)
#else
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
#endif
  "ToxAV",                   /*tp_name*/
  sizeof(ToxAVCore),           /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)ToxAVCore_dealloc, /*tp_dealloc*/
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
  ToxAVCore_methods,             /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)ToxAVCore_init,      /* tp_init */
  0,                         /* tp_alloc */
  ToxAVCore_new,                 /* tp_new */
};

void ToxAVCore_install_dict()
{
#define SET(name)                                           \
    PyObject* obj_##name = PyLong_FromLong(TOXAV_##name);   \
    PyDict_SetItemString(dict, #name, obj_##name);          \
    Py_DECREF(obj_##name);

    PyObject* dict = PyDict_New();
    SET(FRIEND_CALL_STATE_ERROR);
    SET(FRIEND_CALL_STATE_FINISHED);
    SET(FRIEND_CALL_STATE_SENDING_A);
    SET(FRIEND_CALL_STATE_SENDING_V);
    SET(FRIEND_CALL_STATE_ACCEPTING_A);
    SET(FRIEND_CALL_STATE_ACCEPTING_V);

    SET(CALL_CONTROL_RESUME);
    SET(CALL_CONTROL_PAUSE);
    SET(CALL_CONTROL_CANCEL);
    SET(CALL_CONTROL_MUTE_AUDIO);
    SET(CALL_CONTROL_UNMUTE_AUDIO);
    SET(CALL_CONTROL_HIDE_VIDEO);
    SET(CALL_CONTROL_SHOW_VIDEO);
#undef SET

    ToxAVCoreType.tp_dict = dict;
}
