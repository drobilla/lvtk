#!/bin/bash

from subprocess import Popen
from subprocess import PIPE
import glob

def include_what_you_use (file):
    iwyu = ['include-what-you-use', '-Iinclude', '-Isrc' ]
    cmd = iwyu
    cmd.append (file)
    process = Popen(cmd, stdout=PIPE)
    (output, err) = process.communicate()
    if err: 
        print("ERROR:", err)
        exit (99999)
    process.wait()
    return process.returncode

files  = ['include/lvtk/lvtk.h']
files += glob.glob ('include/lvtk/*.hpp')
files += glob.glob ('include/lvtk/ext/*.hpp')
files += glob.glob ('include/lvtk/host/*.hpp')
files += glob.glob ('include/lvtk/ui/*.hpp')

for ext in 'cpp hpp'.split():
    files += glob.glob ('src/*.%s' % ext)
    files += glob.glob ('src/host/*.%s')
    files += glob.glob ('src/ui/*.%s')
    files += glob.glob ('src/ui/details/*.%s')

for f in files:
    code = include_what_you_use (f)
    if (code > 2): exit (code)

exit(0)