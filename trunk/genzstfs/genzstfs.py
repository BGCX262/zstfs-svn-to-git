#!/usr/bin/env python

# $ compresstree.py --help
# USAGE: compresstree.py [source]

# $ fixoutput.py 
# Usage: fixoutput.py src dst [-o]

"""
USAGE:
    genzst -f dest_fs [-V label] [-b cb_size] [-c compression_type]
         -b: 1024, 2048, ... ,33554432
         -c: zlib, bz2, obsolete, bwt, huffman
"""

import sys,os

verbose = False

if os.system('which genromfs > /dev/null'):
    print 'install genromfs'
    exit(1)

def help():
    print __doc__
    exit(1)

if 'help' in sys.argv[1:] or '-h' in sys.argv[1:] or '--help' in sys.argv[1:]:
    help()

dst = None
path = ''
label = None
cbsize = None
ctype = None
#deprecated option: *NOT USE IT*
if '-o' in sys.argv[1:] or '--offset' in sys.argv[1:]:
    offset = True
else:
    offset = False
for i,o in enumerate(sys.argv[1:-1]):
    if o=='-f':
        dst = sys.argv[i+2]
    elif o=='-V':
        label = sys.argv[i+2]
    elif o=='-b':
        cbsize = sys.argv[i+2]
    elif o=='-c':
        ctype = sys.argv[i+2]
if not dst:
    help()

if verbose:
    print path,dst

if os.path.exists('.zst'):
    os.system('rm -R .zst')

assert ctype != 'lz' or int(cbsize) <= 2**16

cmds = [
    'compresstree.py %s %s' % (cbsize and '-b '+cbsize or '', ctype and '-c '+ctype or ''),
    'genromfs -f .zst/romfs -d .zst/files %s' % (label and '-V '+label or ''),
    'fixoutput.py .zst/romfs %s %s %s' % (dst, cbsize and '-b '+cbsize or '', ctype and '-c '+ctype or ''),
    'rm -R .zst'
    ]

if verbose:
    print cmds

exit( os.system(cmds[0]) or
      os.system(cmds[1]) or
      os.system(cmds[2]) or
      os.system(cmds[3]) )
