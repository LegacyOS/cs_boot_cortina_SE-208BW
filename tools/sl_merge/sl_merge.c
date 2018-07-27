/*-----------------------------------------------------------------------------------
*	sl_merge
*
*	Description:
*		To merge several files into a specified destination file
*	
*	Syntax: 
*		sl_merge [output] [size] [file-1] [location] [file-2] [location] ...
*
*	History:
*
*	4/18/2005	Gary Chen	Create
*
*-------------------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define WRITE_BUF_SIZE  32*1024
unsigned char buf[WRITE_BUF_SIZE];

char syntax[]="Cortina merge file utility Version 1.0.0\n"
			   "Syntax: "
			   "cs_merge [output] [size] [file-1] [location] [file-2] [location] ...\n";
			   
static int merge_a_file(FILE *ofile, char *sfile, unsigned long location);
static unsigned long file_size(FILE *infile);
static unsigned long ascii2hex(unsigned char c);
static unsigned long ascii2decimal(unsigned char c);
static unsigned long string2hex(unsigned char *str_p);
static unsigned long string2decimal(unsigned char *str_p);
static unsigned long string2value(unsigned char *str_p);

			   
/*----------------------------------------------------------------------
* Main
* Description:	
*		Main entry for sl_merge command
* Parameters :	
*		int argc	:  
*		char *argv[]:
* Return:
*		int			: return code. 
*						0: OK, 1: Failed
*----------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	FILE *ofile;
	unsigned long output_size, total;
	unsigned long location;
	int num, rc;
   
	if (argc < 7 || !(argc & 1))
	{
		printf(syntax);
		exit(1);
	}
	
	ofile = fopen(argv[1], "wb+");
	if (ofile == NULL)
	{
		printf("Failed to open output file: %s\n", argv[1]);
		printf(syntax);
		exit(1);
	}

	total=output_size=string2value(argv[2]);

	num=3;

	while (num < argc) 
	{
		location = string2value(argv[num+1]);
		if ((rc = merge_a_file(ofile, argv[num], location)) != 0)
			return rc;
		
		num += 2;	
	}
	
	fclose(ofile);
	
	exit(0);
}


/*----------------------------------------------------------------------
* merge_a_file
* Description:	
*		Merge source files to destination file at specified location
* Parameters :	
*		FILE *ofile		: destination file handle
*		char *sfile		: Source file name
*		unsigned long location	: location
* Return:
*		int			: return code.
*					  0: OK, 
*					  -1: Failed to open file
*					  -2: Failed to Write data into destination file
*----------------------------------------------------------------------*/
static int merge_a_file(FILE *ofile, char *sfile, unsigned long location)
{
    unsigned long  size, total_size;
	FILE    *infile;

    fseek(ofile,location,SEEK_SET);
	infile=fopen(sfile, "rb");
	if(infile==NULL)
	{
	   printf("Error opening %s for INPUT\n", sfile);
	   return -1;
	}


    total_size=0;
    while(1)
    {
        size=fread(buf,1,WRITE_BUF_SIZE,infile);
        if (size==0) break;
        if (fwrite(buf,1,size, ofile) != size)
        {
            printf("Error Writing data to OUTPUT file");
            return -2;
		}
    }
	
	fclose(infile);
	return 0;
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

