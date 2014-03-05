/**
 * @file   tox.c
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

#include <stdio.h>

extern PyTypeObject ToxCoreType;
extern PyObject* ToxCoreError;

extern PyTypeObject ToxAVType;
extern PyObject* ToxAVError;

extern void ToxCore_install_dict();

#if PY_MAJOR_VERSION >= 3
struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "tox",
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
PyMODINIT_FUNC PyInit_tox(void)
{
  PyObject *m = PyModule_Create(&moduledef);
#else
PyMODINIT_FUNC inittox(void)
{
  PyObject *m = Py_InitModule("tox", NULL);
#endif

  if (m == NULL) {
    goto error;
  }

  ToxCore_install_dict();

  // Initialize toxcore
  if (PyType_Ready(&ToxCoreType) < 0) {
    fprintf(stderr, "Invalid PyTypeObject `ToxCoreType'\n");
    goto error;
  }

  Py_INCREF(&ToxCoreType);
  PyModule_AddObject(m, "Tox", (PyObject*)&ToxCoreType);

  ToxCoreError = PyErr_NewException("tox.OperationFailedError", NULL, NULL);
  PyModule_AddObject(m, "CoreOperationFailedError", (PyObject*)ToxCoreError);

  // Initialize toxav
  if (PyType_Ready(&ToxAVType) < 0) {
    fprintf(stderr, "Invalid PyTypeObject `ToxAVType'\n");
    goto error;
  }

  Py_INCREF(&ToxAVType);
  PyModule_AddObject(m, "ToxAV", (PyObject*)&ToxAVType);

  ToxAVError = PyErr_NewException("toxav.OperationFailedError", NULL, NULL);
  PyModule_AddObject(m, "AVOperationFailedError", (PyObject*)ToxAVError);

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
