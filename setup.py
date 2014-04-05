from distutils.core import setup, Extension

setup(
    name="PyTox",
    version="0.0.12",
    description = 'Python binding for Tox the skype replacement',
    author = 'Wei-Ning Huang (AZ)',
    author_email = 'aitjcize@gmail.com',
    url = 'http://github.com/aitjcize/PyTox',
    license = 'GPL',
    ext_modules=[
        Extension("tox", [
                "tox/tox.c",
                "tox/core.c",
                "tox/av.c",
                "tox/util.c",
       ],
       extra_compile_args=['-Wno-declaration-after-statement'],
       libraries=['toxcore', 'toxav'])
    ]
)
