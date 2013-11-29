/**
 * @file   core.c
 * @author Wei-Ning Huang (AZ) <aitjcize@gmail.com>
 *
 * Copyright (C) 2013 -  Wei-Ning Huang (AZ) <aitjcize@gmail.com>
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
#include <tox/tox.h>

PyObject* ToxCoreError;

static void bytes_to_hex_string(uint8_t* digest, int length,
    uint8_t* hex_digest)
{
  hex_digest[2 * length] = 0;

  int i, j;
  for(i = j = 0; i < length; ++i) {
    char c;
    c = (digest[i] >> 4) & 0xf;
    c = (c > 9) ? c + 'A'- 10 : c + '0';
    hex_digest[j++] = c;
    c = (digest[i] & 0xf);
    c = (c > 9) ? c + 'A' - 10 : c + '0';
    hex_digest[j++] = c;
  }
}

static int hex_char_to_int(char c)
{
  int val = 0;
  if (c >= '0' && c <= '9') {
    val = c - '0';
  } else if(c >= 'A' && c <= 'F') {
    val = c - 'A' + 10;
  } else if(c >= 'a' && c <= 'f') {
    val = c - 'a' + 10;
  } else {
    val = 0;
  }
  return val;
}

static void hex_string_to_bytes(uint8_t* hexstr, int length, uint8_t* bytes)
{
  int i;
  for (i = 0; i < length; ++i) {
    bytes[i] = (hex_char_to_int(hexstr[2 * i]) << 4)
             | (hex_char_to_int(hexstr[2 * i + 1]));
  }
}

/* core.Tox definition */
typedef struct {
  PyObject_HEAD
  Tox* tox;
} ToxCore;

static void callback_friendrequest(uint8_t* public_key, uint8_t* data,
    uint16_t length, void* self)
{
  uint8_t buf[TOX_FRIEND_ADDRESS_SIZE * 2 + 1];
  bytes_to_hex_string(public_key, TOX_FRIEND_ADDRESS_SIZE, buf);

  PyObject_CallMethod((PyObject*)self, "on_friendrequest", "ss#", buf, data,
      length);
}

static void callback_friendmessage(Tox *tox, int friendnumber,
    uint8_t* message, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_friendmessage", "is#", friendnumber,
      message, length);
}

static void callback_action(Tox *tox, int friendnumber, uint8_t* action,
    uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_action", "is#", friendnumber, action,
      length);
}

static void callback_namechange(Tox *tox, int friendnumber, uint8_t* newname,
    uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_namechange", "is#", friendnumber,
      newname, length);
}

static void callback_statusmessage(Tox *tox, int friendnumber,
    uint8_t *newstatus, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_statusmessage", "is#", friendnumber,
      newstatus, length);
}

static void callback_userstatus(Tox *tox, int friendnumber,
    TOX_USERSTATUS kind, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_userstatus", "ii", friendnumber,
      kind);
}

static void callback_read_receipt(Tox *tox, int friendnumber, uint32_t receipt,
    void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_read_receipt", "ii", friendnumber,
      receipt);
}

static void callback_connectionstatus(Tox *tox, int friendnumber,
    uint8_t status, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_connectionstatus", "ii",
      friendnumber, status);
}

static void callback_group_invite(Tox *tox, int friendnumber,
    uint8_t* group_public_key, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_invite", "is#", friendnumber,
      group_public_key, TOX_FRIEND_ADDRESS_SIZE);
}

static void callback_group_message(Tox *tox, int groupnumber,
    int friendgroupnumber, uint8_t* message, uint16_t length, void *self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_message", "iis#", groupnumber,
      friendgroupnumber, message, length);
}

static void callback_group_namelistchange(Tox *tox, int groupnumber,
    int peernumber, uint8_t change, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_group_namelistchange", "iii",
      groupnumber, peernumber, change);
}

static void callback_file_sendrequest(Tox *m, int friendnumber,
    uint8_t filenumber, uint64_t filesize, uint8_t* filename,
    uint16_t filename_length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_file_sendrequest", "iiKs#",
      friendnumber, filenumber, filesize, filename, filename_length);
}

static void callback_file_control(Tox *m, int friendnumber,
    uint8_t receive_send, uint8_t filenumber, uint8_t control_type,
    uint8_t* data, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_file_control", "iiiis#",
      friendnumber, receive_send, filenumber, control_type, data, length);
}

