/**
 * @file   core.c
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

#include "core.h"
#include "util.h"

#if PY_MAJOR_VERSION < 3
# define BUF_TC "s"
#else
# define BUF_TC "y"
#endif

static void callback_log(Tox *tox, TOX_LOG_LEVEL level, const char *file, uint32_t line, const char *func,
                         const char *message, void* self)
{
    PyObject_CallMethod((PyObject*)self, "on_log", "isiss", level, file, line,
                        func, message);
}

static void callback_self_connection_status(Tox* tox, TOX_CONNECTION connection_status,
                                            void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_self_connection_status", "i",
                        connection_status);
}

static void callback_friend_request(Tox* tox, const uint8_t* public_key,
                                    const uint8_t* data, size_t length, void* self)
{
  uint8_t buf[TOX_PUBLIC_KEY_SIZE * 2 + 1];
  memset(buf, 0, TOX_PUBLIC_KEY_SIZE * 2 + 1);

  bytes_to_hex_string(public_key, TOX_PUBLIC_KEY_SIZE, buf);

  PyObject_CallMethod((PyObject*)self, "on_friend_request", "ss#", buf, data,
      length - (data[length - 1] == 0));
}

static void callback_friend_message(Tox *tox, uint32_t friendnumber, TOX_MESSAGE_TYPE type,
                                    const uint8_t* message, size_t length, void* self)
{
    PyObject_CallMethod((PyObject*)self, "on_friend_message", "iis#", friendnumber, type,
                        message, length - (message[length - 1] == 0));
}

static void callback_friend_name(Tox *tox, uint32_t friendnumber,
                                 const uint8_t* newname, size_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_name", "is#", friendnumber,
      newname, length - (newname[length - 1] == 0));
}

static void callback_friend_status_message(Tox *tox, uint32_t friendnumber,
                                           const uint8_t *newstatus, size_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_status_message", "is#", friendnumber,
      newstatus, length - (newstatus[length - 1] == 0));
}

static void callback_friend_status(Tox *tox, uint32_t friendnumber, TOX_USER_STATUS status,
                                   void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_status", "ii", friendnumber,
      status);
}

static void callback_friend_typing(Tox *tox, uint32_t friendnumber,
    bool is_typing, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_typing", "iO", friendnumber,
      PyBool_FromLong(is_typing));
}

static void callback_friend_read_receipt(Tox *tox, uint32_t friendnumber,
    uint32_t receipt, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_read_receipt", "ii", friendnumber,
      receipt);
}

static void callback_friend_connection_status(Tox *tox, uint32_t friendnumber,
    TOX_CONNECTION status, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_connection_status", "iO",
      friendnumber, PyBool_FromLong(status));
}

static void callback_conference_invite(Tox *tox, uint32_t friendnumber, TOX_CONFERENCE_TYPE type,
    const uint8_t *data, size_t length, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_conference_invite", "ii" BUF_TC "#",
      friendnumber, type, data, length);
}

static void callback_conference_message(Tox *tox, uint32_t conference_number,
    uint32_t peer_number, TOX_MESSAGE_TYPE type, const uint8_t* message, size_t length, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_conference_message", "iiis#", conference_number,
      peer_number, type, message, length - (message[length - 1] == 0));
}

static void callback_conference_namelist_change(Tox *tox, uint32_t conference_number,
    uint32_t peer_number, TOX_CONFERENCE_STATE_CHANGE change, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_conference_namelist_change", "iii",
      conference_number, peer_number, change);
}

static void callback_file_chunk_request(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                        uint64_t position, size_t length, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_file_chunk_request", "iiKi",
                        friend_number, file_number, position, length);
}


static void callback_file_recv(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind,
                               uint64_t file_size,
                               const uint8_t *filename, size_t filename_length, void *self)
{
    if (kind == TOX_FILE_KIND_AVATAR && filename != NULL) {
        assert(TOX_HASH_LENGTH == filename_length);
        char filename_hex[TOX_HASH_LENGTH * 2 + 1];
        memset(filename_hex, 0, TOX_HASH_LENGTH * 2 + 1);
        bytes_to_hex_string(filename, filename_length, (uint8_t*)filename_hex);

        PyObject_CallMethod((PyObject*)self, "on_file_recv", "iiiKs#",
                            friend_number, file_number, kind, file_size, filename_hex, TOX_HASH_LENGTH * 2);
    } else {
        PyObject_CallMethod((PyObject*)self, "on_file_recv", "iiiKs#",
                            friend_number, file_number, kind, file_size, filename, filename_length);
    }
}

static void callback_file_recv_control(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                       TOX_FILE_CONTROL control, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_file_recv_control", "iii",
                        friend_number, file_number, control);
}

static void callback_file_recv_chunk(Tox *tox, uint32_t friend_number, uint32_t file_number,
                                     uint64_t position,
                                     const uint8_t *data, size_t length, void *self)
{
    PyObject_CallMethod((PyObject*)self, "on_file_recv_chunk", "iiK" BUF_TC "#",
                        friend_number, file_number, position, data, length);
}

static void init_options(ToxCore* self, PyObject* pyopts, struct Tox_Options* tox_opts)
{
    char *buf = NULL;
    Py_ssize_t sz = 0;
    PyObject *p = NULL;

    p = PyObject_GetAttrString(pyopts, "savedata_data");
    PyBytes_AsStringAndSize(p, &buf, &sz);
    if (sz > 0) {
        uint8_t *savedata_data = calloc(1, sz); /* XXX: Memory leak! */
        memcpy(savedata_data, buf, sz);
        tox_opts->savedata_data = savedata_data;
        tox_opts->savedata_length = sz;
        tox_opts->savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
    }

    p = PyObject_GetAttrString(pyopts, "proxy_host");
    PyStringUnicode_AsStringAndSize(p, &buf, &sz);
    if (sz > 0) {
        char *proxy_host = calloc(1, sz); /* XXX: Memory leak! */
        memcpy(proxy_host, buf, sz);
        tox_opts->proxy_host = proxy_host;
    }

    p = PyObject_GetAttrString(pyopts, "proxy_port");
    if (p) {
        tox_opts->proxy_port = PyLong_AsLong(p);
    }

    p = PyObject_GetAttrString(pyopts, "proxy_type");
    if (p) {
        tox_opts->proxy_type = PyLong_AsLong(p);
    }

    p = PyObject_GetAttrString(pyopts, "ipv6_enabled");
    if (p) {
        tox_opts->ipv6_enabled = p == Py_True;
    }

    p = PyObject_GetAttrString(pyopts, "udp_enabled");
    if (p) {
        tox_opts->udp_enabled = p == Py_True;
    }

    p = PyObject_GetAttrString(pyopts, "start_port");
    if (p) {
        tox_opts->start_port = PyLong_AsLong(p);
    }

    p = PyObject_GetAttrString(pyopts, "end_port");
    if (p) {
        tox_opts->end_port = PyLong_AsLong(p);
    }

    p = PyObject_GetAttrString(pyopts, "tcp_port");
    if (p) {
        tox_opts->tcp_port = PyLong_AsLong(p);
    }

    tox_opts->log_callback = callback_log;
    tox_opts->log_user_data = self;
}

