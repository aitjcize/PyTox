#ifndef __AV_H__
#define __AV_H__

#include <Python.h>
#include <tox/toxav.h>

/* ToxAV definition */
typedef struct {
  PyObject_HEAD
  PyObject* core;
  ToxAv* av;
  vpx_image_t* in_image;
  uint32_t i_w, i_h;
  unsigned char* out_image;
  uint32_t o_w, o_h;
  ToxAvCSettings cs;
} ToxAV;

void ToxAV_install_dict(void);

#endif /* __AV_H__ */
