/**
 * @file   tox.c
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
#include <stdio.h>

#include "core.h"

#ifdef ENABLE_AV
  #include "av.h"
#endif

extern PyObject* ToxOpError;
extern PyTypeObject ToxCoreType;

#ifdef ENABLE_AV
extern PyTypeObject ToxAVCoreType;
#endif

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "pytox",
  "Python Toxcore module",
  -1,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_pytox(void)
{
  PyObject *m = PyModule_Create(&moduledef);
#else
PyMODINIT_FUNC initpytox(void)
{
  PyObject *m = Py_InitModule("pytox", NULL);
#endif

  if (m == NULL) {
    goto error;
  }

  ToxCore_install_dict();
#ifdef ENABLE_AV
  ToxAVCore_install_dict();
#endif

  // Initialize toxcore
  if (PyType_Ready(&ToxCoreType) < 0) {
    fprintf(stderr, "Invalid PyTypeObject `ToxCoreType'\n");
    goto error;
  }

  Py_INCREF(&ToxCoreType);
  PyModule_AddObject(m, "Tox", (PyObject*)&ToxCoreType);

  ToxOpError = PyErr_NewException("pytox.OperationFailedError", NULL, NULL);
  PyModule_AddObject(m, "OperationFailedError", (PyObject*)ToxOpError);

#ifdef ENABLE_AV
  // Initialize toxav
  if (PyType_Ready(&ToxAVCoreType) < 0) {
    fprintf(stderr, "Invalid PyTypeObject `ToxAVCoreType'\n");
    goto error;
  }

  Py_INCREF(&ToxAVCoreType);
  PyModule_AddObject(m, "ToxAV", (PyObject*)&ToxAVCoreType);
#endif

#if PY_MAJOR_VERSION >= 3
  return m;
#endif

error:
#if PY_MAJOR_VERSION >= 3
  return NULL;
#else
  return;
#endif
}