static int init_helper(ToxCore* self, PyObject* args)
{
  if (self->tox != NULL) {
    tox_kill(self->tox);
    self->tox = NULL;
  }

  PyObject *opts = NULL;

  if (args) {
      if (!PyArg_ParseTuple(args, "O", &opts)) {
          PyErr_SetString(PyExc_TypeError, "must supply a ToxOptions param");
          return -1;
      }
  }

  struct Tox_Options options = {0};
  tox_options_default(&options);

  if (opts != NULL) {
      init_options(self, opts, &options);
  }

  TOX_ERR_NEW err = 0;
  Tox* tox = tox_new(&options, &err);

  if (tox == NULL) {
      PyErr_Format(ToxOpError, "failed to initialize toxcore: %d", err);
      return -1;
  }

  tox_callback_self_connection_status(tox, callback_self_connection_status);
  tox_callback_friend_request(tox, callback_friend_request);
  tox_callback_friend_message(tox, callback_friend_message);
  tox_callback_friend_name(tox, callback_friend_name);
  tox_callback_friend_status_message(tox, callback_friend_status_message);
  tox_callback_friend_status(tox, callback_friend_status);
  tox_callback_friend_typing(tox, callback_friend_typing);
  tox_callback_friend_read_receipt(tox, callback_friend_read_receipt);
  tox_callback_friend_connection_status(tox, callback_friend_connection_status);
  tox_callback_conference_invite(tox, callback_conference_invite);
  tox_callback_conference_message(tox, callback_conference_message);
  tox_callback_conference_namelist_change(tox, callback_conference_namelist_change);
  tox_callback_file_chunk_request(tox, callback_file_chunk_request);
  tox_callback_file_recv_control(tox, callback_file_recv_control);
  tox_callback_file_recv(tox, callback_file_recv);
  tox_callback_file_recv_chunk(tox, callback_file_recv_chunk);

  self->tox = tox;

  return 0;
}

static PyObject*
ToxCore_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
  ToxCore* self = (ToxCore*)type->tp_alloc(type, 0);
  self->tox = NULL;

  /* We don't care about subclass's arguments */
  if (init_helper(self, NULL) == -1) {
    return NULL;
  }

  return (PyObject*)self;
}

static int ToxCore_init(ToxCore* self, PyObject* args, PyObject* kwds)
{
  /* since __init__ in Python is optional(superclass need to call it
   * explicitly), we need to initialize self->tox in ToxCore_new instead of
   * init. If ToxCore_init is called, we re-initialize self->tox and pass
   * the new ipv6enabled setting. */
  return init_helper(self, args);
}

static int
ToxCore_dealloc(ToxCore* self)
{
  if (self->tox) {
    tox_kill(self->tox);
    self->tox = NULL;
  }
  return 0;
}

static PyObject*
ToxCore_callback_stub(ToxCore* self, PyObject* args)
{
  Py_RETURN_NONE;
}

static PyObject*
ToxCore_self_get_address(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t address[TOX_ADDRESS_SIZE];
  uint8_t address_hex[TOX_ADDRESS_SIZE * 2 + 1];
  memset(address_hex, 0, TOX_ADDRESS_SIZE * 2 + 1);

  tox_self_get_address(self->tox, address);
  bytes_to_hex_string(address, TOX_ADDRESS_SIZE, address_hex);

  return PYSTRING_FromString((const char*)address_hex);
}

static PyObject*
ToxCore_friend_add(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* address = NULL;
  int addr_length = 0;
  uint8_t* data = NULL;
  int data_length = 0;

  if (!PyArg_ParseTuple(args, "s#s#", &address, &addr_length, &data,
        &data_length)) {
    return NULL;
  }

  uint8_t pk[TOX_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_ADDRESS_SIZE, pk);

  TOX_ERR_FRIEND_ADD err = 0;
  uint32_t friend_number = 0;
  friend_number = tox_friend_add(self->tox, pk, data, data_length, &err);
  int success = friend_number == UINT32_MAX ? 0 : 1;

  switch (err) {
  case TOX_ERR_FRIEND_ADD_TOO_LONG:
    PyErr_SetString(ToxOpError, "message too long");
    break;
  case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
    PyErr_SetString(ToxOpError, "no message specified");
    break;
  case TOX_ERR_FRIEND_ADD_OWN_KEY:
    PyErr_SetString(ToxOpError, "user's own key");
    break;
  case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
    PyErr_SetString(ToxOpError, "friend request already sent or already "
        "a friend");
    break;
  case TOX_ERR_FRIEND_ADD_NULL:
    PyErr_SetString(ToxOpError, "unknown error");
    break;
  case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
    PyErr_SetString(ToxOpError, "bad checksum in address");
    break;
  case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
    PyErr_SetString(ToxOpError, "the friend was already there but the "
        "nospam was different");
    break;
  case TOX_ERR_FRIEND_ADD_MALLOC:
    PyErr_SetString(ToxOpError, "increasing the friend list size fails");
    break;
  case TOX_ERR_FRIEND_ADD_OK:
#if 0
      success = 1;
#endif
      break;
  }

  if (success) {
    return PyLong_FromLong(friend_number);
  } else {
    return NULL;
  }
}

static PyObject*
ToxCore_friend_add_norequest(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* address = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &address, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_ADDRESS_SIZE, pk);

  TOX_ERR_FRIEND_ADD err = 0;
  int res = tox_friend_add_norequest(self->tox, pk, &err);
  if (res == -1) {
    PyErr_Format(ToxOpError, "failed to add friend: %d", err);
    return NULL;
  }

  return PyLong_FromLong(res);
}

