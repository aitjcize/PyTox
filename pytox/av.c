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

#if PY_MAJOR_VERSION < 3
# define BUF_TCS "s#"
#else
# define BUF_TCS "y#"
#endif

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
    PyGILState_STATE gstate = PyGILState_Ensure();

    uint32_t length = sample_count * channels * 2;

    PyObject_CallMethod((PyObject*)self, "on_audio_receive_frame", "i" BUF_TCS "iii",
                        friend_number, (const char*)pcm, length, sample_count, channels, sampling_rate);

    if (PyErr_Occurred()) {
        PyErr_Print();
    }

    PyGILState_Release(gstate);
}

static void
ToxAVCore_callback_video_receive_frame(ToxAV *toxAV, uint32_t friend_number, uint16_t width,
                                       uint16_t height, const uint8_t *y, const uint8_t *u, const uint8_t *v,
                                       int32_t ystride, int32_t ustride, int32_t vstride,  void *self)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    uint32_t ylength = height * width;
    uint32_t ulength = (height/2) * (width/2);
    uint32_t vlength = (height/2) * (width/2);

    PyObject_CallMethod((PyObject*)self, "on_video_receive_frame", "iii" BUF_TCS BUF_TCS BUF_TCS "iii",
                        friend_number, width, height,
                        (char*)y, ylength, (char*)u, ulength, (char*)v, vlength,
                        ystride, ustride, vstride);

    if (PyErr_Occurred()) {
        PyErr_Print();
    }

    PyGILState_Release(gstate);
}

/**
 * NOTE Compatibility with old toxav group calls TODO remove
 */
static void
ToxAVCore_callback_add_av_groupchat(ToxAV *toxAV, int groupnumber, int peernumber, const int16_t *pcm,
                                    unsigned int samples, uint8_t channels, unsigned int sample_rate, void *self)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    uint32_t length = samples * channels * 2;

    PyObject_CallMethod((PyObject*)self, "on_add_av_groupchat", "ii" BUF_TCS "iii",
                        groupnumber, peernumber, (char*)pcm, length,
                        samples, channels, sample_rate);

    if (PyErr_Occurred()) {
        PyErr_Print();
    }

    PyGILState_Release(gstate);
}

static void
ToxAVCore_callback_join_av_groupchat(ToxAV *toxAV, int groupnumber, int peernumber, const int16_t *pcm,
                                     unsigned int samples, uint8_t channels, unsigned int sample_rate, void *self)
{
    PyGILState_STATE gstate = PyGILState_Ensure();

    uint32_t length = samples * channels * 2;

    PyObject_CallMethod((PyObject*)self, "on_join_av_groupchat", "ii" BUF_TCS "iii",
                        groupnumber, peernumber, (char*)pcm, length,
                        samples, channels, sample_rate);

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

    if (self->av == NULL) {
        PyErr_Format(ToxOpError, "failed to allocate toxav %d", err);
        return -1;
    }

    toxav_callback_call(self->av, ToxAVCore_callback_call, self);
    toxav_callback_call_state(self->av, ToxAVCore_callback_call_state, self);
    toxav_callback_bit_rate_status(self->av, ToxAVCore_callback_bit_rate_status, self);
    toxav_callback_audio_receive_frame(self->av, ToxAVCore_callback_audio_receive_frame, self);
    toxav_callback_video_receive_frame(self->av, ToxAVCore_callback_video_receive_frame, self);

    /**
     * NOTE Compatibility with old toxav group calls TODO remove
     */
    toxav_add_av_groupchat(self->av, ToxAVCore_callback_add_av_groupchat, self);
    // toxav_join_av_groupchat(self->av, ToxAVCore_callback_join_av_groupchat, self);

    return 0;
}

static PyObject*
ToxAVCore_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
    ToxAVCore* self = (ToxAVCore*)type->tp_alloc(type, 0);
    self->av = NULL;
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
    }
    return 0;
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
    uint32_t pcm_length;
    size_t sample_count;
    uint8_t channels;
    uint32_t sampling_rate;

    if (!PyArg_ParseTuple(args, "i" BUF_TCS "iii", &friend_number,
                          &pcm, &pcm_length, &sample_count, &channels, &sampling_rate)) {
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
    uint32_t ylength;
    uint32_t ulength;
    uint32_t vlength;

    if (!PyArg_ParseTuple(args, "iii" BUF_TCS BUF_TCS BUF_TCS, &friend_number, &width, &height,
                          &y, &ylength, &u, &ulength, &v, &vlength)) {
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

static PyObject*
ToxAVCore_join_av_groupchat(ToxAVCore *self, PyObject* args)
{
    uint32_t friend_number;
    uint8_t *data;
    uint32_t length;

    if (!PyArg_ParseTuple(args, "i" BUF_TCS, &friend_number, &data, &length)) {
        return NULL;
    }

    TOXAV_ERR_ANSWER err = 0;
    bool ret = toxav_join_av_groupchat(self->av, friend_number, data, length,
                                       ToxAVCore_callback_join_av_groupchat, self);
    if (ret == false) {
        PyErr_Format(ToxOpError, "toxav join av groupchat error: %d", err);
        return NULL;
    }

    Py_RETURN_TRUE;
}

static PyObject*
ToxAVCore_group_send_audio(ToxAVCore *self, PyObject* args)
{
    uint32_t group_number;
    uint8_t *pcm;
    uint32_t length;
    uint32_t samples;
    uint32_t channels;
    uint32_t sample_rate;

    if (!PyArg_ParseTuple(args, "i" BUF_TCS "iii", &group_number, &pcm, &length,
                          &samples, &channels, &sample_rate)) {
        return NULL;
    }

    TOXAV_ERR_ANSWER err = 0;
    int ret = toxav_group_send_audio(self->av, group_number, pcm, samples, channels, sample_rate);
    if (ret == -1) {
        PyErr_Format(ToxOpError, "toxav group send audio error: %d", err);
        return NULL;
    }

    return PyLong_FromLong(ret);
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
        "answer(friend_number, audio_bit_rate, video_bit_rate)\n"
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
    {
        "join_av_groupchat", (PyCFunction)ToxAVCore_join_av_groupchat, METH_VARARGS,
        "join_av_groupchat(friend_number, data)\n"
        "Join a AV group (you need to have been invited first.)"
        "Returns -1 on failure.\n\n"
    },
    {
        "group_send_audio", (PyCFunction)ToxAVCore_group_send_audio, METH_VARARGS,
        "group_send_audio(groupnumber, pcm, samples, channels, sample_rate)\n"
        "Send audio to the group chat."
        "Returns -1 on failure.\n\n"
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
