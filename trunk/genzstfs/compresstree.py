#!/usr/bin/env python
# given a directory (current) compress it
# in .zst/files

# *Note*: the index of files *here* is only the offset from the beginning of data.
#         In zstfs is a pointer. This must be corrected after call to read genromfs

import sys,os,md5
from pprint import pprint

try:
    import zlib
except ImportError:
    print "zlib compression not supported"
    zlib_works = False
else:
    zlib_works = True

zstpath = '.zst'
verbose = False

# big endian
binint = lambda i: chr(i>>24&0xff)+chr(i>>16&0xff)+chr(i>>8&0xff)+chr(i&0xff)
readbinint = lambda s: (ord(s[0])<<24)+(ord(s[1])<<16)+(ord(s[2])<<8)+(ord(s[3]))

def obsoletecompress(data):
    return '[BEGIN BLOCK>'+data+'<END BLOCK]'

def zlibcompress(data):
    if not zlib_works:
        exit(1)
    return zlib.compress(data,9)

def bz2compress(data):
    print "not yet supported"
    return data

def bwtcompress(data):
    length = len(data)
    i,o = os.popen2('bwtencode')
    i.write(data)
    i.close()
    pi = o.read(4)
    #print "Primary index:",readbinint(pi)
    bwtdata = o.read()
    o.close()
    out = []
    
    if bwtdata:
        data = bwtdata
    #print data
    prev = data[0]
    count = 0
    for curr in data[1:]:
        #print prev,curr
        if prev==curr and count<255:
            count += 1
        else:
            out.append(prev)
            assert count<=255
            out.append(chr(count))
            count = 0
        prev = curr
        out.append(prev)
    assert count<=255
    out.append(chr(count))
    return pi+''.join(out)

#test bwtcompress
if False:
    print bwtcompress(sys.argv[1])
    exit(0)

def huffmancompress(data):
    f = file(os.path.join(zstpath,'.huffmanin'),'w')
    f.write(data)
    f.close()
    if os.system('huffman081 -c -i%s -o%s' % (os.path.join(zstpath,'.huffmanin'),os.path.join(zstpath,'.huffmanout'))):
        print "ERROR: huffman 0.81"
        print 'huffman081 -c -i%s -o%s' % (os.path.join(zstpath,'.huffmanin'),os.path.join(zstpath,'.huffmanout'))
        exit(1)
    f = file(os.path.join(zstpath,'.huffmanout'))
    data = f.read()
    f.close()
    return data

if False:
    print len(huffmancompress(sys.argv[1]))
    exit(0)

def lzcompress(data):
    f = file(os.path.join(zstpath,'.lzin'),'w')
    f.write(data)
    f.close()
    if os.system('lzcompress %s %s > /dev/null' % (os.path.join(zstpath,'.lzin'),os.path.join(zstpath,'.lzout'))):
        print "ERROR: lzcompress"
        exit(1)
    f = file(os.path.join(zstpath,'.lzout'))
    data = f.read()
    f.close()
    return data

def mkpath(fullpath):
    if not fullpath: return
    if not os.path.exists(fullpath):
        path = os.path.split(fullpath)[0]
        mkpath(path)
        os.mkdir(fullpath)

def getfiles(basedir,dir=''):
    curpath = os.path.join(basedir,dir)
    ret = []
    for f in os.listdir(curpath):
        if os.path.isdir(os.path.join(curpath,f)):
            ret += getfiles(basedir,os.path.join(dir,f))
        else:
            ret.append(os.path.join(dir,f))
    return ret

# big endian
binint = lambda i: chr(i>>24&0xff)+chr(i>>16&0xff)+chr(i>>8&0xff)+chr(i&0xff)

# TEST
if False:
    print "Test Mode"

    bsize = 1024
    itype = 1
    pointersize = 4

    #readbinint = lambda s: len(s)==4 and ord(s[0])<<24+ord(s[1])<<16+ord(s[2])<<8+ord(s[3]) or None
    def readbinint(s):
        return (ord(s[0])<<24)+(ord(s[1])<<16)+(ord(s[2])<<8)+(ord(s[3]))

    print readbinint(chr(0)+chr(0)+chr(0)+chr(1))
    #exit(1)

    for i in xrange(10000,40000000,17):
        if i != readbinint(binint(i)):
            print "Error binint"
            print i,len(binint(i)),readbinint(binint(i))
            
    f = '.zst/romfs.txt'

    data = file(f).read()
    
    # it's too difficult test this script... i'll assume that there's non bug

    exit(1)

def main():
    if '-h' in sys.argv[1:] or '--help' in sys.argv[1:]:
        print "USAGE: compresstree.py [-b cb_size] [-c compression_type]"
        print "   -b: 1024*, 2048, ... ,33554432"
        print "   -c: obsolete*, zlib, bz2, bwt, huffman, lz"
        print " (* default)"
        exit(1)

    #get src and dst
    src = os.path.abspath(os.getcwd())

    dst = os.path.join(src,zstpath)
    fdst = os.path.join(dst,'files')
    mkpath(fdst)

    #more setting:
    if '-b' in sys.argv[1:-1]:
        bsize = int(sys.argv[sys.argv.index('-b')+1])
        assert bsize % 1024 == 0
        assert bsize <= 2 ** 25 and bsize>0
    else:
        bsize = 1024

    #index
    # 1. 4 byte for each pointer
    #    A pointer is the offset from
    #    the begin of index
    itype = 1
    pointersize = 4

    #compression
    if '-c' in sys.argv[1:-1]:
        comptype = sys.argv[sys.argv.index('-c')+1]
    else:
        comptype = 'obsolete'
    try:
        compress = {
            'obsolete':obsoletecompress,
            'zlib':zlibcompress,
            'bz2':bz2compress,
            'bwt':bwtcompress,
            'huffman':huffmancompress,
            'lz':lzcompress,
            }[comptype]
    except KeyError:
        print "wrong compression type"
        exit(1)

    if verbose:
        print "cbsize = %d" % bsize
        print "ctype = %s" % comptype

        print 'src',src
        print 'dst',dst

    files = [f for f in getfiles(src) if not f.startswith(zstpath)]
    #remove .zst special dir from input

    if verbose:
        print "Number of file:",len(files)

    for fname in files:
        s = os.path.join(src,fname)
        d = os.path.join(fdst,fname)
        
        data = file(s).read()
        cdata = '' #compressed data
        findex = [] #file index

        for i in xrange(0,len(data),bsize):
            cblock = compress(data[i:i+bsize])
            findex.append(len(cdata))
            cdata += cblock

        #fix index: i must add the findex len (in byte)
        #           and convert it in binary form (big endian)
        if len(findex)==1:
            #file is smaller than a block
            # *index is useless*
            findex=[]
        else:
            findex = [binint(x+len(findex)*pointersize+4) for x in findex]
            # +4 ---> real size field

        mkpath(os.path.split(d)[0])
        # len(data) --> real size
        file(d,'w').write(binint(len(data))+''.join(findex)+cdata)

        #save an info file to fixoutput script
        #m = md5.new()
        #m.update(''.join(findex)+cdata)
        #file(os.path.join(dst,m.hexdigest()),'w').write(str(len(data)))
    

if __name__=="__main__":
    main()
