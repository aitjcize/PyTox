from distutils.core import setup, Extension
from subprocess import Popen, PIPE


def supports_av():
    h = Popen("ld $LDFLAGS -ltoxav", shell=True, stderr=PIPE)
    out, err = h.communicate()
    return 'toxav' not in str(err)

sources = ["pytox/pytox.c", "pytox/core.c", "pytox/util.c"]
libraries = [
  "opus",
  "sodium",
  "toxcore",
#  "toxcrypto",
#  "toxdht",
  "toxdns",
  "toxencryptsave",
#  "toxfriends",
#  "toxgroup",
#  "toxmessenger",
#  "toxnetcrypto",
#  "toxnetwork",
  "vpx",
]
cflags = [
  "-Wall",
  "-Werror",
  "-Wextra",
  "-Wno-declaration-after-statement",
  "-Wno-missing-field-initializers",
  "-Wno-unused-parameter",
  "-fno-strict-aliasing",
]

if supports_av():
    libraries.append("toxav")
    sources.append("pytox/av.c")
    cflags.append("-DENABLE_AV")
else:
    print("Warning: AV support not found, disabled.")

setup(
    name="PyTox",
    version="0.0.23",
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
