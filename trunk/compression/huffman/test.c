#include<stdio.h>
#include "huffman.h"
#define ERR 12
 
#define PLB(pointer) (*pointer % 8) /* POSITION IN BIT AT LAST BYTE */
#define TOBYTE(pointer) (*pointer / 8)

char getch(char *stream,unsigned sc, long *pointer)
{
    unsigned char returnValue = 0;
    unsigned char uno,due;
    unsigned char i;

    if (stream == NULL)
        return ERR;


      /* we can just get byte from file */
    if( PLB(pointer) == 0 ){
      *pointer += 8; /* pointer to first bit unreaded */
      return *(stream + TOBYTE(pointer) );
    }

    if( TOBYTE(pointer) + 2 > sc )
      return ERR;

    /* take the two bytes involved */
    uno = *( stream + TOBYTE(pointer) );
    due = *( stream + TOBYTE(pointer) + 1 );
    /*
      In python is simpler
        def getchar(uno, due, p):
	    return ( uno << p%8 | (due >> 8-p%8) & (2**(p%8)-1) ) & 0xff
    */

    /* i primi (*pointer%8) bit a 1 */
    /*
    mask = 1;
    for( i = 0; i < PLB(pointer) ; ++i )
      mask *= 2;
    mask--;
    */
    returnValue = uno << PLB(pointer) | (due >> 8 - PLB(pointer))/* & mask */ ;
    *pointer += 8; /* pointer to first bit unreaded */

    return returnValue;
}


char getBit( char *stream,int len, long *pointer ){
 
  if( TOBYTE(pointer) + 1 > len )
    return ERR;
  
  (*pointer)++;
 
  return (*(stream + TOBYTE(pointer))  >> ( 8 - PLB(pointer) )) & 1;
}


int getBits(char *stream, int len, void *bits, const unsigned int count, long *pointer)
{
    unsigned char *bytes, shifts;
    int offset, remaining, returnValue;

    bytes = (unsigned char *)bits;

    if ((stream == NULL) || (bits == NULL))
    {
        return ERR;
    }

    offset = 0;
    remaining = count;

    /* read whole bytes */
    while (remaining >= 8)
    {
      printf("len %d, dove sono: %d\n", len ,TOBYTE(pointer)+1 ); 
      if ( TOBYTE(pointer) + 1 > len)
        {
	  return ERR;
        }

      
      returnValue = getch( stream, len, pointer );
      printf( "pointer : %d ho letto %d\n", *pointer, returnValue );

      bytes[offset] = (unsigned char)returnValue;
      remaining -= 8;
      offset++;
    }
    printf( "byte: %d, pointer %d\n", bytes[offset-1], *pointer );

    if (remaining != 0)
    {
        /* read remaining bits */
        shifts = 8 - remaining;
	printf( "remaining %d\n", remaining );

        while (remaining > 0)
        {

	  returnValue = getBit(stream, len, pointer);
	  printf( "%d", returnValue );
	  

            if (returnValue == ERR)
            {
	      return ERR;
            }

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
	    printf( "bytes[offset]: %d\n", bytes[offset] & 1 );
            remaining--;
        }

        /* shift last bits into position */
        bytes[offset] <<= shifts;
	printf( "\nbit: %d\n", bytes[offset] );
    }
    
    return count;
}

/*
void printbit( char c ){
  int mask,i;
  
  mask = 1 << 7;

  for( i = 0 ; i < 8 ; ++i )
    printf( "%d", 
  
    }*/

int main(){
  char str[5] = {100,100,100,100,100};
  long p = 0;
  short ris=0;
  char c=0;
  long i;

  /*
	17990    401 25700
   */

  c = getch( str, 5, &p );

  for( i=0 ; i < 8 ;  )
    printf( "%d", getBit(&c,1, &i ) ); 


  getBits( str ,5, (void *)&ris, 10, &p );
  
  for( i = 0 ; i < 10 ;  )
    printf( "%d", getBit((char *)&ris,2, &i ) ); 

  
  printf( "\npointer %d\n", p );
  getBits( str, 5 , (void *)&ris, 10, &p );

    
  for( i = 0 ; i < 10 ;  )
    printf( "%d", getBit((char *)&ris,2, &i ) ); 

  putchar( '\n' );

  c = getch( str, 5, &p );

  for( i=0 ; i < 8 ;  )
    printf( "%d", getBit(&c,1, &i ) ); 

  /*
  p = 0;

  c=getch(str,5,&p);
  
  for( i = 0 ; i < 8 ;  )
    printf( "%d", getBit(&c ,1, &i ) ); 


  c=getch(str,5,&p);

  for( i = 0 ; i < 8 ;  )
    printf( "%d", getBit(&c ,1, &i ) ); 


  c=getch(str,5,&p);

  for( i = 0 ; i < 8 ;  )
    printf( "%d", getBit(&c ,1, &i ) ); 
  */

}
