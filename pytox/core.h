#ifndef __CORE_H__
#define __CORE_H__

#include <Python.h>
#include <tox/tox.h>

/* ToxAV definition */
typedef struct {
  PyObject_HEAD
  Tox* tox;
} ToxCore;

void ToxCore_install_dict(void);

#endif /* __CORE_H__ */
