#ifndef LZ_H
#define LZ_H


#define   FLAG_Copied         0x80
#define   FLAG_Compress       0x40
#define   TRUE                1
#define   FALSE               0
typedef   signed char         byte;
typedef   signed short        word;
typedef   unsigned char       ubyte;
typedef   unsigned short      uword;
/*typedef   unsigned long       ulong;*/
/*typedef   unsigned char       bool;*/

uword     Decompression(ubyte *Source,ubyte *Dest,uword SourceSize);



#endif
