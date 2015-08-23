#include <stdio.h>
#include <assert.h>
#include <malloc.h>

typedef unsigned char byte;

byte *rotlexcmp_buf = NULL;
int rottexcmp_bufsize = 0;
 
int rotlexcmp(const void *l, const void *r)
{
    int li = *(const int*)l, ri = *(const int*)r, ac=rottexcmp_bufsize;
    if(li == ri) return 0;
    while (rotlexcmp_buf[li] == rotlexcmp_buf[ri])
    {
        if (++li == rottexcmp_bufsize)
            li = 0;
        if (++ri == rottexcmp_bufsize)
            ri = 0;
        if (!--ac)
            return 0;
    }
    if (rotlexcmp_buf[li] > rotlexcmp_buf[ri])
        return 1;
    else
        return -1;
}
 
void bwt_encode(byte *buf_in, byte *buf_out, int size, int *primary_index)
{
    int indices[size];
    int i;
 
    for(i=0; i<size; i++)
        indices[i] = i;
    rotlexcmp_buf = buf_in;
    rottexcmp_bufsize = size;
    qsort (indices, size, sizeof(int), rotlexcmp);
 
    for (i=0; i<size; i++)
        buf_out[i] = buf_in[(indices[i]+size-1)%size];
    for (i=0; i<size; i++)
    {
        if (indices[i] == 1) {
            *primary_index = i;
            return;
        }
    }
    assert (0);
}

#define MAXSIZE 33554432+1 /*EOF*/

int main(int argc, char *argv[])
{
  byte *bufin;
  byte *bufout;
  int primary_index, size=0, i, maxsize=MAXSIZE;

  if(argc>1){
    maxsize = atoi(argv[1]);
  }
  
  bufin = malloc(maxsize);
  bufout = malloc(maxsize);

  while(!feof(stdin)){
    scanf("%c",bufin+size);
    size++;
    assert(size<=maxsize);
  }
  size--;
  bwt_encode (bufin, bufout, size, &primary_index);
  
  //fprintf(stderr,"primary index: %d\n",primary_index);

  /* little endian to big endian */
  primary_index = (
     (primary_index&0xff)<<24)+
    ((primary_index&0xff00)<<8)+
    ((primary_index&0xff0000)>>8)+
    ((primary_index>>24)&0xff
     );
  /*
  int big_primary_index;
  char *lE = (char*) &primary_index;
  char *bE = (char*) &big_primary_index;
  bE[0] = lE[3];
  bE[1] = lE[2];
  bE[2] = lE[1];
  bE[3] = lE[0];

  printf( "%d,%d\n", primary_index, big_primary_index ); 
  */
  fwrite(&primary_index,sizeof(primary_index),1,stdout);
  for(i=0 ; i<size ; i++)
    putchar(bufout[i]);

  return 0;
}
