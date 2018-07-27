/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_printf.c
* Description	: 
*		Handle printf
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/19/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <stdarg.h>
#include <sl2312.h>

static void sys_write_num(
    UINT32  n,              /* number to write              */
    UINT8   base,           /* radix to write to            */
    UINT8   sign,           /* sign, '-' if -ve, '+' if +ve */
    UINT32  pfzero,         /* prefix with zero ?           */
    UINT8 	width           /* min width of number          */
    );

int sys_vprintf(const char *fmt, va_list ap);

static char *sprint_bufp = NULL;

/*--------------------------------------------------------------
* sys_printf
---------------------------------------------------------------*/
void sys_printf(char *fmt, ... )
{
    va_list ap;
    int ret;
    unsigned long old_ints;
    
	HAL_DISABLE_INTERRUPTS(old_ints);
	sprint_bufp = NULL;
    va_start(ap, fmt);
    ret = sys_vprintf(fmt, ap);
    va_end(ap);
    HAL_RESTORE_INTERRUPTS(old_ints);
    HAL_ENABLE_INTERRUPTS();
 }

/**
 * sscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	formatting of buffer
 * @...:	resulting arguments
 */
int sys_sscanf(unsigned char *buf, char *fmt, ...)
{
	va_list ap;
	int i;

	sprint_bufp = buf;
	va_start(ap,fmt);
	i = sys_vsscanf(buf,fmt,ap);
	va_end(ap);
	return i;
}


/*--------------------------------------------------------------
* sys_sprintf
---------------------------------------------------------------*/
int sys_sprintf(char *buf, char *fmt, ... )
{
    va_list ap;
    int ret;

	sprint_bufp = buf;
    va_start(ap, fmt);
    ret = sys_vprintf(fmt, ap);
    va_end(ap);
    
    return (int)(sys_strlen(buf));
    
 }

/*--------------------------------------------------------------
* printf_putc
---------------------------------------------------------------*/
static void printf_putc(UINT8 data)
{
	if (sprint_bufp)
	{
		if (data == '\b')
			sprint_bufp--;
		else
			*sprint_bufp++ = data;
		*sprint_bufp = 0x00;
	}
	else
	{
		uart_putc(data);
	}
}

#if 0
/*--------------------------------------------------------------
* printf_puts
---------------------------------------------------------------*/
static void printf_puts(UINT8 *datap)
{
	if (sprint_bufp)
	{
		strcpy(sprint_bufp, datap);
		sprint_bufp += strlen(datap);
	}
	else
	{
		uart_puts(datap);
	}
}
#endif

/*--------------------------------------------------------------
* sys_write_dec
---------------------------------------------------------------*/
static void sys_write_dec(INT32 n)
{
    UINT8 sign;

    if( n < 0 ) n = -n, sign = '-';
    else sign = '+';
    
    sys_write_num( n, 10, sign, FALSE, 0);
}

/*--------------------------------------------------------------
* sys_write_hex - Write hexadecimal value
---------------------------------------------------------------*/
static void sys_write_hex( UINT32 n)
{
    sys_write_num( n, 16, '+', FALSE, 0);
}    

/*--------------------------------------------------------------
* sys_write_hex - Write hexadecimal value
* Generic number writing function                                      
* The parameters determine what radix is used, the signed-ness of the  
* number, its minimum width and whether it is zero or space filled on  
* the left.                                                            
---------------------------------------------------------------*/
static void sys_write_long_num(
    UINT64  n,              /* number to write              */
    UINT8   base,           /* radix to write to            */
    UINT8   sign,           /* sign, '-' if -ve, '+' if +ve */
    UINT32  pfzero,         /* prefix with zero ?           */
    UINT8 	width           /* min width of number          */
    )
{
    char buf[32];
    int bpos;
    char bufinit = pfzero?'0':' ';
    char *digits = "0123456789ABCDEF";

    /* init buffer to padding char: space or zero */
    for( bpos = 0; bpos < (int)sizeof(buf); bpos++ ) buf[bpos] = bufinit;

    /* Set pos to start */
    bpos = 0;

    /* construct digits into buffer in reverse order */
    if( n == 0 ) buf[bpos++] = '0';
    else while( n != 0 )
    {
        UINT8 d = n % base;
        buf[bpos++] = digits[d];
        n /= base;
    }

    /* set pos to width if less. */
    if( width > bpos ) bpos = width;

    /* set sign if negative. */
    if( sign == '-' )
    {
        if( buf[bpos-1] == bufinit ) bpos--;
        buf[bpos] = sign;
    }
    else bpos--;

    /* Now write it out in correct order. */
    while( bpos >= 0 )
        printf_putc(buf[bpos--]);
}

