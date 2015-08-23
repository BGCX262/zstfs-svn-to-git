
#include  <stdio.h>
#include  "lz.h"

#define   OurIdentifier  0xa5b6c7d8

#define   Header         "\
\n\
   This program is only purpose is to demonstrate the working of the\n\
   LZRW1KH (de)compression routines which can be found in the header\n\
   file LZRW1KH.h !  This  program  simply  takes  two  parameters :\n\
   the name of a source file and the name  of the destination  file.\n\
   If the source file has  already been compressed,  it will be  de-\n\
   compressed and stored into the  destination file.  Otherwise  the\n\
   process will compress the file !\n\
\n\
   Usage :  COMPRESS <Source> <Destination>\n\
\n\
   Program and compression routines written by Kurt Haenen.\n\
\n"

          main(ac,av)
int       ac;
char      *av[];
{    FILE      *FP1,*FP2;
     ulong     Identifier;
     ubyte     Source[16388],Dest[16388];
     uword     SourceSize,DestSize;
     printf(Header);
     if (ac != 3)
          {    printf("Illegal number of parameters !\n");
               return 1;
          }
     if ((FP1 = fopen(av[1],"rb")) == NULL)
          {    printf("File %s not found !\n",av[1]);
               return 1;
          }
     if ((FP2 = fopen(av[2],"wb")) == NULL)
          {    fclose(FP1);
               printf("Couldn't open %s for output !\n",av[2]);
               return 1;
          }
     if (((SourceSize = fread(&Identifier,sizeof(ulong),1,FP1)) != 1)
               || (Identifier != OurIdentifier))
          {    Identifier = OurIdentifier;
               printf("Trying to compress %s to %s !\n",av[1],av[2]);
               puts("writing filesize");
               if (fwrite(&Identifier,sizeof(ulong),1,FP2) != 1)
                    {    fclose(FP1);
                         fclose(FP2);
                         printf("Couldn't write to %s !\n",av[2]);
                         return 1;
                    }
               if (fseek(FP1,0L,SEEK_SET) != 0)
                    {    fclose(FP1);
                         fclose(FP2);
                         printf("Couldn't seek in %s !\n",av[1]);
                         return 1;
                    }
               for (; (SourceSize = fread(Source,1,16384,FP1)) != 0;)
                    {    DestSize = Compression(Source,Dest,SourceSize);
                         puts("writing block to disk");
                         if ((fwrite(&DestSize,sizeof(uword),1,FP2)  != 1)
                                   || (fwrite(Dest,1,DestSize,FP2) != DestSize))
                              {    fclose(FP1);
                                   fclose(FP2);
                                   printf("Couldn't write to %s !\n",av[2]);
                                   return 1;
                              }
                    }
          }
     else {    printf("Trying to decompress %s to %s !\n",av[1],av[2]);
               for (; (fread(&SourceSize,sizeof(uword),1,FP1) == 1);)
                    {    if (fread(Source,1,SourceSize,FP1) != SourceSize)
                              {    fclose(FP1);
                                   fclose(FP2);
                                   printf("Unexspected EOF in file %s!\n",av[1]);
                                   return 1;
                              }
                         DestSize = Decompression(Source,Dest,SourceSize);
                         if (fwrite(Dest,1,DestSize,FP2) != DestSize)
                              {    fclose(FP1);
                                   fclose(FP2);
                                   printf("Couldn't write to %s !\n",av[2]);
                                   return 1;
                              }
                    }
          }
     fclose(FP1);
     fclose(FP2);
}
