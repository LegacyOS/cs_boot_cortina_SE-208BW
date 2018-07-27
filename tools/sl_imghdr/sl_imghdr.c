/*-----------------------------------------------------------------------------------
*	sl_imghdr
*
*	Description:
*		To add a image header on top of the specified file
*	
*	Syntax: 
*		sl_imghdr [input file] [output file]
*
*	History:
*
*	9/14/2005	Gary Chen	Create
*
*-------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define BUF_SIZE  (32*1024)
unsigned char buf[BUF_SIZE];

char syntax[]="Cortina imghdr utility Version 1.0.0\n"
			   "Syntax: "
			   "cs_imghdr [input file] [output file]\n";
			   
static int imghdr_a_file(FILE *ofile, char *sfile, unsigned long location);
static unsigned long file_size(FILE *infile);
static unsigned long ascii2hex(unsigned char c);
static unsigned long ascii2decimal(unsigned char c);
static unsigned long string2hex(unsigned char *str_p);
static unsigned long string2decimal(unsigned char *str_p);
static unsigned long string2value(unsigned char *str_p);
extern unsigned short sys_crc16(unsigned short crc, unsigned char *datap, unsigned long len);
extern unsigned short sys_gen_crc16(unsigned char *datap, unsigned long len);

#define IMGHDR_SIZE			32
#define IMGHDR_NAME_SIZE	16
#define IMGHDR_NAME			"CS-BOOT-001"
typedef struct {
	unsigned char 	name[IMGHDR_NAME_SIZE];
	unsigned long	file_size;
	unsigned char	reserved[IMGHDR_SIZE-IMGHDR_NAME_SIZE-4-2];
	unsigned short	checksum;
} IMGHDR_T;

IMGHDR_T imghdr;
			   
/*----------------------------------------------------------------------
* Main
* Description:	
*		Main entry for sl_imghdr command
* Parameters :	
*		int argc	:  
*		char *argv[]:
* Return:
*		int			: return code. 
*						0: OK, 1: Failed
*----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	FILE			*infile;
	FILE			*ofile;
	unsigned long	size, total_size;
	unsigned short	crc16;
   
	if (argc != 3)
	{
		printf(syntax);
		exit(1);
	}

	memset((void *)&imghdr, 0, sizeof(IMGHDR_T));
	strcpy(imghdr.name, IMGHDR_NAME);
	
	infile = fopen(argv[1], "rb");
	if (infile == NULL)
	{
		printf("Failed to open input file: %s\n", argv[1]);
		printf(syntax);
		exit(1);
	}
	imghdr.file_size = file_size(infile);
	if (imghdr.file_size == 0)
	{
		printf("Input file size is zero! (%s)\n", argv[1]);
		printf(syntax);
		fclose(infile);
		exit(1);
	}
	
	ofile = fopen(argv[2], "wb+");
	if (ofile == NULL)
	{
		printf("Failed to create output file: %s\n", argv[2]);
		printf(syntax);
		fclose(infile);
		exit(1);
	}
	
    crc16 = 0xffff;
    total_size = 0;
    while(1)
    {
        size = fread(buf, 1, BUF_SIZE, infile);
        if (size == 0)
        	break;
        
        crc16 = sys_crc16(crc16, buf, size);
        total_size += size;
    }
    
    crc16 ^= 0xffff;
	imghdr.checksum = crc16;
	
   // write header
	if (fwrite((void *)&imghdr, 1, sizeof(IMGHDR_T), ofile) != sizeof(IMGHDR_T))
	{
		printf("Error Writing data to OUTPUT file\n");
		fclose(ofile);
		fclose(infile);
		exit(1);
	}
    
    rewind(infile);
    while(1)
    {
        size = fread(buf, 1, BUF_SIZE, infile);
        if (size == 0)
        	break;
        	
        if (fwrite(buf, 1, size, ofile) != size)
        {
            printf("Error Writing data to OUTPUT file\n");
            break;
        }
    }
	
	printf("Sucessful to add image header! Size=%u, CRC=0x%04X\n",
			imghdr.file_size, imghdr.checksum);
			
	fclose(ofile);
	fclose(infile);
	exit(0);
}



/*----------------------------------------------------------------------
* file_size
* Description:	
*		To get the size of a specified file
* Parameters :	
*		FILE *infile	: Source file handle
* Return:
*		unsigned long	: file size
*----------------------------------------------------------------------*/
static unsigned long file_size(FILE *infile)
{
	long size;
   
	fseek(infile,0L,SEEK_END);
	size = ftell(infile);
	rewind(infile);
   
	return (unsigned long)size;
}


/*----------------------------------------------------------------------
* ascii2hex
* Description:	
*		To convert a character into hexdecimal
* Parameters :	
*		unsigned char c	: 
* Return:
*		unsigned long	: result
*----------------------------------------------------------------------*/
static unsigned long ascii2hex(unsigned char c)
{
	if (c >= '0' && c <= '9')
	        return (c - '0');
	else if (c >= 'a' && c <= 'f')
	        return (c - 'a' + 10);
	else if (c >= 'A' && c <= 'F')
	        return (c - 'A' + 10);
	else
	        return (0xffffffff);
}

/*----------------------------------------------------------------------
* ascii2decimal
* Description:	
*		To convert a character into decimal value
* Parameters :	
*		unsigned char c	: 
* Return:
*		unsigned long	: result
*----------------------------------------------------------------------*/
static unsigned long ascii2decimal(unsigned char c)
{
	if (c >= '0' && c <= '9')
	        return (c - '0');
	else
	        return (0xffffffff);
}


/*----------------------------------------------------------------------
* string2hex
* Description:	
*		To convert a string int a hexdecimal value
* Parameters :	
*		unsigned char *str_p	: points a source string
* Return:
*		unsigned long	: result
*----------------------------------------------------------------------*/
static unsigned long string2hex(unsigned char *str_p)
{
    unsigned long i, result;

    result=0;
    if (*str_p=='0' && toupper(*(str_p+1))=='X') str_p+=2;
    while ((i=ascii2hex(*str_p))!=0xffffffff)
    {
          result=(result)*16+i;
          str_p++;
    }
    while (*str_p==' '|| *str_p==',') str_p++; /* skip space */

    return(result);

}

/*----------------------------------------------------------------------
* string2decimal
* Description:	
*		To convert a string int a decimal value
* Parameters :	
*		unsigned char *str_p	: points a source string
* Return:
*		unsigned long	: result
*----------------------------------------------------------------------*/
static unsigned long string2decimal(unsigned char *str_p)
{
    unsigned long i, result;

    result=0;
    while ((i=ascii2decimal(*str_p))!=0xffffffff){
          result=(result)*10+i;
          str_p++;
    }
    while (*str_p==' '|| *str_p==',') str_p++; /* skip space */

    return(result);

}

/*----------------------------------------------------------------------
* string2value
* Description:	
*		To convert a string int a decimal or hex value
* Parameters :	
*		unsigned char *str_p	: points a source string
* Return:
*		unsigned long	: result
*----------------------------------------------------------------------*/
static unsigned long string2value(unsigned char *str_p)
{

	if (str_p[0]=='0' && (str_p[1]=='x' || str_p[1]=='X'))
	{
		return(string2hex(str_p+2));
	}
	else
	{
		return(string2decimal(str_p));
	}
}

