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


static void
ToxAVCore_callback_call(ToxAV *toxAV, uint32_t friend_number, bool audio_enabled,
                        bool video_enabled, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_call", "iii",
                        friend_number, audio_enabled, video_enabled);
}

static void
ToxAVCore_callback_call_state(ToxAV *toxAV, uint32_t friend_number, uint32_t state, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_call_state", "ii", friend_number, state);
}

static void
ToxAVCore_callback_bit_rate_status(ToxAV *toxAV, uint32_t friend_number,
                                   uint32_t audio_bit_rate, uint32_t video_bit_rate, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_bit_rate_status", "iii",
                        friend_number, audio_bit_rate, video_bit_rate);
}

static void
ToxAVCore_callback_audio_receive_frame(ToxAV *toxAV, uint32_t friend_number,const int16_t *pcm,
                                       size_t sample_count, uint8_t channels, uint32_t sampling_rate,
                                       void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_audio_receive_frame", "iiiii",
                        friend_number, pcm, sample_count, channels, sampling_rate);
}

static void
ToxAVCore_callback_video_receive_frame(ToxAV *toxAV, uint32_t friend_number, uint16_t width,
                                       uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                                       int32_t ystride, int32_t ustride, int32_t vstride,  void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_video_receive_frame", "iiiiiiiii",
                        friend_number, width, height, y, u, v, ystride, ustride, vstride);
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
    toxav_callback_call_state(self->av, ToxAVCore_callback_call_state, self);
    toxav_callback_bit_rate_status(self->av, ToxAVCore_callback_bit_rate_status, self);
    toxav_callback_audio_receive_frame(self->av, ToxAVCore_callback_audio_receive_frame, self);
    toxav_callback_video_receive_frame(self->av, ToxAVCore_callback_video_receive_frame, self);

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
ToxAVCore_call_control(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint32_t control;

    if (!PyArg_ParseTuple(args, "ii", &friend_number, &control)) {
        return NULL;
    }

    TOXAV_ERR_CALL_CONTROL err = 0;
    bool ret = toxav_call_control(self->av, friend_number, control, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav call control error: %d", err);
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject*
ToxAVCore_bit_rate_set(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint32_t audio_bit_rate;
    uint32_t video_bit_rate;

    if (!PyArg_ParseTuple(args, "iii", &friend_number, &audio_bit_rate, &video_bit_rate)) {
        return NULL;
    }

    TOXAV_ERR_BIT_RATE_SET err = 0;
    bool ret = toxav_bit_rate_set(self->av, friend_number, audio_bit_rate, video_bit_rate, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav bit rate set error: %d", err);
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject*
ToxAVCore_audio_send_frame(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    int16_t *pcm = NULL;
    size_t sample_count;
    uint8_t channels;
    uint32_t sampling_rate;

    // TODO how recv int16_t here
    if (!PyArg_ParseTuple(args, "iiiii", &friend_number, &pcm,
        &sample_count, &channels, &sampling_rate)) {
        return NULL;
    }

    TOXAV_ERR_SEND_FRAME err = 0;
    bool ret = toxav_audio_send_frame(self->av, friend_number, pcm,
                                      sample_count, channels, sampling_rate, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav audio send frame error: %d", err);
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject*
ToxAVCore_video_send_frame(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint16_t width;
    uint16_t height;
    uint8_t *y = NULL;
    uint8_t *u = NULL;
    uint8_t *v = NULL;

    if (!PyArg_ParseTuple(args, "iiiiii", &friend_number, &width, &height,
                          y, u, v)) {
        return NULL;
    }

    TOXAV_ERR_SEND_FRAME err = 0;
    bool ret = toxav_video_send_frame(self->av, friend_number, width, height,
                                      y, u, v, &err);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav video send frame error: %d", err);
        return NULL;
    }
    return PyBool_FromLong(ret);
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

PyMethodDef ToxAVCore_methods[] = {
    {
        "call", (PyCFunction)ToxAVCore_call, METH_VARARGS,
        "call(friend_number, audio_bit_rate, video_bit_rate)\n"
        "Call a friend with *friend_number*."
        "Returns True on success.\n\n"
    },
    {
        "call_control", (PyCFunction)ToxAVCore_call_control, METH_VARARGS,
        "call_control(friend_number, control)\n"
        "Sends a call control command to a friend."
        "Returns True on success.\n\n"
    },
    {
        "bit_rate_set", (PyCFunction)ToxAVCore_bit_rate_set, METH_VARARGS,
        "bit_rate_set(friend_number, audio_bit_rate, video_bit_rate)\n"
        "Set the bit rate to be used in subsequent audio/video frames."
        "Returns True on success.\n\n"
    },
    {
        "audio_send_frame", (PyCFunction)ToxAVCore_audio_send_frame, METH_VARARGS,
        "audio_send_frame(friend_number, pcm, sample_count, channels, sampling_rate)\n"
        "Send an audio frame to a friend."
        "Returns True on success.\n\n"
    },
    {
        "video_send_frame", (PyCFunction)ToxAVCore_video_send_frame, METH_VARARGS,
        "video_send_frame(friend_number, width, height, y, u, v)\n"
        "Send a video frame to a friend."
        "Returns True on success.\n\n"
    },
    {
        "answer", (PyCFunction)ToxAVCore_answer, METH_VARARGS,
        "answer(call_index, call_type)\n"
        "Answer incomming call.\n\n"
        ".. seealso ::\n"
        "    :meth:`.call`"
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
