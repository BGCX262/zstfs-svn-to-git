/***************************************************************************
*                       Huffman Encoding and Decoding
*
*   File    : huffman.c
*   Purpose : Use huffman coding to compress/decompress files
*   Author  : Michael Dipperstein
*   Date    : November 20, 2002
*
****************************************************************************
*   UPDATES
*
*   Date        Change
*   10/21/03    Fixed one symbol file bug discovered by David A. Scott
*   10/22/03    Corrected fix above to handle decodes of file containing
*               multiples of just one symbol.
*   11/20/03    Correcly handle codes up to 256 bits (the theoretical
*               max).  With symbol counts being limited to 32 bits, 31
*               bits will be the maximum code length.
*
*   $Id: huffman.c,v 1.9 2007/09/20 03:30:06 michael Exp $
*   $Log: huffman.c,v $
*   Revision 1.9  2007/09/20 03:30:06  michael
*   Changes required for LGPL v3.
*
*   Revision 1.8  2005/05/23 03:18:04  michael
*   Moved internal routines and definitions common to both canonical and
*   traditional Huffman coding so that they are only declared once.
*
*   Revision 1.7  2004/06/15 13:37:29  michael
*   Change function names and make static functions to allow linkage with chuffman.
*
*   Revision 1.6  2004/02/26 04:55:04  michael
*   Remove main(), allowing code to be generate linkable object file.
*
*   Revision 1.4  2004/01/13 15:49:45  michael
*   Beautify header
*
*   Revision 1.3  2004/01/13 05:45:17  michael
*   Use bit stream library.
*
*   Revision 1.2  2004/01/05 04:04:58  michael
*   Use encoded EOF instead of counting characters.
*
****************************************************************************
*
* Huffman: An ANSI C Huffman Encoding/Decoding Routine
* Copyright (C) 2002-2005, 2007 by
* Michael Dipperstein (mdipper@alumni.engr.ucsb.edu)
*
* This file is part of the Huffman library.
*
* The Huffman library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of the
* License, or (at your option) any later version.
*
* The Huffman library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
* General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************/

/***************************************************************************
*                             INCLUDED FILES
***************************************************************************/
#include "../../include.h"
#include "huflocal.h"
#define ERR ~0

/***************************************************************************
*                            TYPE DEFINITIONS
***************************************************************************/

/***************************************************************************
*                                CONSTANTS
***************************************************************************/

/***************************************************************************
*                                 MACROS
***************************************************************************/


#define PLB(pointer) (*pointer % 8) /* POSITION IN BIT AT LAST BYTE */
#define TOBYTE(pointer) (*pointer / 8)


/***************************************************************************
*                            GLOBAL VARIABLES
***************************************************************************/

/***************************************************************************
*                               PROTOTYPES
***************************************************************************/
static int ReadHeader(huffman_node_t **ht, char *bfp, unsigned sc, long *pointer);
char getBit( char *stream,int len, long *pointer );
int getBits(char *stream, int len, void *bits, const unsigned int count, long *pointer);
char getch(char *stream, char *ris,unsigned sc, long *pointer);

/***************************************************************************
*                                FUNCTIONS
***************************************************************************/

char getBit( char *stream,int len, long *pointer ){
 
  if( TOBYTE(pointer) + 1 > len )
    return ERR;
  
  (*pointer)++;
 
  return (*(stream + TOBYTE(pointer))  >> ( 8 - PLB(pointer) )) & 1;
}

