# simple script to test the completeness of the python bindings:

from sys import version_info

if version_info[0] < 3:
    from urllib import urlopen
else:
    from urllib.request import urlopen

import re


TOXURL="https://raw.github.com/irungentoo/ProjectTox-Core/master/toxcore/tox.h"
PYTOXURL="https://raw.github.com/aitjcize/PyTox/master/tox/core.c"

toxsrc = urlopen(TOXURL).read()
pytoxsrc = urlopen(PYTOXURL).read()

res = re.findall("[\\n|n][a-z0-9]+ (tox\_[\_a-z]+\()", str(toxsrc))

incl = 0
excl = []

for function in res:
    if function in str(pytoxsrc):
        incl += 1
    else:
        excl.append(function)
       
       
print("PyTox includes %d out of %d functions found in tox.h" % (incl, len(res)))
print("Not included are the functions:")
for item in excl:
    print("  %s" % item[:-1])