/*--------------------------------------------------------------
* sys_write_num 
---------------------------------------------------------------*/
static void sys_write_num
(
    UINT32  n,              /* number to write              */
    UINT8   base,           /* radix to write to            */
    UINT8   sign,           /* sign, '-' if -ve, '+' if +ve */
    UINT32  pfzero,         /* prefix with zero ?           */
    UINT8 	width           /* min width of number          */
    )
{
    sys_write_long_num((long long)n, base, sign, pfzero, width);
}

/*--------------------------------------------------------------
* sys_check_string 
---------------------------------------------------------------*/
static UINT32 sys_check_string( const char *str )
{
    UINT32 result = TRUE;
    const char *s;

    if( str == NULL ) return FALSE;
    
    for( s = str ; result && *s ; s++ )
    {
        char c = *s;

        /* Check for a reasonable length string. */
        
        if( s-str > 256 ) result = FALSE;

    }

    return result;
}

/*--------------------------------------------------------------
* _cvt 
---------------------------------------------------------------*/
static int _cvt(unsigned long long val, char *buf, long radix, char *digits)
{
    char temp[80];
    char *cp = temp;
    int length = 0;

	
    if (val == 0) {
        /* Special case */
        *cp++ = '0';
    } else {
        while (val) {
        	if(radix == 0)
        	{
        		*buf = '\0';
    			return (length);
        	}
        		
            *cp++ = digits[(val % radix)&0xf];
            val /= radix;
        }
    }
    while (cp != temp) {
        *buf++ = *--cp;
        length++;
    }
    *buf = '\0';
    
    return (length);
}

#define is_digit(c) ((c >= '0') && (c <= '9'))
#define isspace(c)	((c) == ' ')
#define islower(c)	('a' <= (c) && (c) <= 'z')
#define unlikely(x)	(x)
#define INT_MAX		((int)(~0U>>1))
#define isxdigit(c)	(('0' <= (c) && (c) <= '9') \
			 || ('a' <= (c) && (c) <= 'f') \
			 || ('A' <= (c) && (c) <= 'F'))

static int skip_atoi(const char **s)
{
	int i, c;

	for (i = 0; '0' <= (c = **s) && c <= '9'; ++*s)
		i = i*10 + c - '0';
	return i;
}