/****************************************************************************
*   Function   : HuffmanDecodeFile
*   Description: This routine reads a Huffman coded file and writes out a
*                decoded version of that file.
*   Parameters : inFile - Name of file to decode
*                outFile - Name of file to write a tree to
*   Effects    : Huffman encoded file is decoded
*   Returned   : TRUE for success, otherwise FALSE.
****************************************************************************/
int HuffmanDecodeFile(char *inFile, unsigned sc, char *outFile, unsigned sd)
{
    huffman_node_t *huffmanTree, *currentNode;
    int  c,id=0; /* index destination */
    long i,pointer=0;
    
    /* allocate array of leaves for all possible characters */
    for (i = 0; i < NUM_CHARS; i++)
    {
        if ((huffmanArray[i] = AllocHuffmanNode(i)) == NULL)
        {
            /* allocation failed clear existing allocations */
            for (i--; i >= 0; i--)
            {
                kfree(huffmanArray[i]);
            }

            return FALSE;
        }
    }

    /* populate leaves with frequency information from file header */
    if (!ReadHeader(huffmanArray, inFile, sc, &pointer))
    {
        for (i = 0; i < NUM_CHARS; i++)
        {
            kfree(huffmanArray[i]);
        }

        return FALSE;
    }

    /* put array of leaves into a huffman tree */
    if ((huffmanTree = BuildHuffmanTree(huffmanArray, NUM_CHARS)) == NULL)
    {
        FreeHuffmanTree(huffmanTree);

        return FALSE;
    }

    /* now we should have a tree that matches the tree used on the encode */
    currentNode = huffmanTree;
    printk( "huf decode:\n" );
    for( i = pointer ; i < (sc*8);  )
    {
      printk( "i:%i\n", (int) i);
      c = getBit( inFile, sc, &i );
      if( c == ERR )
	printk("ZST Warning: HuffmanDecode couldn't read bit from stream\n" );
      
        /* traverse the tree finding matches for our characters */
        if (c != 0)
        {
	    currentNode = currentNode->right;
        }
        else
        {
	  
            currentNode = currentNode->left;
	}
	/*qui*/
	if (currentNode->value != COMPOSITE_NODE)
        {
	    /* we've found a character */
            if (currentNode->value == EOF_CHAR)
            {
                /* we've just read the EOF */
                break;
            }

	    *(outFile+id) = currentNode->value; /* write out character */
	    id++;
            currentNode = huffmanTree;          /* back to top of tree */
        }
	
    }
    FreeHuffmanTree(huffmanTree);     /* free allocated memory */

    return TRUE;
}

/****************************************************************************
*   Function   : ReadHeader
*   Description: This function reads the header information stored by
*                WriteHeader.  If the same algorithm that produced the
*                original tree is used with these counts, an exact copy of
*                the tree will be produced.
*   Parameters : ht - pointer to array of pointers to tree leaves
*                inFile - file to read from
*   Effects    : Frequency information is read into the node of ht
*   Returned   : TRUE for success, otherwise FALSE
****************************************************************************/


int getBits(char *stream, int len, void *bits, const unsigned int count, long *pointer)
{
    unsigned char *bytes, shifts;
    int offset, remaining;
    char returnValue;

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
      if ( TOBYTE(pointer) + 1 > len)
        {
	  return ERR;
        }

      
      if( getch( stream, &returnValue, len, pointer ) == ERR )
	return ERR;
      
      bytes[offset] = (unsigned char)returnValue;
      remaining -= 8;
      offset++;
    }
    
    if (remaining != 0)
    {
        /* read remaining bits */
        shifts = 8 - remaining;
	
        while (remaining > 0)
        {

	  returnValue = getBit(stream, len, pointer);
	  

            if (returnValue == ERR)
            {
	      return ERR;
            }

            bytes[offset] <<= 1;
            bytes[offset] |= (returnValue & 0x01);
	    remaining--;
        }

        /* shift last bits into position */
        bytes[offset] <<= shifts;
    }
    
    return count;
}


char getch(char *stream, char *ris,unsigned sc, long *pointer)
{
    unsigned char uno,due;
   
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
    *ris = uno << PLB(pointer) | (due >> (8 - PLB(pointer)))/* & mask */ ;
    *pointer += 8; /* pointer to first bit unreaded */

    return 0;
}

static int ReadHeader(huffman_node_t **ht, char *bfp, unsigned sc, long *pointer)
{
    count_t count;
    int i=0;
    int c;
    int status = FALSE;     /* in case of premature EOF */
   
    printk( "ZST: sc: %d count_t: %d\n", sc, sizeof(count_t) );
    
    while ( getch(bfp,(char *)&c,sc,pointer) != ERR )
    {
      ++i;
      getBits(bfp, sc, (void *)(&count), 8 * sizeof(count_t), pointer);
      
        if ((count == 0) && (c == 0))
        {
            /* we just read end of table marker */
            status = TRUE;
            break;
        }

        ht[c]->count = count;
        ht[c]->ignore = FALSE;
    }
  
    printk( "i: %d\n", i );
    
    /* add assumed EOF */
    ht[EOF_CHAR]->count = 1;
    ht[EOF_CHAR]->ignore = FALSE;

    if (status == FALSE)
    {
        /* we hit EOF before we read a full header */
        printk("ZST error: malformed file header.\n");
    }

    return status;
}
