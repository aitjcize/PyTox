from distutils.core import setup, Extension
from subprocess import Popen, PIPE


sources = ["pytox/pytox.c", "pytox/core.c", "pytox/av.c", "pytox/util.c"]
libraries = [
  "opus",
  "sodium",
  "toxcore",
#  "toxcrypto",
#  "toxdht",
#  "toxdns",
#  "toxencryptsave",
#  "toxfriends",
#  "toxgroup",
#  "toxmessenger",
#  "toxnetcrypto",
#  "toxnetwork",
  "vpx",
]
cflags = [
  "-Wall",
  # "-Werror",
  "-Wextra",
  "-Wno-declaration-after-statement",
  "-Wno-missing-field-initializers",
  "-Wno-unused-parameter",
  "-fno-strict-aliasing",
]

setup(
    name="PyTox",
    version="0.0.24",
    description='Python binding for Tox the skype replacement',
    author='Wei-Ning Huang (AZ)',
    author_email='aitjcize@gmail.com',
    url='http://github.com/aitjcize/PyTox',
    license='GPL',
    ext_modules=[
        Extension(
            "pytox",
            sources,
            extra_compile_args=cflags,
            libraries=libraries
        )
    ]
)
