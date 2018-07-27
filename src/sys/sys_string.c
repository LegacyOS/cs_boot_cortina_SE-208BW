/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_mem.c
* Description	: 
*		Handle 
*			(1) memset
*			(2) memcpy
*			(3) memcmp
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
// #include <stddef.h>
#include <board_config.h>
#include <sl2312.h>

extern int flash_read_buf(void* addr, void* buff, int len);

//#ifdef FLASH_TYPE_NAND
int nand_present=0;
#define ERROR	-1
//#endif
/*--------------------------------------------------------------
* 	sys_memset
---------------------------------------------------------------*/
void *sys_memset(void *datap, int data, int size )
{
	unsigned char *pByte;

	pByte = (unsigned char *)datap;
	while (size--)
		*pByte++=data; 
	
	return (0);
}

/*--------------------------------------------------------------
* 	sys_memcpy
---------------------------------------------------------------*/
void *sys_memcpy(void *dest, const void *srce, int size )
{
	unsigned char *pSrce, *pDest;
	unsigned int    value;
	
	value = REG32(SL2312_FLASH_CTRL_BASE + 0x0c) & 0x1f00;	
	if((value&0x1000)==0x1000)
	{
		if( ((unsigned long)srce>=BOARD_FLASH_BASE_ADDR) && 
		((unsigned long)srce<0x40000000) && nand_present)
		{
			nand_read(dest,srce,size);
		}
		else
		{
			pSrce = (unsigned char *)srce;
			pDest= (unsigned char *)dest;
			
			while (size--)
			{	*pDest++=*pSrce++; }
		}
	}
	else
	{
		pSrce = (unsigned char *)srce;
		pDest= (unsigned char *)dest;
		
		while (size--)
		{		*pDest++=*pSrce++; }
	}

	return (0);
}


/*--------------------------------------------------------------
* 	sys_memcmp
---------------------------------------------------------------*/
int sys_memcmp(const void *dest, const void *srce, int size )
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (size--)
	{
		if (*pDest++ != *pSrce++)
			return(1);
	}
	
	return(0); 
}

/*--------------------------------------------------------------
* 	sys_strrchr
---------------------------------------------------------------*/
char *sys_strrchr(const char *s, int c)
{
       const char *p = s + sys_strlen(s);
       do {
           if (*p == (char)c)
               return (char *)p;
       } while (--p >= s);
       return NULL;
}

/*--------------------------------------------------------------
* 	sys_strchr
---------------------------------------------------------------*/
char *sys_strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *)s;
}

/*--------------------------------------------------------------
* 	sys_memicmp
---------------------------------------------------------------*/
int sys_memicmp(const void *dest, const void *srce, int size )
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (size--)
	{
		if (toupper(*pDest) != toupper(*pSrce))
			return(1);
		
		pDest++;
		pSrce++;
	}
	
	return(0); 
}

/*--------------------------------------------------------------
* 	sys_strcpy
---------------------------------------------------------------*/
void sys_strcpy(void *dest, const void *srce)
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (*pSrce)
		*pDest++=*pSrce++; 

	*pDest = 0x00;
}

/*--------------------------------------------------------------
* 	sys_strlen
---------------------------------------------------------------*/
int sys_strlen(const void *srce)
{
	int len;
	unsigned char *pSrce;
	
	pSrce = (unsigned char *)srce;
	
	len = 0;
	while (*pSrce++)
		len++;

	return len;
}

/*--------------------------------------------------------------
* 	sys_strcmp
---------------------------------------------------------------*/
int sys_strcmp(void *dest, const void *srce)
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (*pSrce && *pDest)
	{
		if (*pDest != *pSrce)
			return(1);
		pDest++;
		pSrce++;
	}
	
	if (*pSrce || *pDest)
		return 1;
	else
		return 0;
}

/*--------------------------------------------------------------
* 	sys_strncmp
---------------------------------------------------------------*/
int sys_strncmp(void *dest, const void *srce, int len)
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (len--)
	{
		if (*pDest != *pSrce)
			return(1);
		pDest++;
		pSrce++;
	}
	
	return 0;
}