static PyObject*
ToxCore_friend_by_public_key(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* id = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &id, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_PUBLIC_KEY_SIZE];
  hex_string_to_bytes(id, TOX_PUBLIC_KEY_SIZE, pk);

  int ret = tox_friend_by_public_key(self->tox, pk, NULL);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "no such friend");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_friend_get_public_key(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t pk[TOX_PUBLIC_KEY_SIZE + 1];
  pk[TOX_PUBLIC_KEY_SIZE] = 0;

  uint8_t hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
  memset(hex, 0, TOX_PUBLIC_KEY_SIZE * 2 + 1);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  tox_friend_get_public_key(self->tox, friend_num, pk, NULL);
  bytes_to_hex_string(pk, TOX_PUBLIC_KEY_SIZE, hex);

  return PYSTRING_FromString((const char*)hex);
}

static PyObject*
ToxCore_friend_delete(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_friend_delete(self->tox, friend_num, NULL) == false) {
    PyErr_SetString(ToxOpError, "failed to delete friend");
    return NULL;
  }

  Py_RETURN_TRUE;
}

static PyObject*
ToxCore_friend_get_connection_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_friend_get_connection_status(self->tox, friend_num, NULL);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get connection status");
    return NULL;
  }

  return PyBool_FromLong(ret);
}

static PyObject*
ToxCore_friend_exists(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_friend_exists(self->tox, friend_num);

  return PyBool_FromLong(ret);
}

static PyObject*
ToxCore_friend_send_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int msg_type = 0;
  int length = 0;
  uint8_t* message = NULL;

  if (!PyArg_ParseTuple(args, "iis#", &friend_num, &msg_type, &message, &length)) {
    return NULL;
  }

  TOX_MESSAGE_TYPE type = msg_type;
  TOX_ERR_FRIEND_SEND_MESSAGE errmsg = 0;
  uint32_t ret = tox_friend_send_message(self->tox, friend_num, type, message, length, &errmsg);
  if (ret == 0) {
     PyErr_SetString(ToxOpError, "failed to send message");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_self_set_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* name = 0;
  int length = 0;

  if (!PyArg_ParseTuple(args, "s#", &name, &length)) {
    return NULL;
  }

  if (tox_self_set_name(self->tox, name, length, NULL) == false) {
    PyErr_SetString(ToxOpError, "failed to set_name");
    return NULL;
  }

  Py_RETURN_TRUE;
}

static PyObject*
ToxCore_self_get_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  tox_self_get_name(self->tox, buf);

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_self_get_name_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ret = tox_self_get_name_size(self->tox);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get self name size");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_friend_get_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_friend_get_name(self->tox, friend_num, buf, NULL) == false) {
    PyErr_SetString(ToxOpError, "failed to get name");
    return NULL;
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_friend_get_name_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_friend_get_name_size(self->tox, friend_num, NULL);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get name size");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_self_set_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* message;
  int length;
  if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
    return NULL;
  }

  if (tox_self_set_status_message(self->tox, message, length, NULL) == false) {
    PyErr_SetString(ToxOpError, "failed to set status_message");
    return NULL;
  }

  Py_RETURN_TRUE;
}

static PyObject*
ToxCore_self_set_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int status = 0;

  if (!PyArg_ParseTuple(args, "i", &status)) {
    return NULL;
  }

  tox_self_set_status(self->tox, status);
  if (true == false) {
    PyErr_SetString(ToxOpError, "failed to set status");
    return NULL;
  }

  Py_RETURN_NONE;
}


static PyObject*
ToxCore_friend_get_status_message_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_friend_get_status_message_size(self->tox, friend_num, NULL);
  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_friend_get_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_STATUS_MESSAGE_LENGTH];
  int friend_num = 0;

  memset(buf, 0, TOX_MAX_STATUS_MESSAGE_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_friend_get_status_message(self->tox, friend_num, buf, NULL);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get status_message");
    return NULL;
  }

  buf[TOX_MAX_STATUS_MESSAGE_LENGTH -1] = 0;

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_self_get_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_STATUS_MESSAGE_LENGTH];
  memset(buf, 0, TOX_MAX_STATUS_MESSAGE_LENGTH);

  tox_self_get_status_message(self->tox, buf);
  buf[TOX_MAX_STATUS_MESSAGE_LENGTH -1] = 0;

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_self_get_status_message_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ret = tox_self_get_status_message_size(self->tox);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get self status_message size");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_friend_get_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int status = tox_friend_get_status(self->tox, friend_num, NULL);

  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_self_get_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int status = tox_self_get_status(self->tox);
  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_friend_get_last_online(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  uint64_t status = tox_friend_get_last_online(self->tox, friend_num, NULL);

  if (status == 0) {
    Py_RETURN_NONE;
  }

  PyObject* datetime = PyImport_ImportModule("datetime");
  PyObject* datetimeClass = PyObject_GetAttrString(datetime, "datetime");
  PyObject* ret = PyObject_CallMethod(datetimeClass, "fromtimestamp", "K", status);

  return ret;
}

