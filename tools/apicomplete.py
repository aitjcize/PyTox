# simple script to test the completeness of the python bindings:

from sys import version_info

if version_info[0] < 3:
    from urllib import urlopen
else:
    from urllib.request import urlopen

import re


TOXURL =\
    "https://raw.githubusercontent.com/irungentoo/toxcore/master/toxcore/tox.h"
PYTOXURL =\
    "https://raw.githubusercontent.com/aitjcize/PyTox/master/pytox/core.c"
PYTOXURL =\
    "https://raw.githubusercontent.com/kitech/PyTox/newapi/pytox/core.c"

toxsrc = urlopen(TOXURL).read()
pytoxsrc = urlopen(PYTOXURL).read()

res = None
if version_info[0] < 3:
    res = re.findall(r"\n[_a-z0-9]+ (tox_[\_a-z]+\()", str(toxsrc))
else:
    res = re.findall(r'[_a-z0-9]+ (tox_[\_a-z]+\()', str(toxsrc))

incl = 0
excl = []

for function in res:
    if function in str(pytoxsrc):
        incl += 1
    else:
        excl.append(function)


print(
    "PyTox includes %d out of %d functions found in tox.h" % (incl, len(res))
)

print("Not included are the functions:")
for item in excl:
    print("  %s" % item[:-1])