static void callback_file_data(Tox *m, int friendnumber, uint8_t filenumber,
    uint8_t* data, uint16_t length, void* self)
{
  PyObject_CallMethod((PyObject*)self, "on_file_data", "iis#",
      friendnumber, filenumber, data, length);
}

static int init_helper(ToxCore* self, PyObject* args)
{
  if (self->tox != NULL) {
    tox_kill(self->tox);
    self->tox = NULL;
  }

  int ipv6enabled = TOX_ENABLE_IPV6_DEFAULT;

  if (!PyArg_ParseTuple(args, "|i", &ipv6enabled)) {
    PyErr_SetString(PyExc_TypeError, "ipv6enabled should be boolean");
    return -1;
  }

  Tox* tox = tox_new(ipv6enabled);

  tox_callback_friend_request(tox, callback_friendrequest, self);
  tox_callback_friend_message(tox, callback_friendmessage, self);
  tox_callback_action(tox, callback_action, self);
  tox_callback_name_change(tox, callback_namechange, self);
  tox_callback_status_message(tox, callback_statusmessage, self);
  tox_callback_user_status(tox, callback_userstatus, self);
  tox_callback_read_receipt(tox, callback_read_receipt, self);
  tox_callback_connection_status(tox, callback_connectionstatus, self);
  tox_callback_group_invite(tox, callback_group_invite, self);
  tox_callback_group_message(tox, callback_group_message, self);
  tox_callback_group_namelist_change(tox, callback_group_namelistchange, self);
  tox_callback_file_send_request(tox, callback_file_sendrequest, self);
  tox_callback_file_control(tox, callback_file_control, self);
  tox_callback_file_data(tox, callback_file_data, self);

  self->tox = tox;
}

