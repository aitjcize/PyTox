/**
 * @file   core.c
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

#include "core.h"
#include "util.h"

#include <arpa/inet.h>

extern PyObject* ToxOpError;

static void callback_friend_request(Tox* tox, const uint8_t* public_key,
    const uint8_t* data, uint16_t length, void* self)
{
  uint8_t buf[TOX_CLIENT_ID_SIZE * 2 + 1];
  memset(buf, 0, TOX_CLIENT_ID_SIZE * 2 + 1);

  bytes_to_hex_string(public_key, TOX_CLIENT_ID_SIZE, buf);

  PyObject_CallMethod((PyObject*)self, "on_friend_request", "ss#", buf, data,
      length - (data[length - 1] == 0));
}

static void callback_friend_message(Tox *tox, int32_t friendnumber,
    uint8_t* message, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_message", "is#", friendnumber,
      message, length - (message[length - 1] == 0));
}

static void callback_action(Tox *tox, int32_t friendnumber, uint8_t* action,
    uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friend_action", "is#", friendnumber,
      action, length - (action[length - 1] == 0));
}

static void callback_name_change(Tox *tox, int32_t friendnumber,
    uint8_t* newname, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_name_change", "is#", friendnumber,
      newname, length - (newname[length - 1] == 0));
}

static void callback_status_message(Tox *tox, int32_t friendnumber,
    uint8_t *newstatus, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_status_message", "is#", friendnumber,
      newstatus, length - (newstatus[length - 1] == 0));
}

static void callback_user_status(Tox *tox, int32_t friendnumber,
    uint8_t kind, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_user_status", "ii", friendnumber,
      kind);
}

static void callback_typing_change(Tox *tox, int32_t friendnumber,
    uint8_t is_typing, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_typing_change", "iO", friendnumber,
      PyBool_FromLong(is_typing));
}

static void callback_read_receipt(Tox *tox, int32_t friendnumber,
    uint32_t receipt, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_read_receipt", "ii", friendnumber,
      receipt);
}

static void callback_connection_status(Tox *tox, int32_t friendnumber,
    uint8_t status, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_connection_status", "iO",
      friendnumber, PyBool_FromLong(status));
}

static void callback_group_invite(Tox *tox, int32_t friendnumber,
    uint8_t* group_public_key, void *self)
{
  uint8_t gpk[TOX_CLIENT_ID_SIZE * 2 + 1];
  memset(gpk, 0, TOX_CLIENT_ID_SIZE * 2 + 1);

  bytes_to_hex_string(group_public_key, TOX_CLIENT_ID_SIZE, gpk);
  PyObject_CallMethod((PyObject*)self, "on_group_invite", "is", friendnumber,
      gpk);
}

static void callback_group_message(Tox *tox, int groupid,
    int friendgroupid, uint8_t* message, uint16_t length, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_message", "iis#", groupid,
      friendgroupid, message, length - (message[length - 1] == 0));
}

static void callback_group_action(Tox *tox, int groupid,
    int friendgroupid, uint8_t* action, uint16_t length, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_action", "iis#", groupid,
      friendgroupid, action, length - (action[length - 1] == 0));
}

static void callback_group_namelist_change(Tox *tox, int groupid,
    int peernumber, uint8_t change, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_namelist_change", "iii",
      groupid, peernumber, change);
}

static void callback_file_send_request(Tox *m, int32_t friendnumber,
    uint8_t filenumber, uint64_t filesize, uint8_t* filename,
    uint16_t filename_length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_file_send_request", "iiKs#",
      friendnumber, filenumber, filesize, filename,
      filename_length - (filename[filename_length - 1] == 0));
}

static void callback_file_control(Tox *m, int32_t friendnumber,
    uint8_t receive_send, uint8_t filenumber, uint8_t control_type,
    uint8_t* data, uint16_t length, void* self)
{
#if PY_MAJOR_VERSION < 3
  PyObject_CallMethod((PyObject*)self, "on_file_control", "iiiis#",
      friendnumber, receive_send, filenumber, control_type, data, length);
#else
  PyObject_CallMethod((PyObject*)self, "on_file_control", "iiiiy#",
      friendnumber, receive_send, filenumber, control_type, data, length);
#endif
}

static void callback_file_data(Tox *m, int32_t friendnumber, uint8_t filenumber,
    uint8_t* data, uint16_t length, void* self)
{
#if PY_MAJOR_VERSION < 3
  PyObject_CallMethod((PyObject*)self, "on_file_data", "iis#",
      friendnumber, filenumber, data, length);
#else
  PyObject_CallMethod((PyObject*)self, "on_file_data", "iiy#",
      friendnumber, filenumber, data, length);
#endif
}

static int init_helper(ToxCore* self, PyObject* args)
{
  if (self->tox != NULL) {
    tox_kill(self->tox);
    self->tox = NULL;
  }

  int ipv6enabled = TOX_ENABLE_IPV6_DEFAULT;

  if (args) {
    if (!PyArg_ParseTuple(args, "|i", &ipv6enabled)) {
      PyErr_SetString(PyExc_TypeError, "ipv6enabled should be boolean");
      return -1;
    }
  }

  Tox* tox = tox_new(ipv6enabled);
  if (tox == NULL) {
      fprintf(stderr, "Warning: failed to initialize toxcore with ipv6, "
          "trying ipv4.\n");

      tox = tox_new(0);
      if (tox == NULL) {
        PyErr_SetString(ToxOpError, "failed to initialize toxcore");
        return -1;
      }
  }

  tox_callback_friend_request(tox, callback_friend_request, self);
  tox_callback_friend_message(tox, callback_friend_message, self);
  tox_callback_friend_action(tox, callback_action, self);
  tox_callback_name_change(tox, callback_name_change, self);
  tox_callback_status_message(tox, callback_status_message, self);
  tox_callback_user_status(tox, callback_user_status, self);
  tox_callback_typing_change(tox, callback_typing_change, self);
  tox_callback_read_receipt(tox, callback_read_receipt, self);
  tox_callback_connection_status(tox, callback_connection_status, self);
  tox_callback_group_invite(tox, callback_group_invite, self);
  tox_callback_group_message(tox, callback_group_message, self);
  tox_callback_group_action(tox, callback_group_action, self);
  tox_callback_group_namelist_change(tox, callback_group_namelist_change, self);
  tox_callback_file_send_request(tox, callback_file_send_request, self);
  tox_callback_file_control(tox, callback_file_control, self);
  tox_callback_file_data(tox, callback_file_data, self);

  self->tox = tox;

  return 0;
}

static PyObject*
ToxCore_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
  ToxCore* self = (ToxCore*)type->tp_alloc(type, 0);
  self->tox = NULL;

  // We don't care about subclass's arguments
  if (init_helper(self, NULL) == -1) {
    return NULL;
  }

  return (PyObject*)self;
}

static int ToxCore_init(ToxCore* self, PyObject* args, PyObject* kwds)
{
  // since __init__ in Python is optional(superclass need to call it
  // explicitly), we need to initialize self->tox in ToxCore_new instead of
  // init. If ToxCore_init is called, we re-initialize self->tox and pass
  // the new ipv6enabled setting.
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
ToxCore_getaddress(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t address[TOX_FRIEND_ADDRESS_SIZE];
  uint8_t address_hex[TOX_FRIEND_ADDRESS_SIZE * 2 + 1];
  memset(address_hex, 0, TOX_FRIEND_ADDRESS_SIZE * 2 + 1);

  tox_get_address(self->tox, address);
  bytes_to_hex_string(address, TOX_FRIEND_ADDRESS_SIZE, address_hex);

  return PYSTRING_FromString((const char*)address_hex);
}

static PyObject*
ToxCore_add_friend(ToxCore* self, PyObject* args)
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

  uint8_t pk[TOX_FRIEND_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_FRIEND_ADDRESS_SIZE, pk);

  int ret = tox_add_friend(self->tox, pk, data, data_length);
  int success = 0;

  switch (ret) {
  case TOX_FAERR_TOOLONG:
    PyErr_SetString(ToxOpError, "message too long");
    break;
  case TOX_FAERR_NOMESSAGE:
    PyErr_SetString(ToxOpError, "no message specified");
    break;
  case TOX_FAERR_OWNKEY:
    PyErr_SetString(ToxOpError, "user's own key");
    break;
  case TOX_FAERR_ALREADYSENT:
    PyErr_SetString(ToxOpError, "friend request already sent or already "
        "a friend");
    break;
  case TOX_FAERR_UNKNOWN:
    PyErr_SetString(ToxOpError, "unknown error");
    break;
  case TOX_FAERR_BADCHECKSUM:
    PyErr_SetString(ToxOpError, "bad checksum in address");
    break;
  case TOX_FAERR_SETNEWNOSPAM:
    PyErr_SetString(ToxOpError, "the friend was already there but the "
        "nospam was different");
    break;
  case TOX_FAERR_NOMEM:
    PyErr_SetString(ToxOpError, "increasing the friend list size fails");
    break;
  default:
    success = 1;
  }

  if (success) {
    return PyLong_FromLong(ret);
  } else {
    return NULL;
  }
}

static PyObject*
ToxCore_add_friend_norequest(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* address = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &address, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_FRIEND_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_FRIEND_ADDRESS_SIZE, pk);

  int res = tox_add_friend_norequest(self->tox, pk);
  if (res == -1) {
    PyErr_SetString(ToxOpError, "failed to add friend");
    return NULL;
  }

  return PyLong_FromLong(res);
}

static PyObject*
ToxCore_get_friend_id(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* id = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &id, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_CLIENT_ID_SIZE];
  hex_string_to_bytes(id, TOX_CLIENT_ID_SIZE, pk);

  int ret = tox_get_friend_number(self->tox, pk);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "no such friend");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_get_client_id(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t pk[TOX_CLIENT_ID_SIZE + 1];
  pk[TOX_CLIENT_ID_SIZE] = 0;

  uint8_t hex[TOX_CLIENT_ID_SIZE * 2 + 1];
  memset(hex, 0, TOX_CLIENT_ID_SIZE * 2 + 1);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  tox_get_client_id(self->tox, friend_num, pk);
  bytes_to_hex_string(pk, TOX_CLIENT_ID_SIZE, hex);

  return PYSTRING_FromString((const char*)hex);
}

static PyObject*
ToxCore_delfriend(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_del_friend(self->tox, friend_num) == -1) {
    PyErr_SetString(ToxOpError, "failed to delete friend");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_get_friend_connection_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_get_friend_connection_status(self->tox, friend_num);
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
ToxCore_send_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int length = 0;
  uint8_t* message = NULL;

  if (!PyArg_ParseTuple(args, "is#", &friend_num, &message, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_message(self->tox, friend_num, message, length);
  if (ret == 0) {
    PyErr_SetString(ToxOpError, "failed to send message");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_send_message_withid(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int length = 0;
  uint32_t id = 0;
  uint8_t* message = NULL;

  if (!PyArg_ParseTuple(args, "iIs#", &friend_num, &id, &message, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_message_withid(self->tox, friend_num, id,message,
      length);
  if (ret == 0) {
    PyErr_SetString(ToxOpError, "failed to send message with id");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_send_action(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int length = 0;
  uint8_t* action = NULL;

  if (!PyArg_ParseTuple(args, "is#", &friend_num, &action, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_action(self->tox, friend_num, action, length);
  if (ret == 0) {
    PyErr_SetString(ToxOpError, "failed to send action");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_send_action_withid(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int length = 0;
  uint32_t id = 0;
  uint8_t* action = NULL;

  if (!PyArg_ParseTuple(args, "iIs#", &friend_num, &id, &action, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_action_withid(self->tox, friend_num, id, action,
      length);
  if (ret == 0) {
    PyErr_SetString(ToxOpError, "failed to send action with id");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_set_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* name = 0;
  int length = 0;

  if (!PyArg_ParseTuple(args, "s#", &name, &length)) {
    return NULL;
  }

  if (tox_set_name(self->tox, name, length) == -1) {
    PyErr_SetString(ToxOpError, "failed to set_name");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_get_self_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  if (tox_get_self_name(self->tox, buf) == 0) {
    PyErr_SetString(ToxOpError, "failed to get self name");
    return NULL;
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_get_self_name_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ret = tox_get_self_name_size(self->tox);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get self name size");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_get_name(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_get_name(self->tox, friend_num, buf) == -1) {
    PyErr_SetString(ToxOpError, "failed to get name");
    return NULL;
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_get_name_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_get_name_size(self->tox, friend_num);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get name size");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_set_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* message;
  int length;
  if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
    return NULL;
  }

  if (tox_set_status_message(self->tox, message, length) == -1) {
    PyErr_SetString(ToxOpError, "failed to set status_message");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_set_user_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int status = 0;

  if (!PyArg_ParseTuple(args, "i", &status)) {
    return NULL;
  }

  if (tox_set_user_status(self->tox, status) == -1) {
    PyErr_SetString(ToxOpError, "failed to set status");
    return NULL;
  }

  Py_RETURN_NONE;
}


static PyObject*
ToxCore_get_status_message_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_get_status_message_size(self->tox, friend_num);
  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_get_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_STATUSMESSAGE_LENGTH];
  int friend_num = 0;

  memset(buf, 0, TOX_MAX_STATUSMESSAGE_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int ret = tox_get_status_message(self->tox, friend_num, buf,
      TOX_MAX_STATUSMESSAGE_LENGTH);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get status_message");
    return NULL;
  }

  buf[TOX_MAX_STATUSMESSAGE_LENGTH -1] = 0;

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_get_self_status_message(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_STATUSMESSAGE_LENGTH];
  memset(buf, 0, TOX_MAX_STATUSMESSAGE_LENGTH);

  int ret = tox_get_self_status_message(self->tox, buf,
      TOX_MAX_STATUSMESSAGE_LENGTH);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get self status_message");
    return NULL;
  }

  buf[TOX_MAX_STATUSMESSAGE_LENGTH -1] = 0;

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_get_self_status_message_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ret = tox_get_self_status_message_size(self->tox);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get self status_message size");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_get_user_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  int status = tox_get_user_status(self->tox, friend_num);

  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_get_self_user_status(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int status = tox_get_self_user_status(self->tox);
  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_get_last_online(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  uint64_t status = tox_get_last_online(self->tox, friend_num);

  if (status == 0) {
    Py_RETURN_NONE;
  }

  PyObject* datetime = PyImport_ImportModule("datetime");
  PyObject* datetimeClass = PyObject_GetAttrString(datetime, "datetime");
  PyObject* ret = PyObject_CallMethod(datetimeClass, "fromtimestamp", "K",
      status);

  return ret;
}

static PyObject*
ToxCore_set_user_is_typing(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int is_typing = 0;

  if (!PyArg_ParseTuple(args, "ii", &friend_num, &is_typing)) {
    return NULL;
  }

  int status = tox_set_user_is_typing(self->tox, friend_num, is_typing);
  return PyLong_FromLong(status);
}

static PyObject*
ToxCore_get_is_typing(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;

  if (!PyArg_ParseTuple(args, "i", &friend_num)) {
    return NULL;
  }

  if (tox_get_is_typing(self->tox, friend_num)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject*
ToxCore_set_send_receipts(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int friend_num = 0;
  int yesno = 0;

  if (!PyArg_ParseTuple(args, "ii", &friend_num, &yesno)) {
    return NULL;
  }

  tox_set_sends_receipts(self->tox, friend_num, yesno);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_count_friendlist(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t count = tox_count_friendlist(self->tox);
  return PyLong_FromUnsignedLong(count);
}

static PyObject*
ToxCore_get_num_online_friends(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t count = tox_get_num_online_friends(self->tox);
  return PyLong_FromUnsignedLong(count);
}

static PyObject*
ToxCore_get_friendlist(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  PyObject* plist = NULL;
  uint32_t count = tox_count_friendlist(self->tox);
  int* list = (int*)malloc(count * sizeof(int));

  uint32_t n = tox_get_friendlist(self->tox, list, count);

  if (!(plist = PyList_New(0))) {
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
ToxCore_add_groupchat(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ret = tox_add_groupchat(self->tox);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to add groupchat");
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_del_groupchat(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int groupid = 0;

  if (!PyArg_ParseTuple(args, "i", &groupid)) {
    return NULL;
  }

  if (tox_del_groupchat(self->tox, groupid) == -1) {
    PyErr_SetString(ToxOpError, "failed to del groupchat");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_group_peername(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  int groupid = 0;
  int peernumber = 0;
  if (!PyArg_ParseTuple(args, "ii", &groupid, &peernumber)) {
    return NULL;
  }

  int ret = tox_group_peername(self->tox, groupid, peernumber, buf);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to get group peername");
  }

  return PYSTRING_FromString((const char*)buf);
}

static PyObject*
ToxCore_invite_friend(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  int groupid = 0;
  if (!PyArg_ParseTuple(args, "ii", &friendnumber, &groupid)) {
    return NULL;
  }

  if (tox_invite_friend(self->tox, friendnumber, groupid) == -1) {
    PyErr_SetString(ToxOpError, "failed to invite friend");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_join_groupchat(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* public_key = NULL;
  int length = 0;
  int32_t friendnumber = 0;
  uint8_t pk[TOX_CLIENT_ID_SIZE];

  if (!PyArg_ParseTuple(args, "is#", &friendnumber, &public_key, &length)) {
    return NULL;
  }

  hex_string_to_bytes(public_key, TOX_CLIENT_ID_SIZE, pk);

  int ret = tox_join_groupchat(self->tox, friendnumber, pk);
  if (ret == -1) {
    PyErr_SetString(ToxOpError, "failed to join group chat");
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_group_message_send(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int groupid = 0;
  uint8_t* message = NULL;
  uint32_t length = 0;

  if (!PyArg_ParseTuple(args, "is#", &groupid, &message, &length)) {
    return NULL;
  }

  if (tox_group_message_send(self->tox, groupid, message, length) == -1) {
    PyErr_SetString(ToxOpError, "failed to send group message");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_group_action_send(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int groupid = 0;
  uint8_t* action = NULL;
  uint32_t length = 0;

  if (!PyArg_ParseTuple(args, "is#", &groupid, &action, &length)) {
    return NULL;
  }

  if (tox_group_action_send(self->tox, groupid, action, length) == -1) {
    PyErr_SetString(ToxOpError, "failed to send group action");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_group_number_peers(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int groupid = 0;

  if (!PyArg_ParseTuple(args, "i", &groupid)) {
    return NULL;
  }

  int ret = tox_group_number_peers(self->tox, groupid);

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_group_get_names(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int groupid = 0;
  if (!PyArg_ParseTuple(args, "i", &groupid)) {
    return NULL;
  }

  int n = tox_group_number_peers(self->tox, groupid);
  uint16_t* lengths = (uint16_t*)malloc(sizeof(uint16_t) * n);
  uint8_t (*names)[TOX_MAX_NAME_LENGTH] = (uint8_t(*)[TOX_MAX_NAME_LENGTH])
    malloc(sizeof(uint8_t) * n * TOX_MAX_NAME_LENGTH);

  int n2 = tox_group_get_names(self->tox, groupid, names, lengths, n);
  if (n2 == -1) {
    PyErr_SetString(ToxOpError, "failed to get group member names");
    return NULL;
  }

  PyObject* list = NULL;
  if (!(list = PyList_New(0))) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n2; ++i) {
    PyList_Append(list,
        PYSTRING_FromStringAndSize((const char*)names[i], lengths[i]));
  }

  free(names);
  free(lengths);

  return list;
}

static PyObject*
ToxCore_count_chatlist(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t n = tox_count_chatlist(self->tox);
  return PyLong_FromUnsignedLong(n);
}

static PyObject*
ToxCore_get_chatlist(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  PyObject* plist = NULL;
  uint32_t count = tox_count_chatlist(self->tox);
  int* list = (int*)malloc(count * sizeof(int));

  int n = tox_get_chatlist(self->tox, list, count);

  if (!(plist = PyList_New(0))) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n; ++i) {
    PyList_Append(plist, PyLong_FromLong(list[i]));
  }
  free(list);

  return plist;
}

static PyObject*
ToxCore_new_filesender(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  uint64_t filesize = 0;
  uint8_t* filename = 0;
  int filename_length = 0;

  if (!PyArg_ParseTuple(args, "iKs#", &friendnumber, &filesize, &filename,
        &filename_length)) {
    return NULL;
  }

  int ret = tox_new_file_sender(self->tox, friendnumber, filesize, filename,
      filename_length);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "tox_new_file_sender() failed");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_file_sendcontrol(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  uint8_t send_receive = 0;
  int filenumber = 0;
  uint8_t message_id = 0;
  uint8_t* data = NULL;
  int data_length = 0;


  if (!PyArg_ParseTuple(args, "ibib|s#", &friendnumber, &send_receive,
        &filenumber, &message_id, &data, &data_length)) {
    return NULL;
  }

  int ret = tox_file_send_control(self->tox, friendnumber, send_receive,
      filenumber, message_id, data, data_length);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "tox_file_send_control() failed");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_file_senddata(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  int filenumber = 0;
  uint8_t* data = NULL;
  uint32_t data_length = 0;


  if (!PyArg_ParseTuple(args, "iis#", &friendnumber, &filenumber, &data,
        &data_length)) {
    return NULL;
  }

  int ret = tox_file_send_data(self->tox, friendnumber, filenumber, data,
      data_length);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "tox_file_send_data() failed");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_file_data_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  if (!PyArg_ParseTuple(args, "i", &friendnumber)) {
    return NULL;
  }

  int ret = tox_file_data_size(self->tox, friendnumber);

  if (ret == -1) {
    PyErr_SetString(ToxOpError, "tox_file_data_size() failed");
    return NULL;
  }

  return PyLong_FromLong(ret);
}

static PyObject*
ToxCore_file_data_remaining(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int32_t friendnumber = 0;
  int filenumber = 0;
  int send_receive = 0;


  if (!PyArg_ParseTuple(args, "iii", &friendnumber, &filenumber,
        &send_receive)) {
    return NULL;
  }

  uint64_t ret = tox_file_data_remaining(self->tox, friendnumber, filenumber,
      send_receive);

  if (!ret) {
    PyErr_SetString(ToxOpError, "tox_file_data_remaining() failed");
    return NULL;
  }

  return PyLong_FromUnsignedLongLong(ret);
}

static PyObject*
ToxCore_get_nospam(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t nospam = tox_get_nospam(self->tox);

  return PyLong_FromUnsignedLongLong(nospam);
}

static PyObject*
ToxCore_set_nospam(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t nospam = 0;

  if (!PyArg_ParseTuple(args, "I", &nospam)) {
    return NULL;
  }

  tox_set_nospam(self->tox, nospam);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_bootstrap_from_address(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  int ipv6enabled = TOX_ENABLE_IPV6_DEFAULT;
  uint16_t port = 0;
  uint8_t* public_key = NULL;
  char* address = NULL;
  int addr_length = 0;
  int pk_length = 0;
  uint8_t pk[TOX_CLIENT_ID_SIZE];

  if (!PyArg_ParseTuple(args, "s#iHs#", &address, &addr_length, &ipv6enabled,
        &port, &public_key, &pk_length)) {
    return NULL;
  }

  hex_string_to_bytes(public_key, TOX_CLIENT_ID_SIZE, pk);
  int ret = tox_bootstrap_from_address(self->tox, address, ipv6enabled,
      htons(port), pk);

  if (!ret) {
    PyErr_SetString(ToxOpError, "failed to resolve address");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_isconnected(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  if (tox_isconnected(self->tox)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
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
ToxCore_do(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  tox_do(self->tox);

  if (PyErr_Occurred()) {
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject*
ToxCore_size(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t size = tox_size(self->tox);

  return PyLong_FromUnsignedLong(size);
}

static PyObject*
ToxCore_save(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint32_t size = tox_size(self->tox);
  uint8_t* buf = (uint8_t*)malloc(size);
  tox_save(self->tox, buf);

  PyObject* res = PYBYTES_FromStringAndSize((const char*)buf, size);
  free(buf);

  return res;
}

static PyObject*
ToxCore_save_to_file(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  char* filename = NULL;

  if (!PyArg_ParseTuple(args, "s", &filename)) {
    return NULL;
  }

  int size = tox_size(self->tox);

  uint8_t* buf = (uint8_t*)malloc(size);

  if (buf == NULL) {
    return PyErr_NoMemory();
  }

  tox_save(self->tox, buf);

  FILE* fp = fopen(filename, "w");

  if (fp == NULL) {
    PyErr_SetString(ToxOpError, "fopen(): can't open file for saving");
    return NULL;
  }

  fwrite(buf, size, 1, fp);
  fclose(fp);

  free(buf);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_load(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  uint8_t* data = NULL;
  int length = 0;

  if (!PyArg_ParseTuple(args, "s#", &data, &length)) {
    PyErr_SetString(ToxOpError, "no data specified");
    return NULL;
  }

  if (tox_load(self->tox, data, length) == -1) {
    PyErr_SetString(ToxOpError, "tox_load(): load failed");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_load_from_file(ToxCore* self, PyObject* args)
{
  CHECK_TOX(self);

  char* filename = NULL;

  if (!PyArg_ParseTuple(args, "s", &filename)) {
    return NULL;
  }

  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    PyErr_SetString(ToxOpError,
        "tox_load(): failed to open file for reading");
    return NULL;
  }

  size_t length = 0;
  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  uint8_t* data = (uint8_t*)malloc(length * sizeof(char));

  if (fread(data, length, 1, fp) != 1) {
    PyErr_SetString(ToxOpError,
        "tox_load(): corrupted data file");
    free(data);
    return NULL;
  }

  if (tox_load(self->tox, data, length) == -1) {
    PyErr_SetString(ToxOpError, "tox_load(): load failed");
    free(data);
    return NULL;
  }

  free(data);
  Py_RETURN_NONE;
}

PyMethodDef Tox_methods[] = {
  {
    "on_friend_request", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_request(address, message)\n"
    "Callback for receiving friend requests, default implementation does "
    "nothing."
  },
  {
    "on_friend_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_message(friend_number, message)\n"
    "Callback for receiving friend messages, default implementation does "
    "nothing."
  },
  {
    "on_friend_action", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_friend_action(friend_number, action)\n"
    "Callback for receiving friend actions, default implementation does "
    "nothing."
  },
  {
    "on_name_change", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_name_change(friend_number, new_name)\n"
    "Callback for receiving friend name changes, default implementation does "
    "nothing."
  },
  {
    "on_status_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_status_message(friend_number, new_status)\n"
    "Callback for receiving friend status message changes, default "
    "implementation does nothing."
  },
  {
    "on_user_status", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_user_status(friend_number, kind)\n"
    "Callback for receiving friend status changes, default implementation "
    "does nothing.\n\n"
    ".. seealso ::\n"
    "    :meth:`.set_user_status`"
  },
  {
    "on_typing_change", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_typing_change(friend_number, is_typing)\n"
    "Callback for receiving friend status changes, default implementation "
    "does nothing.\n\n"
  },
  {
    "on_read_receipt", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_read_receipt(friend_number, receipt)\n"
    "Callback for receiving read receipt, default implementation does nothing."
  },
  {
    "on_connection_status", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_connection_status(friend_number, status)\n"
    "Callback for receiving read receipt, default implementation does "
    "nothing.\n\n"
    "*status* is a boolean value which indicates the status of the friend "
    "indicated by *friend_number*. True means online and False means offline "
    "after previously online."
  },
  {
    "on_group_invite", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_group_invite(friend_number, group_public_key)\n"
    "Callback for receiving group invitations, default implementation does "
    "nothing."
  },
  {
    "on_group_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_group_message(group_number, friend_group_number, message)\n"
    "Callback for receiving group messages, default implementation does "
    "nothing."
  },
  {
    "on_group_action", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_group_action(group_number, friend_group_number, action)\n"
    "Callback for receiving group actions, default implementation does "
    "nothing."
  },
  {
    "on_group_namelist_change", (PyCFunction)ToxCore_callback_stub,
    METH_VARARGS,
    "on_group_namelist_change(group_number, peer_number, change)\n"
    "Callback for receiving group messages, default implementation does "
    "nothing.\n\n"
    "There are there possible *change* values:\n\n"
    "+---------------------------+----------------------+\n"
    "| change                    | description          |\n"
    "+===========================+======================+\n"
    "| Tox.CHAT_CHANGE_PEER_ADD  | a peer is added      |\n"
    "+---------------------------+----------------------+\n"
    "| Tox.CHAT_CHANGE_PEER_DEL  | a peer is deleted    |\n"
    "+---------------------------+----------------------+\n"
    "| Tox.CHAT_CHANGE_PEER_NAME | name of peer changed |\n"
    "+---------------------------+----------------------+\n"
  },
  {
    "on_file_send_request", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_send_request(friend_number, file_number, file_size, filename)\n"
    "Callback for receiving file send requests, default implementation does "
    "nothing."
  },
  {
    "on_file_control", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_control(friend_number, receive_send, file_number, control_type, "
    "data)\n"
    "Callback for receiving file send control, default implementation does "
    "nothing. See :meth:`.file_send_control` for the meaning of *receive_send* "
    "and *control_type*.\n\n"
    ".. seealso ::\n"
    "    :meth:`.file_send_control`"
  },
  {
    "on_file_data", (PyCFunction)ToxCore_callback_stub, METH_VARARGS,
    "on_file_data(friend_number, file_number, data)\n"
    "Callback for receiving file data, default implementation does nothing."
  },
  {
    "get_address", (PyCFunction)ToxCore_getaddress, METH_NOARGS,
    "get_address()\n"
    "Return address to give to others."
  },
  {
    "add_friend", (PyCFunction)ToxCore_add_friend, METH_VARARGS,
    "add_friend(address, message)\n"
    "Add a friend."
  },
  {
    "add_friend_norequest", (PyCFunction)ToxCore_add_friend_norequest,
    METH_VARARGS,
    "add_friend_norequest(address)\n"
    "Add a friend without sending request."
  },
  {
    "get_friend_id", (PyCFunction)ToxCore_get_friend_id, METH_VARARGS,
    "get_friend_id(friend_id)\n"
    "Return the friend id associated to that client id."
  },
  {
    "get_client_id", (PyCFunction)ToxCore_get_client_id, METH_VARARGS,
    "get_client_id(friend_number)\n"
    "Return the public key associated to that friend number."
  },
  {
    "del_friend", (PyCFunction)ToxCore_delfriend, METH_VARARGS,
    "del_friend(friend_number)\n"
    "Remove a friend."
  },
  {
    "get_friend_connection_status",
    (PyCFunction)ToxCore_get_friend_connection_status, METH_VARARGS,
    "get_friend_connection_status(friend_number)\n"
    "Return True if friend is connected(Online) else False."
  },
  {
    "friend_exists", (PyCFunction)ToxCore_friend_exists, METH_VARARGS,
    "friend_exists(friend_number)\n"
    "Checks if there exists a friend with given friendnumber."
  },
  {
    "send_message", (PyCFunction)ToxCore_send_message, METH_VARARGS,
    "send_message(friend_number, message)\n"
    "Send a text chat message to an online friend."
  },
  {
    "send_message_withid", (PyCFunction)ToxCore_send_message_withid,
    METH_VARARGS,
    "send_message_withid(friend_number, id, message)\n"
    "Send a text chat message to an online friend with id."
  },
  {
    "send_action", (PyCFunction)ToxCore_send_action, METH_VARARGS,
    "send_action(friend_number, action)\n"
    "Send an action to an online friend."
  },
  {
    "send_action_withid", (PyCFunction)ToxCore_send_action_withid, METH_VARARGS,
    "send_action_withid(friend_number, id, action)\n"
    "Send an action to an online friend with id."
  },
  {
    "set_name", (PyCFunction)ToxCore_set_name, METH_VARARGS,
    "set_name(name)\n"
    "Set our nickname."
  },
  {
    "get_self_name", (PyCFunction)ToxCore_get_self_name, METH_NOARGS,
    "get_self_name()\n"
    "Get your nickname."
  },
  {
    "get_self_name_size", (PyCFunction)ToxCore_get_self_name_size, METH_NOARGS,
    "get_self_name_size()\n"
    "Get your nickname string length"
  },
  {
    "get_name", (PyCFunction)ToxCore_get_name, METH_VARARGS,
    "get_name(friend_number)\n"
    "Get nickname of *friend_number*."
  },
  {
    "get_name_size", (PyCFunction)ToxCore_get_name_size, METH_VARARGS,
    "get_name_size(friend_number)\n"
    "Get nickname length of *friend_number*."
  },
  {
    "set_status_message", (PyCFunction)ToxCore_set_status_message, METH_VARARGS,
    "set_status_message(message)\n"
    "Set our user status message."
  },
  {
    "set_user_status", (PyCFunction)ToxCore_set_user_status, METH_VARARGS,
    "set_user_status(status)\n"
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
    "get_status_message_size", (PyCFunction)ToxCore_get_status_message_size,
    METH_VARARGS,
    "get_status_message_size(friend_number)\n"
    "Return the length of *friend_number*'s status message."
  },
  {
    "get_status_message", (PyCFunction)ToxCore_get_status_message,
    METH_VARARGS,
    "get_status_message(friend_number)\n"
    "Get status message of a friend."
  },
  {
    "get_self_status_message", (PyCFunction)ToxCore_get_self_status_message,
    METH_NOARGS,
    "get_self_status_message()\n"
    "Get status message of yourself."
  },
  {
    "get_self_status_message_size",
    (PyCFunction)ToxCore_get_self_status_message_size,
    METH_NOARGS,
    "get_self_status_message_size()\n"
    "Get status message string length of yourself."
  },
  {
    "get_user_status", (PyCFunction)ToxCore_get_user_status,
    METH_VARARGS,
    "get_user_status(friend_number)\n"
    "Get friend status.\n\n"
    ".. seealso ::\n"
    "    :meth:`.set_user_status`"
  },
  {
    "get_self_user_status", (PyCFunction)ToxCore_get_self_user_status,
    METH_NOARGS,
    "get_self_user_status()\n"
    "Get user status of youself.\n\n"
    ".. seealso ::\n"
    "    :meth:`.set_user_status`"
  },
  {
    "get_last_online", (PyCFunction)ToxCore_get_last_online,
    METH_VARARGS,
    "get_last_online(friend_number)\n"
    "returns datetime.datetime object representing the last time "
    "*friend_number* was seen online, or None if never seen."
  },
  {
    "set_user_is_typing", (PyCFunction)ToxCore_set_user_is_typing,
    METH_VARARGS,
    "set_user_is_typing(friend_number, is_typing)\n"
    "Set user typing status.\n\n"
  },
  {
    "get_is_typing", (PyCFunction)ToxCore_get_is_typing,
    METH_VARARGS,
    "get_is_typing(friend_number)\n"
    "Return True is user is typing.\n\n"
  },
  {
    "set_send_receipts", (PyCFunction)ToxCore_set_send_receipts,
    METH_VARARGS,
    "set_send_receipts(friend_number, yesno)\n"
    "Sets whether we send read receipts for friendnumber. *yesno* should be "
    "a boolean value."
  },
  {
    "count_friendlist", (PyCFunction)ToxCore_count_friendlist,
    METH_NOARGS,
    "count_friendlist()\n"
    "Return the number of friends."
  },
  {
    "get_num_online_friends", (PyCFunction)ToxCore_get_num_online_friends,
    METH_NOARGS,
    "get_num_online_friends()\n"
    "Return the number of online friends."
  },
  {
    "get_friendlist", (PyCFunction)ToxCore_get_friendlist,
    METH_NOARGS,
    "get_friendlist()\n"
    "Get a list of valid friend numbers."
  },
  {
    "add_groupchat", (PyCFunction)ToxCore_add_groupchat, METH_VARARGS,
    "add_groupchat()\n"
    "Creates a new groupchat and puts it in the chats array."
  },
  {
    "del_groupchat", (PyCFunction)ToxCore_del_groupchat, METH_VARARGS,
    "del_groupchat(gruop_number)\n"
    "Delete a groupchat from the chats array."
  },
  {
    "group_peername", (PyCFunction)ToxCore_group_peername, METH_VARARGS,
    "group_peername(group_number, peer_number)\n"
    "Get the group peer's name."
  },
  {
    "invite_friend", (PyCFunction)ToxCore_invite_friend, METH_VARARGS,
    "invite_friend(friend_number, group_number)\n"
    "Invite friendnumber to groupnumber."
  },
  {
    "join_groupchat", (PyCFunction)ToxCore_join_groupchat, METH_VARARGS,
    "join_groupchat(friend_number, publick_key)\n"
    "Join a group (you need to have been invited first.)"
  },
  {
    "group_message_send", (PyCFunction)ToxCore_group_message_send, METH_VARARGS,
    "group_message_send(group_number, message)\n"
    "send a group message."
  },
  {
    "group_action_send", (PyCFunction)ToxCore_group_action_send, METH_VARARGS,
    "group_action_send(group_number, action)\n"
    "send a group action."
  },
  {
    "group_number_peers", (PyCFunction)ToxCore_group_number_peers, METH_VARARGS,
    "group_number_peers(group_number)\n"
    "Return the number of peers in the group chat."
  },
  {
    "group_get_names", (PyCFunction)ToxCore_group_get_names, METH_VARARGS,
    "group_get_names(group_number)\n"
    "List all the peers in the group chat."
  },
  {
    "count_chatlist", (PyCFunction)ToxCore_count_chatlist, METH_VARARGS,
    "count_chatlist()\n"
    "Return the number of chats in the current Tox instance."
  },
  {
    "get_chatlist", (PyCFunction)ToxCore_get_chatlist, METH_VARARGS,
    "get_chatlist()\n"
    "Return a list of valid group numbers."
  },
  {
    "new_file_sender", (PyCFunction)ToxCore_new_filesender, METH_VARARGS,
    "new_file_sender(friend_number, file_size, filename)\n"
    "Send a file send request. Returns file number to be sent."
  },
  {
    "file_send_control", (PyCFunction)ToxCore_file_sendcontrol, METH_VARARGS,
    "file_send_control(friend_number, send_receive, file_number, control_type"
    "[, data=None])\n"
    "Send file transfer control.\n\n"
    "*send_receive* is 0 for sending and 1 for receiving.\n\n"
    "*control_type* can be one of following value:\n\n"
    "+-------------------------------+------------------------+\n"
    "| control_type                  | description            |\n"
    "+===============================+========================+\n"
    "| Tox.FILECONTROL_ACCEPT        | accepts transfer       |\n"
    "+-------------------------------+------------------------+\n"
    "| Tox.FILECONTROL_PAUSE         | pause transfer         |\n"
    "+-------------------------------+------------------------+\n"
    "| Tox.FILECONTROL_KILL          | kill/rejct transfer    |\n"
    "+-------------------------------+------------------------+\n"
    "| Tox.FILECONTROL_FINISHED      | transfer finished      |\n"
    "+-------------------------------+------------------------+\n"
    "| Tox.FILECONTROL_RESUME_BROKEN | resume broken transfer |\n"
    "+-------------------------------+------------------------+\n"
  },
  {
    "file_send_data", (PyCFunction)ToxCore_file_senddata, METH_VARARGS,
    "file_send_data(friend_number, file_number, data)\n"
    "Send file data."
  },
  {
    "file_data_size", (PyCFunction)ToxCore_file_data_size, METH_VARARGS,
    "file_data_size(friend_number)\n"
    "Returns the recommended/maximum size of the filedata you send with "
    ":meth:`.file_send_data`."},
  {
    "file_data_remaining", (PyCFunction)ToxCore_file_data_remaining,
    METH_VARARGS,
    "file_data_remaining(friend_number, file_number, send_receive)\n"
    "Give the number of bytes left to be sent/received. *send_receive* is "
    "0 for sending and 1 for receiving."
  },
  {
    "get_nospam", (PyCFunction)ToxCore_get_nospam,
    METH_NOARGS,
    "get_nospam()\n"
    "get nospam part from ID"
  },
  {
    "set_nospam", (PyCFunction)ToxCore_set_nospam,
    METH_VARARGS,
    "set_nospam(nospam)\n"
    "set nospam part of ID. *nospam* should be of type uint32"
  },
  {
    "bootstrap_from_address", (PyCFunction)ToxCore_bootstrap_from_address,
    METH_VARARGS,
    "bootstrap_from_address(address, ipv6enabled, port, public_key)\n"
    "Resolves address into an IP address. If successful, sends a 'get nodes'"
    "request to the given node with ip, port."
  },
  {
    "isconnected", (PyCFunction)ToxCore_isconnected, METH_NOARGS,
    "isconnected()\n"
    "Return False if we are not connected to the DHT."
  },
  {
    "kill", (PyCFunction)ToxCore_kill, METH_NOARGS,
    "kill()\n"
    "Run this before closing shop."
  },
  {
    "do", (PyCFunction)ToxCore_do, METH_NOARGS,
    "do()\n"
    "The main loop that needs to be run at least 20 times per second."
  },
  {
    "size", (PyCFunction)ToxCore_size, METH_NOARGS,
    "size()\n"
    "return size of messenger data (for saving)."
  },
  {
    "save", (PyCFunction)ToxCore_save, METH_VARARGS,
    "save()\n"
    "Return messenger blob in str."
  },
  {
    "save_to_file", (PyCFunction)ToxCore_save_to_file, METH_VARARGS,
    "save_to_file(filename)\n"
    "Save the messenger to a file."
  },
  {
    "load", (PyCFunction)ToxCore_load, METH_VARARGS,
    "load(blob)\n"
    "Load the messenger from *blob*."
  },
  {
    "load_from_file", (PyCFunction)ToxCore_load_from_file, METH_VARARGS,
    "load_from_file(filename)\n"
    "Load the messenger from file."
  },
  {NULL}
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
  PyObject* obj_##name = PyLong_FromLong(TOX_##name);        \
  PyDict_SetItemString(dict, #name, obj_##name);             \
  Py_DECREF(obj_##name);

  PyObject* dict = PyDict_New();
  SET(FAERR_TOOLONG)
  SET(FAERR_NOMESSAGE)
  SET(FAERR_OWNKEY)
  SET(FAERR_ALREADYSENT)
  SET(FAERR_UNKNOWN)
  SET(FAERR_BADCHECKSUM)
  SET(FAERR_SETNEWNOSPAM)
  SET(FAERR_NOMEM)
  SET(USERSTATUS_NONE)
  SET(USERSTATUS_AWAY)
  SET(USERSTATUS_BUSY)
  SET(USERSTATUS_INVALID)
  SET(CHAT_CHANGE_PEER_ADD)
  SET(CHAT_CHANGE_PEER_DEL)
  SET(CHAT_CHANGE_PEER_NAME)
  SET(FILECONTROL_ACCEPT)
  SET(FILECONTROL_PAUSE)
  SET(FILECONTROL_KILL)
  SET(FILECONTROL_FINISHED)
  SET(FILECONTROL_RESUME_BROKEN)

#undef SET

  ToxCoreType.tp_dict = dict;
}
