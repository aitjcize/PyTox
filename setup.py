import setuptools

from distutils.core import setup, Extension
from subprocess import Popen, PIPE

def supports_av():
    h = Popen("ldconfig -p | grep toxav", shell=True, stdout=PIPE)
    out, err = h.communicate()
    return 'toxav' in str(out)

sources = ["tox/tox.c", "tox/core.c", "tox/util.c"]
libraries = ["toxcore"]
cflags = ["-g", "-Wno-declaration-after-statement"]

if supports_av():
    libraries.append("toxav")
    sources.append("tox/av.c")
    cflags.append("-DENABLE_AV")
else:
    print("Warning: AV support not found, disabled.")

setup(
    name="PyTox",
    version="0.0.21",
    description = 'Python binding for Tox the skype replacement',
    author = 'Wei-Ning Huang (AZ)',
    author_email = 'aitjcize@gmail.com',
    url = 'http://github.com/aitjcize/PyTox',
    license = 'GPL',
    ext_modules=[
       Extension("tox", sources,
       extra_compile_args=cflags,
       libraries=libraries)
    ]
)