static PyObject*
ToxCore_new(PyTypeObject *type, PyObject* args, PyObject* kwds)
{
  ToxCore* self = (ToxCore*)type->tp_alloc(type, 0);
  self->tox = NULL;

  if (init_helper(self, args) == -1) {
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
  uint8_t address[TOX_FRIEND_ADDRESS_SIZE];
  uint8_t address_hex[TOX_FRIEND_ADDRESS_SIZE * 2 + 1];

  tox_get_address(self->tox, address);
  bytes_to_hex_string(address, TOX_FRIEND_ADDRESS_SIZE, address_hex);

  PyObject* res = PyString_FromString((const char*)address_hex);
  return res;
}

static PyObject*
ToxCore_addfriend(ToxCore* self, PyObject* args)
{
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

  int ret = tox_add_friend(self->tox, pk, data, data_length + 1);
  int success = 0;

  switch (ret) {
  case TOX_FAERR_TOOLONG:
    PyErr_SetString(ToxCoreError, "message too long");
    break;
  case TOX_FAERR_NOMESSAGE:
    PyErr_SetString(ToxCoreError, "no message specified");
    break;
  case TOX_FAERR_OWNKEY:
    PyErr_SetString(ToxCoreError, "user's own key");
    break;
  case TOX_FAERR_ALREADYSENT:
    PyErr_SetString(ToxCoreError, "friend request already sent or already "
        "a friend");
    break;
  case TOX_FAERR_UNKNOWN:
    PyErr_SetString(ToxCoreError, "unknown error");
    break;
  case TOX_FAERR_BADCHECKSUM:
    PyErr_SetString(ToxCoreError, "bad checksum in address");
    break;
  case TOX_FAERR_SETNEWNOSPAM:
    PyErr_SetString(ToxCoreError, "the friend was already there but the "
        "nospam was different");
    break;
  case TOX_FAERR_NOMEM:
    PyErr_SetString(ToxCoreError, "increasing the friend list size fails");
    break;
  default:
    success = 1;
  }

  if (success) {
    return PyInt_FromLong(ret);
  } else {
    return NULL;
  }
}

static PyObject*
ToxCore_addfriend_norequest(ToxCore* self, PyObject* args)
{
  uint8_t* address = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &address, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_FRIEND_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_FRIEND_ADDRESS_SIZE, pk);

  if (tox_add_friend_norequest(self->tox, pk) == -1) {
    PyErr_SetString(ToxCoreError, "failed to add friend");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_getfriend_id(ToxCore* self, PyObject* args)
{
  uint8_t* address = NULL;
  int addr_length = 0;

  if (!PyArg_ParseTuple(args, "s#", &address, &addr_length)) {
    return NULL;
  }

  uint8_t pk[TOX_FRIEND_ADDRESS_SIZE];
  hex_string_to_bytes(address, TOX_FRIEND_ADDRESS_SIZE, pk);

  int ret = tox_get_friend_id(self->tox, pk);
  if (ret == -1) {
    PyErr_SetString(ToxCoreError, "no such friend");
    return NULL;
  }

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_getclient_id(ToxCore* self, PyObject* args)
{
  uint8_t pk[TOX_FRIEND_ADDRESS_SIZE + 1];
  pk[TOX_FRIEND_ADDRESS_SIZE] = 0;

  uint8_t hex[TOX_FRIEND_ADDRESS_SIZE * 2 + 1];

  int friendid = 0;

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  tox_get_client_id(self->tox, friendid, pk);
  bytes_to_hex_string(pk, TOX_FRIEND_ADDRESS_SIZE, hex);

  return PyString_FromString((const char*)hex);
}

static PyObject*
ToxCore_delfriend(ToxCore* self, PyObject* args)
{
  int friendid = 0;

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  if (tox_del_friend(self->tox, friendid) == -1) {
    PyErr_SetString(ToxCoreError, "failed to delete friend");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_get_friend_connectionstatus(ToxCore* self, PyObject* args)
{
  int friendid = 0;

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  int ret = tox_get_friend_connection_status(self->tox, friendid);
  if (ret == -1) {
    PyErr_SetString(ToxCoreError, "failed get connection status");
    return NULL;
  }

  return PyBool_FromLong(ret);
}

static PyObject*
ToxCore_friend_exists(ToxCore* self, PyObject* args)
{
  int friendid = 0;

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  int ret = tox_friend_exists(self->tox, friendid);

  return PyBool_FromLong(ret);
}

static PyObject*
ToxCore_sendmessage(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  int length = 0;
  uint8_t* message = NULL;

  if (!PyArg_ParseTuple(args, "is#", &friendid, &message, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_message(self->tox, friendid, message, length + 1);
  if (ret == 0) {
    PyErr_SetString(ToxCoreError, "failed to send message");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_sendmessage_withid(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  int length = 0;
  uint32_t id = 0;
  uint8_t* message = NULL;

  if (!PyArg_ParseTuple(args, "iIs#", &friendid, &id, &message, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_message_withid(self->tox, friendid, id,message,
      length + 1);
  if (ret == 0) {
    PyErr_SetString(ToxCoreError, "failed to send message with id");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_sendaction(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  int length = 0;
  uint8_t* action = NULL;

  if (!PyArg_ParseTuple(args, "is#", &friendid, &action, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_action(self->tox, friendid, action, length + 1);
  if (ret == 0) {
    PyErr_SetString(ToxCoreError, "failed to send action");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_sendaction_withid(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  int length = 0;
  uint32_t id = 0;
  uint8_t* action = NULL;

  if (!PyArg_ParseTuple(args, "iIs#", &friendid, &id, &action, &length)) {
    return NULL;
  }

  uint32_t ret = tox_send_action_withid(self->tox, friendid, id, action,
      length + 1);
  if (ret == 0) {
    PyErr_SetString(ToxCoreError, "failed to send action with id");
    return NULL;
  }

  return PyLong_FromUnsignedLong(ret);
}

static PyObject*
ToxCore_setname(ToxCore* self, PyObject* args)
{
  uint8_t* name = 0;
  int length = 0;

  if (!PyArg_ParseTuple(args, "s#", &name, &length)) {
    return NULL;
  }

  if (tox_set_name(self->tox, name, length + 1) == -1) {
    PyErr_SetString(ToxCoreError, "failed to setname");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_getselfname(ToxCore* self, PyObject* args)
{
  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  if (tox_get_self_name(self->tox, buf, TOX_MAX_NAME_LENGTH) == 0) {
    PyErr_SetString(ToxCoreError, "failed to get self name");
    return NULL;
  }

  return PyString_FromString((const char*)buf);
}

static PyObject*
ToxCore_getname(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  if (tox_get_name(self->tox, friendid, buf) == -1) {
    PyErr_SetString(ToxCoreError, "failed to get name");
    return NULL;
  }

  return PyString_FromString((const char*)buf);
}

static PyObject*
ToxCore_set_statusmessage(ToxCore* self, PyObject* args)
{
  uint8_t* message;
  int length;
  if (!PyArg_ParseTuple(args, "s#", &message, &length)) {
    return NULL;
  }

  if (tox_set_status_message(self->tox, message, length + 1) == -1) {
    PyErr_SetString(ToxCoreError, "failed to set statusmessage");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_set_userstatus(ToxCore* self, PyObject* args)
{
  int status = 0;

  if (!PyArg_ParseTuple(args, "i", &status)) {
    return NULL;
  }

  if (tox_set_userstatus(self->tox, status) == -1) {
    PyErr_SetString(ToxCoreError, "failed to set status");
    return NULL;
  }

  Py_RETURN_NONE;
}


static PyObject*
ToxCore_get_statusmessage_size(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  int ret = tox_get_status_message_size(self->tox, friendid);
  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_copy_statusmessage(ToxCore* self, PyObject* args)
{
  uint8_t buf[TOX_MAX_STATUSMESSAGE_LENGTH];
  int friendid = 0;

  memset(buf, 0, TOX_MAX_STATUSMESSAGE_LENGTH);

  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  int ret = tox_get_status_message(self->tox, friendid, buf,
      TOX_MAX_STATUSMESSAGE_LENGTH);

  if (ret == -1) {
    PyErr_SetString(ToxCoreError, "failed to copy statusmessage");
    return NULL;
  }

  buf[TOX_MAX_STATUSMESSAGE_LENGTH -1] = 0;

  return PyString_FromString((const char*)buf);
}

static PyObject*
ToxCore_copy_self_statusmessage(ToxCore* self, PyObject* args)
{
  uint8_t buf[TOX_MAX_STATUSMESSAGE_LENGTH];
  memset(buf, 0, TOX_MAX_STATUSMESSAGE_LENGTH);

  int ret = tox_get_self_status_message(self->tox, buf,
      TOX_MAX_STATUSMESSAGE_LENGTH);

  if (ret == -1) {
    PyErr_SetString(ToxCoreError, "failed to copy self statusmessage");
    return NULL;
  }

  buf[TOX_MAX_STATUSMESSAGE_LENGTH -1] = 0;

  return PyString_FromString((const char*)buf);
}

static PyObject*
ToxCore_get_userstatus(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  if (!PyArg_ParseTuple(args, "i", &friendid)) {
    return NULL;
  }

  int status = tox_get_user_status(self->tox, friendid);

  return PyInt_FromLong(status);
}

static PyObject*
ToxCore_get_selfuserstatus(ToxCore* self, PyObject* args)
{
  int status = tox_get_self_user_status(self->tox);
  return PyInt_FromLong(status);
}

static PyObject*
ToxCore_set_send_receipts(ToxCore* self, PyObject* args)
{
  int friendid = 0;
  int yesno = 0;

  if (!PyArg_ParseTuple(args, "ii", &friendid, &yesno)) {
    return NULL;
  }

  tox_set_sends_receipts(self->tox, friendid, yesno);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_count_friendlist(ToxCore* self, PyObject* args)
{
  uint32_t count = tox_count_friendlist(self->tox);
  return PyLong_FromUnsignedLong(count);
}

static PyObject*
ToxCore_copy_friendlist(ToxCore* self, PyObject* args)
{
  PyObject* plist = NULL;
  uint32_t count = tox_count_friendlist(self->tox);
  int* list = (int*)malloc(count * sizeof(int));

  uint32_t n = tox_get_friendlist(self->tox, list, count);

  if (!(plist = PyList_New(0))) {
    return NULL;
  }

  uint32_t i = 0;
  for (i = 0; i < n; ++i) {
    PyList_Append(plist, PyInt_FromLong(list[i]));
  }
  free(list);

  return plist;
}

static PyObject*
ToxCore_add_groupchat(ToxCore* self, PyObject* args)
{
  int ret = tox_add_groupchat(self->tox);
  if (ret == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to add groupchat");
  }

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_del_groupchat(ToxCore* self, PyObject* args)
{
  int groupnumber = 0;

  if (!PyArg_ParseTuple(args, "i", &groupnumber)) {
    return NULL;
  }

  if (tox_del_groupchat(self->tox, groupnumber) == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to del groupchat");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_group_peername(ToxCore* self, PyObject* args)
{
  uint8_t buf[TOX_MAX_NAME_LENGTH];
  memset(buf, 0, TOX_MAX_NAME_LENGTH);

  int groupnumber = 0;
  int peernumber = 0;
  if (!PyArg_ParseTuple(args, "ii", &groupnumber, &peernumber)) {
    return NULL;
  }

  int ret = tox_group_peername(self->tox, groupnumber, peernumber, buf);
  if (ret == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to get group peername");
  }

  return PyString_FromString((const char*)buf);
}

static PyObject*
ToxCore_invite_friend(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  int groupnumber = 0;
  if (!PyArg_ParseTuple(args, "ii", &friendnumber, &groupnumber)) {
    return NULL;
  }

  if (tox_invite_friend(self->tox, friendnumber, groupnumber) == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to invite friend");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_join_groupchat(ToxCore* self, PyObject* args)
{
  uint8_t* pk = NULL;
  int length = 0;
  int friendnumber = 0;

  uint8_t pkbytes[TOX_FRIEND_ADDRESS_SIZE + 1];

  if (!PyArg_ParseTuple(args, "is#", &friendnumber, &pk, &length)) {
    return NULL;
  }

  hex_string_to_bytes(pk, TOX_FRIEND_ADDRESS_SIZE, pkbytes);

  int ret = tox_join_groupchat(self->tox, friendnumber, pkbytes);
  if (ret == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to join group chat");
  }

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_group_message_send(ToxCore* self, PyObject* args)
{
  int groupnumber = 0;
  uint8_t* message = NULL;
  uint32_t length = 0;

  if (!PyArg_ParseTuple(args, "is#", &groupnumber, &message, &length)) {
    return NULL;
  }

  if (tox_group_message_send(self->tox, groupnumber, message, length) == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to send group message");
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_group_number_peers(ToxCore* self, PyObject* args)
{
  int groupnumber = 0;

  if (!PyArg_ParseTuple(args, "i", &groupnumber)) {
    return NULL;
  }

  int ret = tox_group_number_peers(self->tox, groupnumber);

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_group_copy_names(ToxCore* self, PyObject* args)
{
  int groupnumber = 0;
  if (!PyArg_ParseTuple(args, "i", &groupnumber)) {
    return NULL;
  }

  int n = tox_group_number_peers(self->tox, groupnumber);
  uint8_t (*names)[TOX_MAX_NAME_LENGTH] = (uint8_t(*)[TOX_MAX_NAME_LENGTH])
    malloc(sizeof(uint8_t*) * n);

  int n2 = tox_group_get_names(self->tox, groupnumber, names, n);
  if (n2 == -1) {
    PyErr_SetString(PyExc_TypeError, "failed to copy group member names");
    return NULL;
  }

  PyObject* list = NULL;
  if (!(list = PyList_New(0))) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n2; ++i) {
    PyList_Append(list, PyString_FromString((const char*)names[i]));
  }

  return list;
}

static PyObject*
ToxCore_count_chatlist(ToxCore* self, PyObject* args)
{
  uint32_t n = tox_count_chatlist(self->tox);
  return PyLong_FromUnsignedLong(n);
}

static PyObject*
ToxCore_copy_chatlist(ToxCore* self, PyObject* args)
{
  PyObject* plist = NULL;
  uint32_t count = tox_count_chatlist(self->tox);
  int* list = (int*)malloc(count * sizeof(int));

  int n = tox_get_chatlist(self->tox, list, count);

  if (!(plist = PyList_New(0))) {
    return NULL;
  }

  int i = 0;
  for (i = 0; i < n; ++i) {
    PyList_Append(plist, PyInt_FromLong(list[i]));
  }
  free(list);

  return plist;
}

static PyObject*
ToxCore_new_filesender(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  int filesize = 0;
  uint8_t* filename = 0;
  int filename_length = 0;

  if (!PyArg_ParseTuple(args, "iKs#", &friendnumber, &filesize, &filename,
        &filename_length)) {
    return NULL;
  }

  int ret = tox_new_file_sender(self->tox, friendnumber, filesize, filename,
      filename_length);

  if (ret == -1) {
    PyErr_SetString(PyExc_TypeError, "tox_new_file_sender() failed");
    return NULL;
  }

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_file_sendcontrol(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  uint8_t send_receive = 0;
  int filenumber = 0;
  uint8_t message_id = 0;
  uint8_t* data = NULL;
  int data_length = 0;


  if (!PyArg_ParseTuple(args, "ibibs#", &friendnumber, &send_receive,
        &filenumber, &message_id, &data, &data_length)) {
    return NULL;
  }

  int ret = tox_file_send_control(self->tox, friendnumber, send_receive,
      filenumber, message_id, data, data_length);

  if (!ret) {
    PyErr_SetString(PyExc_TypeError, "tox_file_send_control() failed");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_file_senddata(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  int filenumber = 0;
  uint8_t* data = NULL;
  uint32_t data_length = 0;


  if (!PyArg_ParseTuple(args, "iis#", &friendnumber, &filenumber, &data,
        &data_length)) {
    return NULL;
  }

  int ret = tox_file_send_data(self->tox, friendnumber, filenumber, data,
      data_length);

  if (!ret) {
    PyErr_SetString(PyExc_TypeError, "tox_file_send_data() failed");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_filedata_size(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  if (!PyArg_ParseTuple(args, "i", &friendnumber)) {
    return NULL;
  }

  int ret = tox_file_data_size(self->tox, friendnumber);

  if (!ret) {
    PyErr_SetString(PyExc_TypeError, "tox_file_data_size() failed");
    return NULL;
  }

  return PyInt_FromLong(ret);
}

static PyObject*
ToxCore_file_dataremaining(ToxCore* self, PyObject* args)
{
  int friendnumber = 0;
  int filenumber = 0;
  uint8_t send_receive = 0;


  if (!PyArg_ParseTuple(args, "iic", &friendnumber, &filenumber,
        &send_receive)) {
    return NULL;
  }

  uint64_t ret = tox_file_data_remaining(self->tox, friendnumber, filenumber,
      send_receive);

  if (!ret) {
    PyErr_SetString(PyExc_TypeError, "tox_file_data_remaining() failed");
    return NULL;
  }

  return PyLong_FromUnsignedLongLong(ret);
}

static PyObject*
ToxCore_bootstrap_from_address(ToxCore* self, PyObject* args)
{
  int ipv6enabled = TOX_ENABLE_IPV6_DEFAULT;
  int port = 0;
  uint8_t* public_key = NULL;
  char* address = NULL;
  int addr_length = 0;
  int pk_length = 0;

  if (!PyArg_ParseTuple(args, "s#iis#", &address, &addr_length, &ipv6enabled,
        &port, &public_key, &pk_length)) {
    return NULL;
  }

  int ret = tox_bootstrap_from_address(self->tox, address, ipv6enabled, port,
      public_key);

  if (!ret) {
    PyErr_SetString(ToxCoreError, "failed to resolve address");
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_isconnected(ToxCore* self, PyObject* args)
{
  if (tox_isconnected(self->tox)) {
    Py_RETURN_TRUE;
  } else {
    Py_RETURN_FALSE;
  }
}

static PyObject*
ToxCore_kill(ToxCore* self, PyObject* args)
{
  tox_kill(self->tox);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_do(ToxCore* self, PyObject* args)
{
  tox_do(self->tox);

  Py_RETURN_NONE;
}

static PyObject*
ToxCore_size(ToxCore* self, PyObject* args)
{
  uint32_t size = tox_size(self->tox);

  return PyLong_FromUnsignedLong(size);
}

static PyObject*
ToxCore_save(ToxCore* self, PyObject* args)
{
  PyObject* res = NULL;
  int size = tox_size(self->tox);
  uint8_t* buf = (uint8_t*)malloc(size);

  if (buf == NULL) {
    return PyErr_NoMemory();
  }

  tox_save(self->tox, buf);

  res = PyString_FromStringAndSize((const char*)buf, size);
  free(buf);

  return res;
}

static PyObject*
ToxCore_save_to_file(ToxCore* self, PyObject* args)
{
  char* filename;
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
    PyErr_SetString(ToxCoreError, "tox_save(): can't open file for saving");
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
  uint8_t* data = NULL;
  int length = 0;

  if (!PyArg_ParseTuple(args, "s#", &data, &length)) {
    PyErr_SetString(PyExc_TypeError, "no data specified");
    return NULL;
  }

  if (tox_load(self->tox, data, length) == -1) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}

static PyObject*
ToxCore_load_from_file(ToxCore* self, PyObject* args)
{
  char* filename = NULL;

  if (!PyArg_ParseTuple(args, "s", &filename)) {
    return NULL;
  }

  FILE* fp = fopen(filename, "r");
  if (fp == NULL) {
    PyErr_SetString(PyExc_TypeError,
        "tox_load(): failed to open file for reading");
    return NULL;
  }

  size_t length = 0;
  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char* data = (char*)malloc(length * sizeof(char));

  if (fread(data, length, 1, fp) != 1) {
    PyErr_SetString(PyExc_TypeError,
        "tox_load(): corrupted data file");
    return NULL;
  }

  if (tox_load(self->tox, data, length) == -1) {
    Py_RETURN_FALSE;
  } else {
    Py_RETURN_TRUE;
  }
}

static PyMethodDef Rabin_methods[] = {
  {"on_friendrequest", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_friendmessage", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_action", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_namechange", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_statusmessage", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_userstatus", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_read_receipt", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_connectionstatus", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_group_invite", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_group_message", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_group_namelistchange", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_file_sendrequest", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_file_control", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"on_file_data", (PyCFunction)ToxCore_callback_stub, METH_VARARGS, ""},
  {"getaddress", (PyCFunction)ToxCore_getaddress, METH_NOARGS,
    "return FRIEND_ADDRESS_SIZE byte address to give to others" },
  {"addfriend", (PyCFunction)ToxCore_addfriend, METH_VARARGS,
    "add a friend" },
  {"addfriend_norequest", (PyCFunction)ToxCore_addfriend_norequest,
    METH_VARARGS,
    "add a friend without sending request" },
  {"getfriend_id", (PyCFunction)ToxCore_getfriend_id, METH_VARARGS,
    "return the friend id associated to that client id" },
  {"getclient_id", (PyCFunction)ToxCore_getclient_id, METH_VARARGS,
    "Copies the public key associated to that friend id into client_id buffer"
  },
  {"delfriend", (PyCFunction)ToxCore_delfriend, METH_VARARGS,
    "Remove a friend" },
  {"get_friend_connectionstatus",
    (PyCFunction)ToxCore_get_friend_connectionstatus, METH_VARARGS,
    "Checks friend's connecting status" },
  {"friend_exists", (PyCFunction)ToxCore_friend_exists, METH_VARARGS,
    "Checks if there exists a friend with given friendnumber" },
  {"sendmessage", (PyCFunction)ToxCore_sendmessage, METH_VARARGS,
    "Send a text chat message to an online friend" },
  {"sendmessage_withid", (PyCFunction)ToxCore_sendmessage_withid, METH_VARARGS,
    "Send a text chat message to an online friend with id" },
  {"sendaction", (PyCFunction)ToxCore_sendaction, METH_VARARGS,
    "Send an action to an online friend" },
  {"sendaction_withid", (PyCFunction)ToxCore_sendaction_withid, METH_VARARGS,
    "Send an action to an online friend with id" },
  {"setname", (PyCFunction)ToxCore_setname, METH_VARARGS,
    "Set our nickname" },
  {"getselfname", (PyCFunction)ToxCore_getselfname, METH_NOARGS,
    "Get your nickname" },
  {"getname", (PyCFunction)ToxCore_getname, METH_VARARGS,
    "Get name of friendnumber and put it in name" },
  {"set_statusmessage", (PyCFunction)ToxCore_set_statusmessage, METH_VARARGS,
    "Set our user status message" },
  {"set_userstatus", (PyCFunction)ToxCore_set_userstatus, METH_VARARGS,
    "Set our user status" },
  {"get_statusmessage_size", (PyCFunction)ToxCore_get_statusmessage_size,
    METH_VARARGS,
    "return the length of friendnumber's status message, including null" },
  {"get_statusmessage", (PyCFunction)ToxCore_copy_statusmessage,
    METH_VARARGS,
    "get status message of a friend" },
  {"get_selfstatusmessage", (PyCFunction)ToxCore_copy_self_statusmessage,
    METH_NOARGS,
    "get status message of yourself" },
  {"get_userstatus", (PyCFunction)ToxCore_get_userstatus,
    METH_VARARGS,
    "get friend status" },
  {"get_selfuserstatus", (PyCFunction)ToxCore_get_selfuserstatus,
    METH_VARARGS,
    "get self userstatus" },
  {"set_sends_receipts", (PyCFunction)ToxCore_set_send_receipts,
    METH_VARARGS,
    "Sets whether we send read receipts for friendnumber." },
  {"count_friendlist", (PyCFunction)ToxCore_count_friendlist,
    METH_NOARGS,
    "Return the number of friends" },
  {"friendlist", (PyCFunction)ToxCore_copy_friendlist,
    METH_NOARGS,
    "Copy a list of valid friend IDs into the array out_list" },
  {"add_groupchat", (PyCFunction)ToxCore_add_groupchat, METH_VARARGS,
    "Creates a new groupchat and puts it in the chats array"},
  {"del_groupchat", (PyCFunction)ToxCore_del_groupchat, METH_VARARGS,
    "Delete a groupchat from the chats array"},
  {"group_peername", (PyCFunction)ToxCore_group_peername, METH_VARARGS,
    "get the group peer's name"},
  {"invite_friend", (PyCFunction)ToxCore_invite_friend, METH_VARARGS,
    "invite friendnumber to groupnumber "},
  {"join_groupchat", (PyCFunction)ToxCore_join_groupchat, METH_VARARGS,
    "Join a group (you need to have been invited first.)"},
  {"group_message_send", (PyCFunction)ToxCore_group_message_send, METH_VARARGS,
    "send a group message"},
  {"group_number_peers", (PyCFunction)ToxCore_group_number_peers, METH_VARARGS,
    "Return the number of peers in the group chat on success."},
  {"group_names", (PyCFunction)ToxCore_group_copy_names, METH_VARARGS,
    "List all the peers in the group chat."},
  {"count_chatlist", (PyCFunction)ToxCore_count_chatlist, METH_VARARGS,
    "Return the number of chats in the instance m."},
  {"chatlist", (PyCFunction)ToxCore_copy_chatlist, METH_VARARGS,
    "Copy a list of valid chat IDs into the array out_list."},
  {"new_filesender", (PyCFunction)ToxCore_new_filesender, METH_VARARGS,
    "Send a file send request."},
  {"file_sendcontrol", (PyCFunction)ToxCore_file_sendcontrol, METH_VARARGS,
    "Send a file control request."},
  {"file_senddata", (PyCFunction)ToxCore_file_senddata, METH_VARARGS,
    "Send file data."},
  {"filedata_size", (PyCFunction)ToxCore_filedata_size, METH_VARARGS,
    "Returns the recommended/maximum size of the filedata you send with "
    "tox_file_send_data()"},
  {"file_dataremaining", (PyCFunction)ToxCore_file_dataremaining, METH_VARARGS,
    "Give the number of bytes left to be sent/received."},
  {"bootstrap_from_address", (PyCFunction)ToxCore_bootstrap_from_address,
    METH_VARARGS,
    "Resolves address into an IP address. If successful, sends a 'get nodes'"
    "request to the given node with ip, port " },
  {"isconnected", (PyCFunction)ToxCore_isconnected, METH_NOARGS,
    "return False if we are not connected to the DHT." },
  {"kill", (PyCFunction)ToxCore_kill, METH_NOARGS,
    "Run this before closing shop" },
  {"do", (PyCFunction)ToxCore_do, METH_NOARGS,
    "The main loop that needs to be run at least 20 times per second" },
  {"size", (PyCFunction)ToxCore_size, METH_NOARGS,
    "return size of messenger data (for saving)." },
  {"save", (PyCFunction)ToxCore_save, METH_NOARGS,
    "Save the messenger in data" },
  {"save_to_file", (PyCFunction)ToxCore_save_to_file, METH_VARARGS,
    "Save the messenger to a file" },
  {"load", (PyCFunction)ToxCore_load, METH_VARARGS,
    "Load the messenger from data of size length." },
  {"load_from_file", (PyCFunction)ToxCore_load_from_file, METH_VARARGS,
    "Load the messenger from file" },
  {NULL}
};

PyTypeObject ToxCoreType = {
  PyObject_HEAD_INIT(NULL)
  0,                         /*ob_size*/
  "core.Tox",                /*tp_name*/
  sizeof(ToxCore),             /*tp_basicsize*/
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
  "ToxCore object",            /* tp_doc */
  0,                         /* tp_traverse */
  0,                         /* tp_clear */
  0,                         /* tp_richcompare */
  0,                         /* tp_weaklistoffset */
  0,                         /* tp_iter */
  0,                         /* tp_iternext */
  Rabin_methods,             /* tp_methods */
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
