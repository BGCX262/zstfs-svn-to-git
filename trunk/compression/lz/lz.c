

/*   ###################################################################
     ##                                                               ##
     ##      ##    ##### #####  ##   ##  ##      ## ##  ## ##  ##     ##
     ##      ##      ### ##  ## ## # ## ###     ##  ## ##  ##  ##     ##
     ##      ##     ###  #####  #######  ##    ##   ####   ######     ##
     ##      ##    ###   ##  ## ### ###  ##   ##    ## ##  ##  ##     ##
     ##      ##### ##### ##  ## ##   ## #### ##     ##  ## ##  ##     ##
     ##                                                               ##
     ##   EXTREMELY FAST AND EASY TO UNDERSTAND COMPRESSION ALGORITM  ##
     ##                                                               ##
     ###################################################################
     ##                                                               ##
     ##   This header file implements the  LZRW1/KH algoritm which    ##
     ##   also implements  some RLE coding  which is usefull  when    ##
     ##   compressing files containing a lot of consecutive  bytes    ##
     ##   having the same value.   The algoritm is not as good  as    ##
     ##   LZH, but can compete with Lempel-Ziff.   It's the fasted    ##
     ##   one I've encountered upto now.                              ##
     ##                                                               ##
     ##                                                Kurt HAENEN    ##
     ##                                                               ##
     ###################################################################   */

#include "lz.h"


uword     Decompression(Source,Dest,SourceSize)
ubyte     *Source,*Dest;
uword     SourceSize;
{    uword     X                        = 3;
     uword     Y                        = 0;
     uword     Pos,Size,K;
     uword     Command                  = (Source[1] << 8) + Source[2];
     ubyte     Bit                      = 16;
     if (Source[0] == FLAG_Copied)
          {    for (Y = 1; Y < SourceSize; Dest[Y-1] = Source[Y++]);
               return(SourceSize-1);
          }
     for (; X < SourceSize;)
          {    if (Bit == 0)
                    {    Command = (Source[X++] << 8);
                         Command += Source[X++];
                         Bit = 16;
                    }
               if (Command & 0x8000)
                    {    Pos = (Source[X++] << 4);
                         Pos += (Source[X] >> 4);
                         if (Pos)
                              {    Size = (Source[X++] & 0x0f)+3;
                                   for (K = 0; K < Size; K++)
                                        Dest[Y+K] = Dest[Y-Pos+K];
                                   Y += Size;
                              }
                         else {    Size = (Source[X++] << 8);
                                   Size += Source[X++] + 16;
                                   for (K = 0; K < Size; Dest[Y+K++] = Source[X]);
                                   X++;
                                   Y += Size;
                              }
                    }
               else Dest[Y++] = Source[X++];
               Command <<= 1;
               Bit--;
          }
     return(Y);
}
