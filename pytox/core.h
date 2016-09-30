/**
 * @file   core.h
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

#ifndef PYTOX_CORE_H
#define PYTOX_CORE_H

#include <Python.h>
#include <tox/tox.h>

/* ToxAV definition */
typedef struct {
  PyObject_HEAD
  Tox* tox;
} ToxCore;

/* This needs to be extern as it's dynamically loaded by the Python interpreter. */
extern PyTypeObject ToxCoreType;

void ToxCore_install_dict(void);

#endif /* PYTOX_CORE_H */