unsigned long long simple_strtoull(const char *cp,char **endp,unsigned int base)
{
	unsigned long long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	} else if (base == 16) {
		if (cp[0] == '0' && toupper(cp[1]) == 'X')
			cp += 2;
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

/**
 * simple_strtoll - convert a string to a signed long long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
long long simple_strtoll(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoull(cp+1,endp,base);
	return simple_strtoull(cp,endp,base);
}

/**
 * simple_strtoul - convert a string to an unsigned long
 * @cp: The start of the string
 * @endp: A pointer to the end of the parsed string will be placed here
 * @base: The number base to use
 */
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((toupper(*cp) == 'X') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	} else if (base == 16) {
		if (cp[0] == '0' && toupper(cp[1]) == 'X')
			cp += 2;
	}
	while (isxdigit(*cp) &&
	       (value = isdigit(*cp) ? *cp-'0' : toupper(*cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}


/*--------------------------------------------------------------
* sys_vprintf 
---------------------------------------------------------------*/
int sys_vprintf(const char *fmt, va_list ap)
{
    char buf[sizeof(long long)*8];
    char c, sign, *cp=buf;
    int left_prec, right_prec, zero_fill, pad, pad_on_right, 
        i, islong, islonglong;
    long long val = 0;
    int res = 0, length = 0;

    if (!sys_check_string(fmt)) {
        uart_puts("<Bad format string>\n");
        return 0;
    }
    while ((c = *fmt++) != '\0') {
        if (c == '%') {
            c = *fmt++;
            left_prec = right_prec = pad_on_right = islong = islonglong = 0;
            if (c == '-') {
                c = *fmt++;
                pad_on_right++;
            }
            if (c == '0') {
                zero_fill = TRUE;
                c = *fmt++;
            } else {
                zero_fill = FALSE;
            }
            while (is_digit(c)) {
                left_prec = (left_prec * 10) + (c - '0');
                c = *fmt++;
            }
            if (c == '.') {
                c = *fmt++;
                zero_fill++;
                while (is_digit(c)) {
                    right_prec = (right_prec * 10) + (c - '0');
                    c = *fmt++;
                }
            } else {
                right_prec = left_prec;
            }
            sign = '\0';
            if (c == 'l') {
                // 'long' qualifier
                c = *fmt++;
		islong = 1;
                if (c == 'l') {
                    // long long qualifier
                    c = *fmt++;
                    islonglong = 1;
                }
            }
            // Fetch value [numeric descriptors only]
            switch (c) {
            case 'p':
		islong = 1;
            case 'd':
            case 'D':
            case 'x':
            case 'X':
            case 'u':
            case 'U':
            case 'b':
            case 'B':
                if (islonglong) {
                    val = va_arg(ap, long long);
	        } else if (islong) {
                    val = (long long)va_arg(ap, long);
		} else{
                    val = (long long)va_arg(ap, int);
                }
                if ((c == 'd') || (c == 'D')) {
                    if (val < 0) {
                        sign = '-';
                        val = -val;
                    }
                } else {
                    // Mask to unsigned, sized quantity
                    if (islong) {
                        val &= ((long long)1 << (sizeof(long) * 8)) - 1;
                    } else{
                        val &= ((long long)1 << (sizeof(int) * 8)) - 1;
                    }
                }
                break;
            default:
                break;
            }
            // Process output
            switch (c) {
            case 'p':  // Pointer
                printf_putc('0');
                printf_putc('x');
                zero_fill = TRUE;
                left_prec = sizeof(unsigned long)*2;
            case 'd':
            case 'D':
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                switch (c) {
                case 'd':
                case 'D':
                case 'u':
                case 'U':
                    length = _cvt(val, buf, 10, "0123456789");
                    break;
                case 'p':
                case 'x':
                    length = _cvt(val, buf, 16, "0123456789abcdef");
                    break;
                case 'X':
                    length = _cvt(val, buf, 16, "0123456789ABCDEF");
                    break;
                }
                cp = buf;
                break;
            case 's':
            case 'S':
                cp = va_arg(ap, char *);
                if (cp == NULL) 
                    cp = "<null>";
                else if (!sys_check_string(cp)) {
                    uart_puts("<Not a string: 0x");
                    sys_write_hex((UINT32)cp);
                    cp = ">";
                }
                length = 0;
                while (cp[length] != '\0') length++;
                break;
            case 'c':
            case 'C':
                c = va_arg(ap, int /*char*/);
                printf_putc(c);
                res++;
                continue;
            case 'b':
            case 'B':
                length = left_prec;
                if (left_prec == 0) {
                    if (islonglong)
                        length = sizeof(long long)*8;
                    else if (islong)
                        length = sizeof(long)*8;
                    else
                        length = sizeof(int)*8;
                }
                for (i = 0;  i < length-1;  i++) {
                    buf[i] = ((val & ((long long)1<<i)) ? '1' : '.');
                }
                cp = buf;
                break;
            case '%':
                printf_putc('%');
                break;
            default:
                printf_putc('%');
                printf_putc(c);
                res += 2;
            }
            pad = left_prec - length;
            if (sign != '\0') {
                pad--;
            }
            if (zero_fill) {
                c = '0';
                if (sign != '\0') {
                    printf_putc(sign);
                    res++;
                    sign = '\0';
                }
            } else {
                c = ' ';
            }
            if (!pad_on_right) {
                while (pad-- > 0) {
                    printf_putc(c);
                    res++;
                }
            }
            if (sign != '\0') {
                printf_putc(sign);
                res++;
            }
            while (length-- > 0) {
                c = *cp++;
                if (c == '\n') {
                    printf_putc('\r');
                    res++;
                }
                printf_putc(c);
                res++;
            }
            if (pad_on_right) {
                while (pad-- > 0) {
                    printf_putc(' ');
                    res++;
                }
            }
        } else {
            if (c == '\n') {
                printf_putc('\r');
                res++;
            }
            printf_putc(c);
            res++;
        }
    }
    return (res);
}

/**
 * vsscanf - Unformat a buffer into a list of arguments
 * @buf:	input buffer
 * @fmt:	format of buffer
 * @args:	arguments
 */
 
int sys_vsscanf(unsigned char *buf, char *fmt, va_list args)
{
	unsigned char *str = buf;
	char *next;
	char digit;
	int num = 0;
	int qualifier;
	int base;
	int field_width;
	int is_sign = 0;

	while((*fmt != '\0') && *str) {
//			while(*fmt && *str) {
		/* skip any white space in format */
		/* white space in format matchs any amount of
		 * white space, including none, in the input.
		 */
		if (isspace(*fmt)) {
			while (isspace(*fmt))
				++fmt;
			while (isspace(*str))
				++str;
		}

		/* anything that is not a conversion must match exactly */
		if (*fmt != '%' && *fmt) {
			if (*fmt++ != *str++)
				break;
			continue;
		}

		if (!*fmt)
			break;
		++fmt;
		
		/* skip this conversion.
		 * advance both strings to next white space
		 */
		if (*fmt == '*') {
			while (!isspace(*fmt) && *fmt)
				fmt++;
			while (!isspace(*str) && *str)
				str++;
			continue;
		}

		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);

		/* get conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' ||
		    *fmt == 'Z' || *fmt == 'z') {
			qualifier = *fmt++;
			if (unlikely(qualifier == *fmt)) {
				if (qualifier == 'h') {
					qualifier = 'H';
					fmt++;
				} else if (qualifier == 'l') {
					qualifier = 'L';
					fmt++;
				}
			}
		}
		base = 10;
		is_sign = 0;

		if (!*fmt || !*str)
			break;

		switch(*fmt++) {
		case 'c':
		{
			char *s = (char *) va_arg(args,char*);
			if (field_width == -1)
				field_width = 1;
			do {
				*s++ = *str++;
			} while (--field_width > 0 && *str);
			num++;
		}
		continue;
		case 's':
		{
			char *s = (char *) va_arg(args, char *);
			if(field_width == -1)
				field_width = INT_MAX;
			/* first, skip leading white space in buffer */
			while (isspace(*str))
				str++;

			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width--) {
				*s++ = *str++;
			}
			*s = '\0';
			num++;
		}
		continue;
		case '[':
		{
			char *s = (char *) va_arg(args, char *);
			char tmp1,tmp2;
			if(field_width == -1)
				field_width = INT_MAX;
			/* first, skip leading white space in buffer */
			while (isspace(*str))
				str++;
			while ((tmp2==']') && field_width--) {
				tmp1 = tmp2;
				tmp2 = *fmt++;
			}
			/* now copy until next white space */
			while (*str && !isspace(*str) && field_width-- && !(tmp1==*str) && !(*str=='\r')) {
				*s++ = *str++;
			}
			*s = '\0';
			num++;
			field_width = INT_MAX;
			while (!(*fmt==']') && field_width--) {
					fmt++;
				}
			fmt++;
		}
		continue;
		case 'n':
			/* return number of characters read so far */
		{
			int *i = (int *)va_arg(args,int*);
			*i = str - buf;
		}
		continue;
		case 'o':
			base = 8;
			break;
		case 'x':
		case 'X':
			base = 16;
			break;
		case 'i':
                        base = 0;
		case 'd':
			is_sign = 1;
		case 'u':
			break;
		case '%':
			/* looking for '%' in str */
			if (*str++ != '%') 
				return num;
			continue;
		default:
			/* invalid format; stop here */
			return num;
		}

		/* have some sort of integer conversion.
		 * first, skip white space in buffer.
		 */
		while (isspace(*str))
			str++;

		digit = *str;
		if (is_sign && digit == '-')
			digit = *(str + 1);

		if (!digit
                    || (base == 16 && !isxdigit(digit))
                    || (base == 10 && !isdigit(digit))
                    || (base == 8 && (!isdigit(digit) || digit > '7'))
                    || (base == 0 && !isdigit(digit)))
				break;

		switch(qualifier) {
		case 'H':	/* that's 'hh' in format */
			if (is_sign) {
				signed char *s = (signed char *) va_arg(args,signed char *);
				*s = (signed char) simple_strtol(str,&next,base);
			} else {
				unsigned char *s = (unsigned char *) va_arg(args, unsigned char *);
				*s = (unsigned char) simple_strtoul(str, &next, base);
			}
			break;
		case 'h':
			if (is_sign) {
				short *s = (short *) va_arg(args,short *);
				*s = (short) simple_strtol(str,&next,base);
			} else {
				unsigned short *s = (unsigned short *) va_arg(args, unsigned short *);
				*s = (unsigned short) simple_strtoul(str, &next, base);
			}
			break;
		case 'l':
			if (is_sign) {
				long *l = (long *) va_arg(args,long *);
				*l = simple_strtol(str,&next,base);
			} else {
				unsigned long *l = (unsigned long*) va_arg(args,unsigned long*);
				*l = simple_strtoul(str,&next,base);
			}
			break;
		case 'L':
			if (is_sign) {
				long long *l = (long long*) va_arg(args,long long *);
				*l = simple_strtoll(str,&next,base);
			} else {
				unsigned long long *l = (unsigned long long*) va_arg(args,unsigned long long*);
				*l = simple_strtoull(str,&next,base);
			}
			break;
		case 'Z':
		case 'z':
		{
			size_t *s = (size_t*) va_arg(args,size_t*);
			*s = (size_t) simple_strtoul(str,&next,base);
		}
		break;
		default:
			if (is_sign) {
				int *i = (int *) va_arg(args, int*);
				*i = (int) simple_strtol(str,&next,base);
			} else {
				unsigned int *i = (unsigned int*) va_arg(args, unsigned int*);
				*i = (unsigned int) simple_strtoul(str,&next,base);
			}
			break;
		}
		num++;

		if (!next)
			break;
		str = next;
	}
	return num;
}
