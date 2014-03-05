#include "util.h"

void bytes_to_hex_string(uint8_t* digest, int length, uint8_t* hex_digest)
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

void hex_string_to_bytes(uint8_t* hexstr, int length, uint8_t* bytes)
{
  int i;
  for (i = 0; i < length; ++i) {
    bytes[i] = (hex_char_to_int(hexstr[2 * i]) << 4)
             | (hex_char_to_int(hexstr[2 * i + 1]));
  }
}

void PyStringUnicode_AsStringAnsSize(PyObject* object, char** str,
    Py_ssize_t* len)
{
#if PY_MAJOR_VERSION < 3
    PyString_AsStringAndSize(object, str, len);
#else
# if PY_MINOR_VERSION == 2
    *str = PyUnicode_AS_DATA(object);
    *len = PyUnicode_GET_DATA_SIZE(object);
# else
    *str = PyUnicode_AsUTF8AndSize(object, len);
# endif
#endif
}