static PyObject*
ToxCore_self_set_typing(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int is_typing = 0;

  if (!PyArg_ParseTuple(args, "ii", &friend_num, &is_typing)) {
    return NULL;
  }

  int status = tox_self_set_typing(self->tox, friend_num, is_typing, NULL);
  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_friend_get_typing(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_friend_get_typing(self->tox, friend_num, NULL)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject*
ToxCore_self_get_friend_list_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t count = tox_self_get_friend_list_size(self->tox);
  return PyLong_FromUnsignedLong(count);
}

static PyObject*
ToxCore_self_get_friend_list(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t count = tox_self_get_friend_list_size(self->tox);
  uint32_t* list = (uint32_t*)malloc(count * sizeof(uint32_t));
  if (!list) {
    return NULL;
  }

  uint32_t n = count;
  tox_self_get_friend_list(self->tox, list);

  PyObject* plist = PyList_New(0);
  if (!plist) {
    free(list);
    return NULL;
  }

  uint32_t i = 0;
  for (i = 0; i < n; ++i) {
    PyList_Append(plist, PyLong_FromLong(list[i]));
  }
  free(list);

  return plist;
}

static PyObject*
ToxCore_conference_new(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  TOX_ERR_CONFERENCE_NEW error;
  uint32_t conference_number = tox_conference_new(self->tox, &error);
  if (error != TOX_ERR_CONFERENCE_NEW_OK) {
    PyErr_SetString(ToxOpError, "failed to create conference");
  }

  return PyLong_FromLong(conference_number);
}

static PyObject*
ToxCore_conference_delete(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;

  if (!PyArg_ParseTuple(args, "i", &conference_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_DELETE error;
  tox_conference_delete(self->tox, conference_number, &error);
  if (error != TOX_ERR_CONFERENCE_DELETE_OK) {
    PyErr_SetString(ToxOpError, "failed to delete conference");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_conference_get_title(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;
  if (!PyArg_ParseTuple(args, "i", &conference_number)) {
    return NULL;
  }

  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  TOX_ERR_CONFERENCE_TITLE error;
  tox_conference_get_title(self->tox, conference_number, buf, &error);
  if (error != TOX_ERR_CONFERENCE_TITLE_OK) {
    return PYSTRING_FromString("");  /* no title. */
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_conference_set_title(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;
  uint8_t* title = NULL;
  uint32_t length = 0;

  if (!PyArg_ParseTuple(args, "is#", &conference_number, &title, &length)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_TITLE error;
  tox_conference_set_title(self->tox, conference_number, title, length, &error);
  if (error != TOX_ERR_CONFERENCE_TITLE_OK) {
    PyErr_SetString(ToxOpError, "failed to set the conference title");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_conference_get_type(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;
  if (!PyArg_ParseTuple(args, "i", &conference_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_GET_TYPE error;
  TOX_CONFERENCE_TYPE type = tox_conference_get_type(self->tox, conference_number, &error);
  if (error != TOX_ERR_CONFERENCE_GET_TYPE_OK) {
    PyErr_SetString(ToxOpError, "failed to get conference type");
  }

  return PyLong_FromLong(type);
}

static PyObject*
ToxCore_conference_peer_get_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  int conference_number = 0;
  int peer_number = 0;
  if (!PyArg_ParseTuple(args, "ii", &conference_number, &peer_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_PEER_QUERY error;
  tox_conference_peer_get_name(self->tox, conference_number,
      peer_number, buf, &error);
  if (error != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
    PyErr_SetString(ToxOpError, "failed to get conference peer name");
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_conference_invite(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_number = 0;
  int conference_number = 0;
  if (!PyArg_ParseTuple(args, "ii", &friend_number, &conference_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_INVITE error;
  tox_conference_invite(self->tox, friend_number, conference_number, &error);
  if (error != TOX_ERR_CONFERENCE_INVITE_OK) {
    PyErr_SetString(ToxOpError, "failed to invite friend");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_conference_join(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_number = 0;
  uint8_t* cookie = NULL;
  int length = 0;

  if (!PyArg_ParseTuple(args, "is#", &friend_number, &cookie, &length)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_JOIN error;
  uint32_t ret = tox_conference_join(self->tox, friend_number, cookie, length,
      &error);
  if (error != TOX_ERR_CONFERENCE_JOIN_OK) {
    PyErr_SetString(ToxOpError, "failed to join conference");
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_conference_send_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;
  int type = 0;
  uint8_t* message = NULL;
  uint32_t length = 0;

  if (!PyArg_ParseTuple(args, "iis#", &conference_number, &type, &message, &length)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_SEND_MESSAGE error;
  tox_conference_send_message(self->tox, conference_number, type, message, length, &error);
  if (error != TOX_ERR_CONFERENCE_SEND_MESSAGE_OK) {
    PyErr_SetString(ToxOpError, "failed to send conference message");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_conference_peer_number_is_ours(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;
  int peer_number = 0;

  if (!PyArg_ParseTuple(args, "ii", &conference_number, &peer_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_PEER_QUERY error;
  bool ret = tox_conference_peer_number_is_ours(self->tox, conference_number, peer_number, &error);
  if (error != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
    PyErr_SetString(ToxOpError, "failed to check if peer number is ours");
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_conference_peer_count(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int conference_number = 0;

  if (!PyArg_ParseTuple(args, "i", &conference_number)) {
    return NULL;
  }

  TOX_ERR_CONFERENCE_PEER_QUERY error;
  uint32_t count = tox_conference_peer_count(self->tox, conference_number, &error);
  if (error != TOX_ERR_CONFERENCE_PEER_QUERY_OK) {
    PyErr_SetString(ToxOpError, "failed to get conference peer count");
  }

  return PyLong_FromLong(count);
}

static PyObject*
ToxCore_conference_get_chatlist_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  size_t n = tox_conference_get_chatlist_size(self->tox);
  return PyLong_FromUnsignedLong(n);
}

static PyObject*
ToxCore_conference_get_chatlist(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t count = tox_conference_get_chatlist_size(self->tox);
  uint32_t* list = (uint32_t*)malloc(count * sizeof(uint32_t));
  if (list == NULL) {
    PyErr_SetString(ToxOpError, "allocation failure");
    Py_RETURN_NONE;
  }

  tox_conference_get_chatlist(self->tox, list);

  PyObject* plist = PyList_New(0);
  if (!plist) {
    return NULL;
  }

  uint32_t i = 0;
  for (i = 0; i < count; ++i) {
    PyList_Append(plist, PyLong_FromLong(list[i]));
  }
  free(list);

  return plist;
}

static PyObject*
ToxCore_file_send(ToxCore* self, PyObject*args)
{
    CHECK_TOX(self);
    uint32_t friend_number = 0;
    uint32_t kind = 0;
    uint64_t file_size = 0;
    uint8_t* file_id = NULL;
    uint8_t* filename = 0;
    int filename_length = 0;

    if (!PyArg_ParseTuple(args, "iiKss#", &friend_number, &kind, &file_size, &file_id,
                          &filename, &filename_length)) {
        return NULL;
    }

    TOX_ERR_FILE_SEND err = 0;
    uint32_t file_number =
        tox_file_send(self->tox, friend_number, kind, file_size, file_id, filename, filename_length, &err);
    if (file_number == UINT32_MAX) {
        PyErr_Format(ToxOpError, "tox_file_send() failed: %d", err);
        return NULL;
    }

    return PyLong_FromLong(file_number);
}

static PyObject*
ToxCore_file_control(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);
    uint32_t friend_number = 0;
    uint32_t file_number = 0;
    int control = 0;

    if (!PyArg_ParseTuple(args, "iii", &friend_number, &file_number, &control)) {
        return NULL;
    }

    bool ret = tox_file_control(self->tox, friend_number, file_number, control, NULL);
    if (!ret) {
        PyErr_SetString(ToxOpError, "tox_file_control() failed");
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}

static PyObject*
ToxCore_file_send_chunk(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);
    uint32_t friend_number = 0;
    uint32_t file_number = 0;
    uint64_t position = 0;
    uint8_t *data = 0;
    size_t length = 0;

    if (!PyArg_ParseTuple(args, "iiKs#", &friend_number, &file_number, &position,
                          &data, &length)) {
        return NULL;
    }

    TOX_ERR_FILE_SEND_CHUNK err = 0;
    bool ret = tox_file_send_chunk(self->tox, friend_number, file_number, position,
                                   data, length, &err);
    if (!ret) {
        PyErr_Format(ToxOpError, "tox_file_send_chunk() failed:%d", err);
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}

static PyObject*
ToxCore_file_seek(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);
    uint32_t friend_number = 0;
    uint32_t file_number = 0;
    uint64_t position = 0;

    if (!PyArg_ParseTuple(args, "iiK", &friend_number, &file_number, &position)) {
        return NULL;
    }

    TOX_ERR_FILE_SEEK err = 0;
    bool ret = tox_file_seek(self->tox, friend_number, file_number, position, &err);

    if (!ret) {
        PyErr_Format(ToxOpError, "tox_file_seek() failed: %d", err);
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}

static PyObject*
ToxCore_file_get_file_id(ToxCore* self, PyObject* args)
{
    CHECK_TOX(self);
    uint32_t friend_number = 0;
    uint32_t file_number = 0;

    if (!PyArg_ParseTuple(args, "ii", &friend_number, &file_number)) {
        return NULL;
    }

    uint8_t file_id[TOX_FILE_ID_LENGTH + 1];
    file_id[TOX_FILE_ID_LENGTH] = 0;

    uint8_t hex[TOX_FILE_ID_LENGTH * 2 + 1];
    memset(hex, 0, TOX_FILE_ID_LENGTH * 2 + 1);

    TOX_ERR_FILE_GET err = 0;
    bool ret = tox_file_get_file_id(self->tox, friend_number, file_number, file_id, &err);
    if (!ret) {
        PyErr_Format(ToxOpError, "tox_file_get_file_id() failed: %d", err);
        Py_RETURN_NONE;
    }

    bytes_to_hex_string(file_id, TOX_FILE_ID_LENGTH, hex);
    return PYSTRING_FromStringAndSize((char*)hex, TOX_FILE_ID_LENGTH * 2);
}

static PyObject*
ToxCore_self_get_nospam(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t nospam = tox_self_get_nospam(self->tox);

  return PyLong_FromUnsignedLongLong(nospam);
}

static PyObject*
ToxCore_self_set_nospam(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t nospam = 0;

  if (!PyArg_ParseTuple(args, "I", &nospam)) {
    return NULL;
  }

  tox_self_set_nospam(self->tox, nospam);
  Py_RETURN_NONE;
}

static PyObject*
ToxCore_self_get_keys(ToxCore* self, PyObject* args)
{
  uint8_t public_key[TOX_PUBLIC_KEY_SIZE];
  uint8_t secret_key[TOX_PUBLIC_KEY_SIZE];

  uint8_t public_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
  uint8_t secret_key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];

  tox_self_get_public_key(self->tox, public_key);
  tox_self_get_secret_key(self->tox, secret_key);
  bytes_to_hex_string(public_key, TOX_PUBLIC_KEY_SIZE, public_key_hex);
  bytes_to_hex_string(secret_key, TOX_PUBLIC_KEY_SIZE, secret_key_hex);

  PyObject* res = PyTuple_New(2);
  PyTuple_SetItem(res, 0, PYSTRING_FromStringAndSize((char*)public_key_hex,
        TOX_PUBLIC_KEY_SIZE * 2));
  PyTuple_SetItem(res, 1, PYSTRING_FromStringAndSize((char*)secret_key_hex,
        TOX_PUBLIC_KEY_SIZE * 2));
  return res;
}

static PyObject*
ToxCore_bootstrap(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint16_t port = 0;
  uint8_t* public_key = NULL;
  char* address = NULL;
  int addr_length = 0;
  int pk_length = 0;
  uint8_t pk[TOX_PUBLIC_KEY_SIZE];

  if (!PyArg_ParseTuple(args, "s#Hs#", &address, &addr_length, &port,
        &public_key, &pk_length)) {
    return NULL;
  }

  hex_string_to_bytes(public_key, TOX_PUBLIC_KEY_SIZE, pk);
  bool ret = tox_bootstrap(self->tox, address, port, pk, NULL);

  if (!ret) {
    PyErr_SetString(ToxOpError, "failed to resolve address");
    return NULL;
  }

  Py_RETURN_TRUE;
}

static PyObject*
ToxCore_add_tcp_relay(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint16_t port = 0;
  uint8_t* public_key = NULL;
  char* address = NULL;
  int addr_length = 0;
  int pk_length = 0;
  uint8_t pk[TOX_PUBLIC_KEY_SIZE];

  if (!PyArg_ParseTuple(args, "s#Hs#", &address, &addr_length, &port,
        &public_key, &pk_length)) {
    return NULL;
  }

  hex_string_to_bytes(public_key, TOX_PUBLIC_KEY_SIZE, pk);
  bool ret = tox_add_tcp_relay(self->tox, address, port, pk, NULL);

  if (!ret) {
    PyErr_SetString(ToxOpError, "failed to add tcp relay");
    return NULL;
  }

  Py_RETURN_TRUE;
}

static PyObject*
ToxCore_self_get_connection_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  TOX_CONNECTION conn = tox_self_get_connection_status(self->tox);

  return PyLong_FromLong(conn);
}

static PyObject*
ToxCore_kill(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  tox_kill(self->tox);
  self->tox = NULL;

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_iteration_interval(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t interval = tox_iteration_interval(self->tox);

  return PyLong_FromUnsignedLongLong(interval);
}

static PyObject*
ToxCore_iterate(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  tox_iterate(self->tox, self);

  if (PyErr_Occurred()) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject*
ToxCore_get_savedata_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t size = tox_get_savedata_size(self->tox);

  return PyLong_FromUnsignedLong(size);
}

static PyObject*
ToxCore_get_savedata(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t size = tox_get_savedata_size(self->tox);
  uint8_t* buf = (uint8_t*)malloc(size);
  tox_get_savedata(self->tox, buf);

  PyObject* res = PYBYTES_FromStringAndSize((const char*)buf, size);
  free(buf);

  return res;
}

static PyMethodDef Tox_methods[] = {
  {
    "on_log", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_log(level, file, line, func, message)\n"
    "Callback for internal log messages, default implementation does "
    "nothing."
  },
  {
    "on_self_connection_status", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_self_connection_status(friend_number, status)\n"
    "Callback for receiving read receipt, default implementation does "
    "nothing."
  },
  {
    "on_friend_request", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_request(address, message)\n"
    "Callback for receiving friend requests, default implementation does "
    "nothing."
  },
  {
    "on_friend_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_message(friend_number, type, message)\n"
    "Callback for receiving friend messages, default implementation does "
    "nothing."
  },
  {
    "on_friend_name", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_name(friend_number, new_name)\n"
    "Callback for receiving friend name changes, default implementation does "
    "nothing."
  },
  {
    "on_friend_status_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_status_message(friend_number, new_status)\n"
    "Callback for receiving friend status message changes, default "
    "implementation does nothing."
  },
  {
    "on_friend_status", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_status(friend_number, kind)\n"
    "Callback for receiving friend status changes, default implementation "
    "does nothing."
  },
  {
    "on_friend_typing", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_typing(friend_number, is_typing)\n"
    "Callback for receiving friend status changes, default implementation "
    "does nothing.\n\n"
  },
  {
    "on_friend_read_receipt", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_read_receipt(friend_number, receipt)\n"
    "Callback for receiving read receipt, default implementation does nothing."
  },
  {
    "on_friend_connection_status", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_connection_status(friend_number, status)\n"
    "Callback for receiving read receipt, default implementation does "
    "nothing.\n\n"
    "*status* is a boolean value which indicates the status of the friend "
    "indicated by *friend_number*. True means online and False means offline "
    "after previously online."
  },
  {
    "on_conference_invite", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_conference_invite(friend_number, type, cookie)\n"
    "Callback for receiving conference invitations, default implementation does "
    "nothing.\n\n"
    ".. seealso ::\n"
    "    :meth:`.conference_get_type`"
  },
  {
    "on_conference_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_conference_message(conference_number, peer_number, message)\n"
    "Callback for receiving conference messages, default implementation does "
    "nothing."
  },
  {
    "on_conference_namelist_change", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_conference_namelist_change(conference_number, peer_number, change)\n"
    "Callback for joins/parts/name changes, default implementation does "
    "nothing.\n\n"
    "There are there possible *change* values:\n\n"
    "+----------------------------------------------+----------------------+\n"
    "| change                                       | description          |\n"
    "+==============================================+======================+\n"
    "| Tox.CONFERENCE_STATE_CHANGE_PEER_JOIN        | a peer is added      |\n"
    "+----------------------------------------------+----------------------+\n"
    "| Tox.CONFERENCE_STATE_CHANGE_PEER_EXIT        | a peer is deleted    |\n"
    "+----------------------------------------------+----------------------+\n"
    "| Tox.CONFERENCE_STATE_CHANGE_PEER_NAME_CHANGE | name of peer changed |\n"
    "+----------------------------------------------+----------------------+\n"
  },
  {
    "on_file_recv", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_recv(friend_number, file_number,  kind, file_size, filename)\n"
    "Callback for receiving file transfer, default implementation does nothing."
  },
  {
    "on_file_recv_control", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_recv_control(friend_number, file_number, control)\n"
    "Callback for receiving file control, default implementation does nothing."
  },
  {
    "on_file_recv_chunk", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_recv_chunk(friend_number, file_number, position, data)\n"
    "Callback for receiving file chunk, default implementation does nothing."
  },
  {
    "on_file_chunk_request", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_chunk_request(friend_number, file_number, position, length)\n"
    "Callback for more file chunk, default implementation does nothing."
  },
  {
    "self_get_address", (PyCFunction)ToxCore_self_get_address, METH_NOARGS,
    "self_get_address()\n"
    "Return address to give to others."
  },
  {
    "friend_add", (PyCFunction)ToxCore_friend_add, METH_VARARGS,
    "friend_add(address, message)\n"
    "Add a friend."
  },
  {
    "friend_add_norequest", (PyCFunction)ToxCore_friend_add_norequest, METH_VARARGS,
    "friend_add_norequest(address)\n"
    "Add a friend without sending request."
  },
  {
    "friend_by_public_key", (PyCFunction)ToxCore_friend_by_public_key, METH_VARARGS,
    "friend_by_public_key(friend_id)\n"
    "Return the friend id associated to that client id."
  },
  {
    "friend_get_public_key", (PyCFunction)ToxCore_friend_get_public_key, METH_VARARGS,
    "friend_get_public_key(friend_number)\n"
    "Return the public key associated to that friend number."
  },
  {
    "friend_delete", (PyCFunction)ToxCore_friend_delete, METH_VARARGS,
    "friend_delete(friend_number)\n"
    "Remove a friend."
  },
  {
    "friend_get_connection_status", (PyCFunction)ToxCore_friend_get_connection_status, METH_VARARGS,
    "friend_get_connection_status(friend_number)\n"
    "Return True if friend is connected(Online) else False."
  },
  {
    "friend_exists", (PyCFunction)ToxCore_friend_exists, METH_VARARGS,
    "friend_exists(friend_number)\n"
    "Checks if there exists a friend with given friendnumber."
  },
  {
    "friend_send_message", (PyCFunction)ToxCore_friend_send_message, METH_VARARGS,
    "friend_send_message(friend_number, type, message)\n"
    "Send a text chat message to an online friend."
  },
  {
    "self_set_name", (PyCFunction)ToxCore_self_set_name, METH_VARARGS,
    "self_set_name(name)\n"
    "Set our self nickname."
  },
  {
    "self_get_name", (PyCFunction)ToxCore_self_get_name, METH_NOARGS,
    "self_get_name()\n"
    "Get our self nickname."
  },
  {
    "self_get_name_size", (PyCFunction)ToxCore_self_get_name_size, METH_NOARGS,
    "self_get_name_size()\n"
    "Get our self nickname string length"
  },
  {
    "friend_get_name", (PyCFunction)ToxCore_friend_get_name, METH_VARARGS,
    "friend_get_name(friend_number)\n"
    "Get nickname of *friend_number*."
  },
  {
    "friend_get_name_size", (PyCFunction)ToxCore_friend_get_name_size, METH_VARARGS,
    "friend_get_name_size(friend_number)\n"
    "Get nickname length of *friend_number*."
  },
  {
    "self_set_status_message", (PyCFunction)ToxCore_self_set_status_message, METH_VARARGS,
    "self_set_status_message(message)\n"
    "Set our self status message."
  },
  {
    "self_set_status", (PyCFunction)ToxCore_self_set_status, METH_VARARGS,
    "self_set_status(status)\n"
    "Set our user status, status can have following values:\n\n"
    "+------------------------+--------------------+\n"
    "| kind                   | description        |\n"
    "+========================+====================+\n"
    "| Tox.USERSTATUS_NONE    | the user is online |\n"
    "+------------------------+--------------------+\n"
    "| Tox.USERSTATUS_AWAY    | the user is away   |\n"
    "+------------------------+--------------------+\n"
    "| Tox.USERSTATUS_BUSY    | the user is busy   |\n"
    "+------------------------+--------------------+\n"
    "| Tox.USERSTATUS_INVALID | invalid status     |\n"
    "+------------------------+--------------------+\n"
  },
  {
    "friend_get_status_message_size", (PyCFunction)ToxCore_friend_get_status_message_size, METH_VARARGS,
    "friend_get_status_message_size(friend_number)\n"
    "Return the length of *friend_number*'s status message."
  },
  {
    "friend_get_status_message", (PyCFunction)ToxCore_friend_get_status_message, METH_VARARGS,
    "friend_get_status_message(friend_number)\n"
    "Get status message of a friend."
  },
  {
    "self_get_status_message", (PyCFunction)ToxCore_self_get_status_message,
    METH_NOARGS,
    "self_get_status_message()\n"
    "Get status message of yourself."
  },
  {
    "self_get_status_message_size",
    (PyCFunction)ToxCore_self_get_status_message_size,
    METH_NOARGS,
    "self_get_status_message_size()\n"
    "Get status message string length of yourself."
  },
  {
    "friend_get_status", (PyCFunction)ToxCore_friend_get_status, METH_VARARGS,
    "friend_get_status(friend_number)\n"
    "Get friend status.\n\n"
    ".. seealso ::\n"
    "    :meth:`.set_user_status`"
  },
  {
    "self_get_status", (PyCFunction)ToxCore_self_get_status,
    METH_NOARGS,
    "self_get_status()\n"
    "Get user status of youself.\n\n"
    ".. seealso ::\n"
    "    :meth:`.set_user_status`"
  },
  {
    "friend_get_last_online", (PyCFunction)ToxCore_friend_get_last_online, METH_VARARGS,
    "friend_get_last_online(friend_number)\n"
    "returns datetime.datetime object representing the last time "
    "*friend_number* was seen online, or None if never seen."
  },
  {
    "self_set_typing", (PyCFunction)ToxCore_self_set_typing, METH_VARARGS,
    "self_set_typing(friend_number, is_typing)\n"
    "Set user typing status.\n\n"
  },
  {
    "friend_get_typing", (PyCFunction)ToxCore_friend_get_typing, METH_VARARGS,
    "friend_get_typing(friend_number)\n"
    "Return True is user is typing.\n\n"
  },
  {
    "self_get_friend_list_size", (PyCFunction)ToxCore_self_get_friend_list_size,
    METH_NOARGS,
    "self_get_friend_list_size()\n"
    "Return the number of friends."
  },
  {
    "self_get_friend_list", (PyCFunction)ToxCore_self_get_friend_list,
    METH_NOARGS,
    "self_get_friend_list()\n"
    "Get a list of valid friend numbers."
  },
  {
    "conference_get_title", (PyCFunction)ToxCore_conference_get_title, METH_VARARGS,
    "conference_get_title(conference_number)\n"
    "Returns the title for a conference."
  },
  {
    "conference_set_title", (PyCFunction)ToxCore_conference_set_title, METH_VARARGS,
    "conference_set_title(conference_number, title)\n"
    "Sets the title for a conference."
  },
  {
    "conference_get_type", (PyCFunction)ToxCore_conference_get_type, METH_VARARGS,
    "conference_get_type(conference_number)\n"
    "Return the type of conference, could be the following value:\n\n"
    "+--------------------------+-------------+\n"
    "| type                     | description |\n"
    "+==========================+=============+\n"
    "| Tox.CONFERENCE_TYPE_TEXT | text chat   |\n"
    "+--------------------------+-------------+\n"
    "| Tox.CONFERENCE_TYPE_AV   | video chat  |\n"
    "+--------------------------+-------------+\n"
  },
  {
    "conference_new", (PyCFunction)ToxCore_conference_new, METH_VARARGS,
    "conference_new()\n"
    "Creates a new conference and puts it in the chats array."
  },
  {
    "conference_delete", (PyCFunction)ToxCore_conference_delete, METH_VARARGS,
    "conference_delete(conference_number)\n"
    "Delete a conference from the chats array."
  },
  {
    "conference_peer_get_name", (PyCFunction)ToxCore_conference_peer_get_name, METH_VARARGS,
    "conference_peer_get_name(conference_number, peer_number)\n"
    "Get the conference peer's name."
  },
  {
    "conference_invite", (PyCFunction)ToxCore_conference_invite, METH_VARARGS,
    "conference_invite(friend_number, conference_number)\n"
    "Invite friend_number to conference_number."
  },
  {
    "conference_join", (PyCFunction)ToxCore_conference_join, METH_VARARGS,
    "conference_join(friend_number, cookie)\n"
    "Join a conference (you need to have been invited first.). Returns the "
    "conference number of success."
  },
  {
    "conference_send_message", (PyCFunction)ToxCore_conference_send_message, METH_VARARGS,
    "conference_send_message(conference_number, type, message)\n"
    "Send a conference message."
  },
  {
    "conference_peer_count", (PyCFunction)ToxCore_conference_peer_count, METH_VARARGS,
    "conference_peer_count(conference_number)\n"
    "Return the number of peers in the conference."
  },
  {
    "conference_get_chatlist_size", (PyCFunction)ToxCore_conference_get_chatlist_size, METH_VARARGS,
    "conference_get_chatlist_size()\n"
    "Return the number of conferences in the current Tox instance."
  },
  {
    "conference_peer_number_is_ours", (PyCFunction)ToxCore_conference_peer_number_is_ours, METH_VARARGS,
    "conference_peer_number_is_ours(conference_number, peer_number)\n"
    "Check if the current peer number corresponds to ours."
  },
  {
    "conference_get_chatlist", (PyCFunction)ToxCore_conference_get_chatlist, METH_VARARGS,
    "conference_get_chatlist()\n"
    "Return a list of valid conference numbers."
  },
  {
    "file_send", (PyCFunction)ToxCore_file_send, METH_VARARGS,
    "file_send(friend_number, kind, file_size, file_id, filename)\n"
    "Send a file send request. Returns file number to be sent."
  },
  {
    "file_control", (PyCFunction)ToxCore_file_control, METH_VARARGS,
    "file_control(friend_number, file_number, control)\n"
    "Send a file send request. Returns file number to be sent."
  },
  {
    "file_send_chunk", (PyCFunction)ToxCore_file_send_chunk, METH_VARARGS,
    "file_send_chunk(friend_number, file_number, position, data)\n"
    "Send a file send request. Returns file number to be sent."
  },
  {
    "file_seek", (PyCFunction)ToxCore_file_seek, METH_VARARGS,
    "file_seek(friend_number, file_number, position)\n"
    "Send a file send request. Returns file number to be sent."
  },
  {
    "file_get_file_id", (PyCFunction)ToxCore_file_get_file_id, METH_VARARGS,
    "file_get_file_id(friend_number, file_number)\n"
    "Send a file send request. Returns file id's hex string"
  },
  {
    "self_get_nospam", (PyCFunction)ToxCore_self_get_nospam,
    METH_NOARGS,
    "self_get_nospam()\n"
    "get nospam part from ID"
  },
  {
    "self_set_nospam", (PyCFunction)ToxCore_self_set_nospam, METH_VARARGS,
    "self_set_nospam(nospam)\n"
    "set nospam part of ID. *nospam* should be of type uint32"
  },
  {
    "self_get_keys", (PyCFunction)ToxCore_self_get_keys,
    METH_NOARGS,
    "self_get_keys()\n"
    "Get the public and secret key from the Tox object. Return a tuple "
    "(public_key, secret_key)"
  },
  {
    "bootstrap", (PyCFunction)ToxCore_bootstrap, METH_VARARGS,
    "bootstrap(address, port, public_key)\n"
    "Resolves address into an IP address. If successful, sends a 'get nodes'"
    "request to the given node with ip, port."
  },
  {
    "add_tcp_relay", (PyCFunction)ToxCore_add_tcp_relay, METH_VARARGS,
    "add_tcp_relay(address, port, public_key)\n"
    ""
  },
  {
    "self_get_connection_status", (PyCFunction)ToxCore_self_get_connection_status, METH_NOARGS,
    "self_get_connection_status()\n"
    "Return False if we are not connected to the DHT."
  },
  {
    "kill", (PyCFunction)ToxCore_kill, METH_NOARGS,
    "kill()\n"
    "Run this before closing shop."
  },
  {
    "iteration_interval", (PyCFunction)ToxCore_iteration_interval, METH_NOARGS,
    "iteration_interval()\n"
    "returns time (in ms) before the next tox_iterate() needs to be run on success."
  },
  {
    "iterate", (PyCFunction)ToxCore_iterate, METH_NOARGS,
    "iterate()\n"
    "The main loop that needs to be run at least 20 times per second."
  },
  {
    "get_savedata_size", (PyCFunction)ToxCore_get_savedata_size, METH_NOARGS,
    "get_savedata_size()\n"
    "return size of messenger data (for saving)."
  },
  {
    "get_savedata", (PyCFunction)ToxCore_get_savedata, METH_NOARGS,
    "get_savedata()\n"
    "Return messenger blob in str."
  },
  {
    NULL, NULL, 0,
    NULL,
  }
};

PyTypeObject ToxCoreType = {
#if PY_MAJOR_VERSION >= 3
  PyVarObject_HEAD_INIT(NULL, 0)
#else
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
#endif
  "Tox",                     /*tp_name*/
  sizeof(ToxCore),           /*tp_basicsize*/
  0,                         /*tp_itemsize*/
  (destructor)ToxCore_dealloc, /*tp_dealloc*/
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
  "ToxCore object",          /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Tox_methods,               /* tp_methods */
  0,                         /* tp_members */
  0,                         /* tp_getset */
  0,                         /* tp_base */
  0,                         /* tp_dict */
  0,                         /* tp_descr_get */
  0,                         /* tp_descr_set */
  0,                         /* tp_dictoffset */
  (initproc)ToxCore_init,    /* tp_init */
  0,                         /* tp_alloc */
  ToxCore_new,               /* tp_new */
};

void ToxCore_install_dict()
{
#define SET(name)                                            \
    PyObject* obj_##name = PyLong_FromLong(TOX_##name);      \
    PyDict_SetItemString(dict, #name, obj_##name);           \
    Py_DECREF(obj_##name);

    PyObject* dict = PyDict_New();
    SET(ERR_FRIEND_ADD_TOO_LONG)
    SET(ERR_FRIEND_ADD_NO_MESSAGE)
    SET(ERR_FRIEND_ADD_OWN_KEY)
    SET(ERR_FRIEND_ADD_ALREADY_SENT)
    SET(ERR_FRIEND_ADD_NULL)
    SET(ERR_FRIEND_ADD_BAD_CHECKSUM)
    SET(ERR_FRIEND_ADD_SET_NEW_NOSPAM)
    SET(ERR_FRIEND_ADD_MALLOC)
    SET(CONNECTION_NONE)
    SET(CONNECTION_TCP)
    SET(CONNECTION_UDP)
    SET(PROXY_TYPE_NONE)
    SET(PROXY_TYPE_HTTP)
    SET(PROXY_TYPE_SOCKS5)
    SET(MESSAGE_TYPE_NORMAL)
    SET(MESSAGE_TYPE_ACTION)
    SET(SAVEDATA_TYPE_NONE)
    SET(SAVEDATA_TYPE_TOX_SAVE)
    SET(SAVEDATA_TYPE_SECRET_KEY)
    SET(USER_STATUS_NONE)
    SET(USER_STATUS_AWAY)
    SET(USER_STATUS_BUSY)
    SET(CONFERENCE_STATE_CHANGE_PEER_JOIN)
    SET(CONFERENCE_STATE_CHANGE_PEER_EXIT)
    SET(CONFERENCE_STATE_CHANGE_PEER_NAME_CHANGE)
    SET(FILE_KIND_DATA)
    SET(FILE_KIND_AVATAR)
    SET(FILE_CONTROL_RESUME)
    SET(FILE_CONTROL_PAUSE)
    SET(FILE_CONTROL_CANCEL)
    SET(CONFERENCE_TYPE_TEXT)
    SET(CONFERENCE_TYPE_AV)
    SET(LOG_LEVEL_TRACE)
    SET(LOG_LEVEL_DEBUG)
    SET(LOG_LEVEL_INFO)
    SET(LOG_LEVEL_WARNING)
    SET(LOG_LEVEL_ERROR)

#undef SET

  ToxCoreType.tp_dict = dict;
}
