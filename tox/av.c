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

#include "core.h"
#include "util.h"

/* ToxAV definition */
typedef struct {
  PyObject_HEAD
  PyObject* core;
  ToxAv* av;
  vpx_image_t* image;
} ToxAV;

extern PyObject* ToxOpError;


#define AUDIO_FRAME_SIZE (av_DefaultSettings.audio_sample_rate * av_DefaultSettings.audio_frame_duration / 1000)

#define CALLBACK_DEF(name)                                   \
  void ToxAV_callback_##name(void* self)                     \
  {                                                          \
    fprintf(stderr, "<%s>\n", #name); \
    PyObject_CallMethod((PyObject*)self, #name, NULL);       \
  }

CALLBACK_DEF(on_invite);
CALLBACK_DEF(on_start);
CALLBACK_DEF(on_cancel);
CALLBACK_DEF(on_reject);
CALLBACK_DEF(on_end);
CALLBACK_DEF(on_ringing);
CALLBACK_DEF(on_starting);
CALLBACK_DEF(on_ending);
CALLBACK_DEF(on_error);
CALLBACK_DEF(on_request_timeout);
CALLBACK_DEF(on_peer_timeout);

static int init_helper(ToxAV* self, PyObject* args)
{
  if (self->av) {
    toxav_kill(self->av);
    self->av = NULL;
  }

  PyObject* core = NULL;
  int width = 0, height = 0;

  if (!PyArg_ParseTuple(args, "Oii", &core, &width, &height)) {
    PyErr_SetString(PyExc_TypeError, "must associate with a core instance");
    return -1;
  }

  self->core = core;
  Py_INCREF(self->core);

  ToxAvCodecSettings cs = av_DefaultSettings;
  cs.video_height = height;
  cs.video_width = width;

  self->av = toxav_new(((ToxCore*)self->core)->tox, &cs);

#define REG_CALLBACK(id, name) \
  toxav_register_callstate_callback(ToxAV_callback_##name, id, self)

  REG_CALLBACK(av_OnInvite, on_invite);
  REG_CALLBACK(av_OnStart, on_start);
  REG_CALLBACK(av_OnCancel, on_cancel);
  REG_CALLBACK(av_OnReject, on_reject);
  REG_CALLBACK(av_OnEnd, on_end);
  REG_CALLBACK(av_OnRinging, on_ringing);
  REG_CALLBACK(av_OnStarting, on_starting);
  REG_CALLBACK(av_OnEnding, on_ending);
  REG_CALLBACK(av_OnError, on_error);
  REG_CALLBACK(av_OnRequestTimeout, on_request_timeout);
  REG_CALLBACK(av_OnPeerTimeout, on_peer_timeout);

#undef REG_CALLBACK


  if (self->image == NULL) {
    self->image = vpx_img_alloc(NULL, VPX_IMG_FMT_I420, width, height, 1);
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
  self->image = NULL;

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
    vpx_img_free(self->image);
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
  default:
    msg = "Unknown error";
  }

  PyErr_SetString(ToxOpError, msg);
}

static PyObject*
ToxAV_call(ToxAV* self, PyObject* args)
{
  int peer_id = 0;
  int call_type = 0;
  int seconds = 0;

  if (!PyArg_ParseTuple(args, "iii", &peer_id, &call_type, &seconds)) {
    return NULL;
  }

  int ret = toxav_call(self->av, peer_id, call_type, seconds);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_hangup(ToxAV* self, PyObject* args)
{
  int ret = toxav_hangup(self->av);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_answer(ToxAV* self, PyObject* args)
{
  int call_type = 0;

  if (!PyArg_ParseTuple(args, "i", &call_type)) {
    return NULL;
  }

  int ret = toxav_answer(self->av, call_type);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_reject(ToxAV* self, PyObject* args)
{
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "s", &res)) {
    return NULL;
  }

  int ret = toxav_reject(self->av, res);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_cancel(ToxAV* self, PyObject* args)
{
  int peer_id = 0;
  const char* res = NULL;

  if (!PyArg_ParseTuple(args, "is", &peer_id, &res)) {
    return NULL;
  }

  int ret = toxav_cancel(self->av, peer_id, res);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_stop_call(ToxAV* self, PyObject* args)
{
  int ret = toxav_stop_call(self->av);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_prepare_transmission(ToxAV* self, PyObject* args)
{
  int support_video = 0;

  if (!PyArg_ParseTuple(args, "i", &support_video)) {
    return NULL;
  }

  int ret = toxav_prepare_transmission(self->av, support_video);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_kill_transmission(ToxAV* self, PyObject* args)
{
  int ret = toxav_kill_transmission(self->av);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_recv_video(ToxAV* self, PyObject* args)
{
  vpx_image_t* image = NULL;

  int ret = toxav_recv_video(self->av, &image);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  if (image == NULL) {
    Py_RETURN_NONE;
  }

  int stride = image->d_w * image->d_h;
  uint8_t* img = (uint8_t*)malloc(stride * 3);
  memcpy(img, image->planes[VPX_PLANE_Y], stride);
  memcpy(img + stride, image->planes[VPX_PLANE_U], stride);
  memcpy(img + (stride << 1), image->planes[VPX_PLANE_V], stride);

  PyObject* d = PyDict_New();
  PyDict_SetItemString(d, "d_w", PyLong_FromLong(image->d_w));
  PyDict_SetItemString(d, "d_h", PyLong_FromLong(image->d_h));
  PyDict_SetItemString(d, "data", PYBYTES_FromStringAndSize(img, stride * 3));

  free(img);

  return d;
}

static PyObject*
ToxAV_recv_audio(ToxAV* self, PyObject* args)
{
  int16_t* PCM = (int16_t*)malloc(AUDIO_FRAME_SIZE * sizeof(int16_t));

  int ret = toxav_recv_audio(self->av, AUDIO_FRAME_SIZE, PCM);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  if (ret == 0) {
    free(PCM);
    Py_RETURN_NONE;
  }

  PyObject* d = PyDict_New();
  PyDict_SetItemString(d, "size", PyLong_FromLong(ret));
  PyDict_SetItemString(d, "data",
      PYBYTES_FromStringAndSize((char*)PCM, AUDIO_FRAME_SIZE << 1));

  free(PCM);
  return d;
}

static PyObject*
ToxAV_send_video(ToxAV* self, PyObject* args)
{
  int d_w = 0, d_h = 0, len = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "iis#", &d_w, &d_h, &data, &len)) {
    return NULL;
  }

  int stride = d_w * d_h;
  memcpy(self->image->planes[VPX_PLANE_Y], data, stride);
  memcpy(self->image->planes[VPX_PLANE_U], data + stride, stride);
  memcpy(self->image->planes[VPX_PLANE_V], data + (stride << 1), stride);

  int ret = toxav_send_video(self->av, self->image);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_send_audio(ToxAV* self, PyObject* args)
{
  int frame_size = 0, len = 0;
  char* data = NULL;

  if (!PyArg_ParseTuple(args, "is#",  &frame_size, &data, &len)) {
    return NULL;
  }

  int ret = toxav_send_audio(self->av, (int16_t*)data, frame_size);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxAV_get_peer_transmission_type(ToxAV* self, PyObject* args)
{
  int peer = 0;

  if (!PyArg_ParseTuple(args, "i", &peer)) {
    return NULL;
  }

  int ret = toxav_get_peer_transmission_type(self->av, peer);
  if (ret != 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxAV_get_peer_id(ToxAV* self, PyObject* args)
{
  int peer = 0;

  if (!PyArg_ParseTuple(args, "i", &peer)) {
    return NULL;
  }

  int ret = toxav_get_peer_id(self->av, peer);
  if (ret < 0) {
    ToxAV_set_Error(ret);
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxAV_capability_supported(ToxAV* self, PyObject* args)
{
  int cap = 0;

  if (!PyArg_ParseTuple(args, "i", &cap)) {
    return NULL;
  }

  int ret = toxav_capability_supported(self->av, cap);

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

#define METHOD_DEF(name)                                     \
  {                                                          \
    #name, (PyCFunction)ToxAV_callback_stub, METH_NOARGS, "" \
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
    "call", (PyCFunction)ToxAV_call, METH_VARARGS, ""
  },
  {
    "hangup", (PyCFunction)ToxAV_hangup, METH_NOARGS, ""
  },
  {
    "answer", (PyCFunction)ToxAV_answer, METH_VARARGS, ""
  },
  {
    "reject", (PyCFunction)ToxAV_reject, METH_VARARGS, ""
  },
  {
    "cancel", (PyCFunction)ToxAV_cancel, METH_VARARGS, ""
  },
  {
    "stop_call", (PyCFunction)ToxAV_stop_call, METH_NOARGS, ""
  },
  {
    "prepare_transmission", (PyCFunction)ToxAV_prepare_transmission,
    METH_VARARGS, ""
  },
  {
    "kill_transmission", (PyCFunction)ToxAV_kill_transmission,
    METH_NOARGS, ""
  },
  {
    "recv_video", (PyCFunction)ToxAV_recv_video, METH_VARARGS, ""
  },
  {
    "recv_audio", (PyCFunction)ToxAV_recv_audio, METH_VARARGS, ""
  },
  {
    "send_video", (PyCFunction)ToxAV_send_video, METH_VARARGS, ""
  },
  {
    "send_audio", (PyCFunction)ToxAV_send_audio, METH_VARARGS, ""
  },
  {
    "get_peer_transmission_type",
    (PyCFunction)ToxAV_get_peer_transmission_type, METH_VARARGS, ""
  },
  {
    "get_peer_id", (PyCFunction)ToxAV_get_peer_id, METH_VARARGS, ""
  },
  {
    "capability_supported", (PyCFunction)ToxAV_capability_supported,
    METH_VARARGS, ""
  },
  {
    "get_tox", (PyCFunction)ToxAV_get_tox,
    METH_VARARGS, ""
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
#define SET(name) \
  PyDict_SetItemString(dict, #name, PyLong_FromLong(name));

  PyObject* dict = PyDict_New();
  SET(TypeAudio);
  SET(TypeVideo);
  SET(None);
  SET(AudioEncoding);
  SET(AudioDecoding);
  SET(VideoEncoding);
  SET(VideoDecoding);

#undef SET

  ToxAVType.tp_dict = dict;
}
