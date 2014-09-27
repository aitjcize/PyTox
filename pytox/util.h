/**
 * @file   util.h
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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <Python.h>

#define CHECK_TOX(self)                                        \
  if ((self)->tox == NULL) {                                   \
    PyErr_SetString(ToxOpError, "toxcore object killed.");     \
    return NULL;                                               \
  }

#if PY_MAJOR_VERSION < 3
  #define PYSTRING_FromString PyString_FromString
  #define PYSTRING_FromStringAndSize PyString_FromStringAndSize
  #define PYSTRING_Check PyString_Check

  #define PYBYTES_FromStringAndSize PyString_FromStringAndSize
#else
  #define PYSTRING_FromString PyUnicode_FromString
  #define PYSTRING_FromStringAndSize PyUnicode_FromStringAndSize
  #define PYSTRING_Check PyUnicode_Check

  #define PYBYTES_FromStringAndSize PyBytes_FromStringAndSize
#endif

void bytes_to_hex_string(const uint8_t* digest, int length, uint8_t* hex_digest);

void hex_string_to_bytes(uint8_t* hexstr, int length, uint8_t* bytes);

void PyStringUnicode_AsStringAnsSize(PyObject* object, char** str,
    Py_ssize_t* len);

#endif /* __UTIL_H__ */