/*--------------------------------------------------------------
* 	sys_strncasecmp
---------------------------------------------------------------*/
int sys_strncasecmp(void *dest, const void *srce)
{
	unsigned char *pSrce, *pDest;
	
	pSrce = (unsigned char *)srce;
	pDest= (unsigned char *)dest;
	
	while (*pSrce && *pDest)
	{
		if (toupper(*pDest) != toupper(*pSrce))
			return(1);
		pDest++;
		pSrce++;
	}
	
	if (*pSrce || *pDest)
		return 1;
	else
		return 0;
}

/*--------------------------------------------------------------
* 	sys_strstr
---------------------------------------------------------------*/
char *sys_strstr(char *s1,  char *s2)
{
	int l1, l2;

	l2 = sys_strlen(s2);
	if (!l2)
		return (char *)s1;
	l1 = sys_strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!sys_memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}

/*----------------------------------------------------------------------
* char2hex
*----------------------------------------------------------------------*/
UINT32 char2hex(UINT8 c)
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
* char2decimal
*----------------------------------------------------------------------*/
UINT32 char2decimal(UINT8 c)
{
	if (c >= '0' && c <= '9')
		return (c - '0');
	else
		return (0xffffffff);
}


/*----------------------------------------------------------------------
* str2hex
*----------------------------------------------------------------------*/
UINT32 str2hex(UINT8 *cp)
{
    UINT32 value, result;

    result = 0;
    if (*cp=='0' && toupper(*(cp+1))=='X')
    	cp += 2;
    
    while ((value = char2hex(*cp)) != 0xffffffff)
    {
          result = result * 16 + value;
          cp++;
    }

    return(result);
}

/*----------------------------------------------------------------------
* str2decimal
*----------------------------------------------------------------------*/
UINT32 str2decimal(UINT8 *cp)
{
    UINT32 value, result;

	result=0;
    while ((value = char2decimal(*cp)) != 0xffffffff)
    {
		result= result * 10 + value;
		cp++;
    }

    return(result);
}


/*----------------------------------------------------------------------
* str2value
*----------------------------------------------------------------------*/
UINT32 str2value(UINT8 *strp)
{
    UINT32	value, result;
    char	*cp;
    int		is_hex = 0;

    cp = strp;
    if (*cp=='0' && toupper(*(cp+1))=='X')
    {
    	strp += 2;
    	is_hex = 1;
    }
    
    // check 
    cp = strp;
    while (*cp && *cp != ' ' && *cp != ':' && *cp != ',' && *cp != '.')
    {
    	if (ishex(*cp))
    		is_hex = 1;
		else if (!isdigit(*cp))
			return 0;
		cp++;
	}
    
    return (is_hex) ? str2hex(strp) : str2decimal(strp);
    	
}


/*----------------------------------------------------------------------
* str2ip
*----------------------------------------------------------------------*/
UINT32 str2ip(UINT8 *cp)
{
    UINT32 i, value, ip[4], result;

	result=0;
	for (i=0; i<4; i++)
	{
		ip[i] = 0;
    	while ((value = char2decimal(*cp)) != 0xffffffff)
    	{
			ip[i] = ip[i] * 10 + value;
			cp++;
    	}
    	if (i < 3 && *cp != '.')
    		return 0;
    	cp++;
	}    	

    return IPIV(ip[0], ip[1], ip[2], ip[3]);
}


//#ifdef FLASH_TYPE_NAND	
int nand_read(void *dst,void *src,size_t count)
{
#ifdef BOARD_SUPPORT_NAND_INDIRECT
	unsigned int *addr=(src-SL2312_FLASH_BASE);
#else
	unsigned int *addr=src;
#endif
	
	unsigned char *buff=dst;
#ifdef BOARD_NAND_BOOT	

	//read src addr is block alignment
	if(flash_read_buf(addr,buff,(int)count)){
		printf("ERROR: copy from NAND Flash Fail!!\n");
		return ERROR;
	}
	else
		return 0;
#endif		
}
//#endif
