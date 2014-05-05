#ifndef __AV_H__
#define __AV_H__

#include <Python.h>
#include <tox/toxav.h>

/* ToxAV definition */
typedef struct {
  PyObject_HEAD
  PyObject* core;
  ToxAv* av;
  int16_t* pcm;
  vpx_image_t* in_image;
  uint32_t o_w, o_h;
  unsigned char* out_image;
  ToxAvCodecSettings cs;
} ToxAV;

void ToxAV_install_dict();

#endif /* __AV_H__ */
