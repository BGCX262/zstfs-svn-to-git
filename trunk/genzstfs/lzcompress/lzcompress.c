#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include "lz.h"

#define MAXSIZE 33554432+1 /*EOF*/

// uword     Compression(Source,Dest,SourceSize)

int main(int argc, char *argv[])
{
  byte *bufin;
  byte *bufout;
  int primary_index, size=0, i, maxsize=MAXSIZE;

  /*
  if(argc>1){
    maxsize = atoi(argv[1]);
  }
  */
  
  bufin = malloc(maxsize);
  bufout = malloc(maxsize);

  while(!feof(stdin)){
    scanf("%c",bufin+size);
    size++;
    /*fprintf(stderr,"%d\n",size);*/
    assert(size<=maxsize);
  }
  size--;
  /*
  for(i=0 ; i<size ; i++)
    putc(bufin[i],stderr);
  */

  if (argc>1 && strcmp(argv[1][0],"-d")==0)
    size = Decompression(bufin, bufout, size);
  else
    size = Compression(bufin, bufout, size);

  for(i=0 ; i<size ; i++)
    putchar(bufout[i]);

  return 0;
}
