#!/usr/bin/env python

__doc__="""
Usage: fixoutput.py src dst [-o] [-c ...] [-b ...]
    -o   No fix index
    ...  see genzstfs --help
"""

import sys,math,md5

verbose = False

# this script requires *a lot* of ram with big files

# TODO:
# set checksum both files and filesystem

#settings:
magic_zst = 'zst0'

if '-b' in sys.argv[1:-1]:
    bsize = int(sys.argv[sys.argv.index('-b')+1])
    assert bsize % 1024 == 0
    assert bsize <= 2 ** 25 and bsize>0
else:
    bsize = 1024

#compression
if '-c' in sys.argv[1:-1]:
    comptype = sys.argv[sys.argv.index('-c')+1]
else:
    comptype = 'obsolete'
try:
    ctype = {
        'obsolete':1,
        'bz2':2,
        'zlib':3,
        'bwt':4,
        'huffman':5,
        'lz':6,
        }[comptype]
except KeyError:
    print "wrong compression type"
    exit(1)

if verbose:
    print "ctype =",ctype

# big endian
binint = lambda i: chr(i>>24&0xff)+chr(i>>16&0xff)+chr(i>>8&0xff)+chr(i&0xff)
readbinint = lambda s: (ord(s[0])<<24)+(ord(s[1])<<16)+(ord(s[2])<<8)+(ord(s[3]))

cstring = lambda s: s[:s.index(chr(0))]

def read_hpointers(f,hp):
    #Doesn't preserve cursor
    #print "hp",hp
    if not hp:
        return []
    f.seek(hp)
    first = readbinint(f.read(4))
    newhp = first & 0xfffffff0
    ftype = first & 0x7
    assert hp!=newhp
    sub = []
    if ftype == 1:
        #is dir
        spec = readbinint(f.read(4)) & 0xfffffff0
        #print "spec",spec
        if spec != hp:
            sub = read_hpointers(f,spec)
    #print "return"
    return [hp] + sub + read_hpointers(f,newhp)

def main():
    try:
        src = file(sys.argv[1])
        dst = file(sys.argv[2],'w')
    except IndexError:
        print __doc__
        exit(1)

    if '-o' in sys.argv[1:]:
        indexoffset = True
    else:
        indexoffset = False

    #check magic number
    magic = src.read(8)
    if magic!="-rom1fs-":
        print "Source is not a romfs"
        exit(2)
    #write magic number
    dst.write(magic_zst)#4

    #write ctype (8) and bsyze (4)
    dst.write(chr(ctype)+chr(int(math.log(bsize/1024,2))<<4)+chr(0)+chr(0))#8

    #copy full size
    dst.write(src.read(4))#12

    #set checksum to 0
    dst.write(chr(0)*4)#16

    #may i should check checksum
    src.read(4)

    #volumename
    name=''
    buf=''
    counter=16#keeps the bytes from beginning
    while not chr(0) in buf:
        buf=src.read(16)
        counter += 16
        name += buf
    dst.write(name)
    if verbose:
        print "Volume name",cstring(name)

        print "Length header:",counter,dst.tell()

    # -- END HEADER FILE SYSTEM --

    #get all file pointer
    
    hpointers = read_hpointers(src,counter)
    #print len(hpointers)
    hpointers.sort() # i think that thi is useless
    #print hpointers

    #if True:
    #for each file
    for fenum,int_hpointer in enumerate(hpointers):
        if verbose:
            print "file: %d counter %d" % (fenum,counter)
        
        assert counter == dst.tell()
        
        src.seek(int_hpointer)
        
        #header pointer is the same
        hpointer = src.read(4)
        int_ftype = readbinint(hpointer) & 0x7
            
        header = hpointer
        counter += 4

        #spec.info is the same
        header += src.read(4)
        counter += 4

        #size: this is a *problem*:
        #now is the compressed size of file
        #how does romfs use it? how does romfs use hpointer?
        size = src.read(4)
        int_size = readbinint(size)
        if verbose:
            print "file size",int_size
        header += size
        counter += 4

        #checksum
        src.read(4)
        #i must recompute it
        header += chr(0)*4
        counter += 4

        #filename: only copy it
        buf=''
        name = ''
        while not chr(0) in buf:
            buf = src.read(16)
            header += buf
            name += buf
            counter += 16
        
        #check if name is ASCII coded
        assert len(cstring(name)) == len([x for x in cstring(name) if x>=' ' and x<='~']) # 32 - 127

        if verbose:
            print "name:",cstring(name)

        data = ''

        #index: i must fix it
        offset = counter
        

        #read whole data of the current file
        readto = counter+int_size
        #print counter,readto
        while counter < readto:
            #print "counter %4d "%counter,
            data += src.read(16)
            counter += 16
            #if not counter%10**6:
            #    print "!",counter
            
        if verbose:
            print "size:",len(data),int_size
        assert len(data)-int_size<16
        #data = data[:int_size] #i must write all data, not only the file
        #print "@"+data+"@"

        #modify only regular files
        if not indexoffset and int_ftype==2:
            #data --> real size | index | real compress data
            realsize = readbinint(data[:4])
            #len index
            leni = realsize>bsize and realsize / bsize + (realsize % bsize and 1 or 0) or 0
            if verbose:
                print "real size:",realsize
                print "index size:",leni
            if leni:
                findex = []
                begincdata = 4+leni*4
                    #            ^---> real size
                for i in xrange(4,begincdata,4):
                    findex.append( binint(readbinint(data[i:i+4]) + offset) )
                data = binint(realsize) + ''.join(findex) + data[begincdata:]
                #compute checksum

        if verbose:
            print

        dst.write(header+data)

    # *whole size of the file system must be padded to an 1024 byte boundary*.
    dst.write( chr(0) * (1024-dst.tell()%1024) )

    src.close()
    dst.close()

if __name__=="__main__":
    main()
