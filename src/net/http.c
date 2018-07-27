// Copyright (C) 2002 Mason Kidd (mrkidd@nettaxi.com)
//
// This file is part of MicroWeb.
//
// MicroWeb is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// MicroWeb is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MicroWeb; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

// http.c: HTTP protocol processing

#include <define.h>
#include <net.h>
#include <sys_fis.h>
#include "chksum.h"

#ifdef BOARD_SUPPORT_WEB

void http_response(TCP_SOCK_T *sock);
void http_upgrade(TCP_SOCK_T *sock, int type, int got_size);
void http_reboot(TCP_SOCK_T *sock);
void http_error(int nStatus, char *szTitle, char *szText,TCP_SOCK_T *sock);
void http_header(int nStatus, char *szTitle, int nLength);
int image_upgrade(TCP_SOCK_T *sock);
extern int web_upgrade(int type, int got_size, unsigned char *filep);

http_TCB http_t;

unsigned char c_len[] = "Content-Length: ";
unsigned char c_type[] = "Content-Type: ";
unsigned char c_host[] = "Host: ";
unsigned char c_headerend[] = "\r\n\r\n";
unsigned char c_post[] = "POST";
unsigned char c_post1[] = "ugrade.html";
unsigned char c_boundary[] = "boundary=";
unsigned char c_fname[] = "filename=";
unsigned char c_eb1[] = "\r\n--";
unsigned char c_eb2[] = "--\r\n";
unsigned char c_mark[] = "--\r\n";
unsigned char c_slboot[]="cs-boot.bin";
unsigned char c_kernel[]="zImage";
unsigned char c_rd[]="rd.gz";
unsigned char c_ap[]="hddapp.bz2";
unsigned char c_flashimage[]="flashimage.bin";
unsigned char c_kernel2[]="zImage2";
unsigned char c_rd2[]="rd2.gz";

unsigned int pszBuf = 0;
unsigned int phtmlBuf = 0;
unsigned char cTCB = 0;
//unsigned char boundary[100];
unsigned char file_name[100];

#define SZBUF 800
unsigned char szBuf[SZBUF];
unsigned char htmlBuf[SZBUF];

char head[]=  "HTTP/1.0 200 OK\n\r" \
               "Server: http v0.1\n\r" \
               "Content-type: ";

char txt400[]="<html><head><title>400 Bad Request</title></head>\n\r" \
               "<body><h1>400 Bad Request.</h1>\n\r" \
               "Your client sent an invalid request\n\r";

char txt404[]="<html><head><title>404 Not Found.</title></head>\n\r" \
               "<body><h1>404 Not Found.</h1>\n\r" \
               "<hr><em><b>Zhttpd v0.1 &copy;1999 D Morris</b></em>\n\r" \
               "</body></html>\n\r";


char upgrade200[]="<script language=\"javascript\"> var count=" ;

char upgrade200_1[]="; function clock() {	var min = Math.floor(count / 60); 	var sec = count - min*60 ;"\
									"	document.form.mm.value= min; 	document.form.ss.value= sec;"\
									"	count--; 	if (count) 		setTimeout(\"clock()\",1000); "\
									"else setTimeout(\"location='/reboot.html'\",1000);  }"\
									"</script>"\
									"<noscript>"\
									"<meta http-equiv=\"Refresh\" content=\"5;URL=/reboot.html\" />"\
									"</noscript>"\
									"<body onLoad=\"clock();\">"\
									"	<center><font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>"\
									"<form name=\"form\" method=\"post\" action="">"\
									"Please wait  <input type=\"text\" name=\"mm\" size=\"2\"> Min &nbsp;<input type=\"text\" name=\"ss\" size=\"2\"> Sec. &nbsp;"\
									"</Form>	"\
									"<hr><ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>"\
									"<font face=Verdana size=1><li>If you upload the binary file to the wrong TARGET, "\
									"the router may not work properly or even could not boot-up again.</li><hr></font></body></html>" ;


//char upgrade200_boot[]="30" ;
//char upgrade200_zImage[]="70" ;
//char upgrade200_rd1[]="175" ;
//char upgrade200_rd2[]="120" ;
//char upgrade200_zImage2[]="140" ;
//char upgrade200_hddapp[]="140" ;
//char upgrade200_flashimage[]="460" ;

char txt200[]="<body><form method=\"post\" action=\"/upgrade.html\" enctype=\"multipart/form-data\"><center>\n\r" \
			"<font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>" \
			"<p><table cellPadding=0 cellSpacing=10><tr><td align=right>File Path</td>" \
			"<td>&nbsp;&nbsp;<input type=file name=files value=""></td></tr><tr><td>&nbsp;</td>" \
			"<td><input type=\"submit\" value=\"Send\"></td></tr></table></form><hr>" \
			"<ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>" \
			"<font face=Verdana size=1><li>If you upload the binary file to the wrong TARGET, " \
			"the router may not work properly or even could not boot-up again.</li><hr></font></body></html>";



char reboot200[]=	"<body> <form> <center>"\
									"<font color=blue face=verdana size=3><b>Firmware Upgrade</b></font>"\
									"<p><p>"\
									"<font color=red face=verdana size=5><b><marquee width=\"400\" height=\"20\" behavior=alternate>Reboot System Now .....</marquee></b></font>"\
									"<p> <p> </form> "\
									"<hr><ul><font face=Verdana color=red size=4>NOTICE !!</font></ul><br></center>"\
									"<font face=Verdana size=1><li>If you upload the binary file to the wrong TARGET, "\
									"the router may not work properly or even could not boot-up again.</li><hr></font></body></html>";
																											
																											
void tx_http_packet(unsigned char *szData, unsigned int nLength, TCP_SOCK_T *sock)
{
					if (tcp_sendto(sock, szData, nLength, sock->from_ip, sock->local_port, sock->remote_port) < 0)
  	  				{
								//	tcp_close(sock);
  	  		    			return ;//TCP_ERROR_SENDTO;
  	  				}	
}

void rx_http_packet(unsigned char *szData, unsigned int nLength, TCP_SOCK_T *sock)
{
	unsigned char method[5], path[100], protocol[10], llen[10], btp[100];
	unsigned char *file,*curr_ptr ,*cptr, *startp;
	int nFileLength = 0,len,tmp;
	UINT64	delay_time;
	UINT32	delay_ticks;

	pszBuf = 0;
	phtmlBuf = 0;
	memset(szBuf, 0 , SZBUF);
	memset(htmlBuf, 0 , SZBUF);
	if((sock->new_seq==sock->old_seq)&&(sock->post_data == 1))
	{
		if((http_t.tol_len == 0) || (strlen(sock->boundary)==0) || (strncmp(http_t.post_buf+2,sock->boundary,strlen(sock->boundary))))
		{
			memcpy((http_t.post_buf+http_t.curr_len), szData, nLength);
			http_t.curr_len += nLength;
						
			if(http_t.tol_len==0)
			{
					cptr = http_t.post_buf;
					curr_ptr = strstr(cptr,c_len);
					if(!curr_ptr)
					{
						sock->old_seq = sock->new_seq + nLength;
							sock->flag |= TCP_CNTRL_ACK;
							tx_http_packet(szBuf, 0, sock);
						return;
					}
					curr_ptr += strlen(c_len);
					if (sscanf(curr_ptr, "%[^\n]", llen) != 1)
					{
						//printf("\nPost length error in second packet.\n");
						;
					}
					http_t.tol_len = str2decimal(llen);
			}

			if(strlen(sock->boundary)==0)
			{
					cptr = http_t.post_buf;
					curr_ptr = strstr(cptr,c_boundary);
					if(!curr_ptr)
					{
						sock->old_seq = sock->new_seq + nLength;
							sock->flag |= TCP_CNTRL_ACK;
							tx_http_packet(szBuf, 0, sock);
						return;
					}
					if (sscanf((curr_ptr+9), "%[^\r\n]", btp) != 1)
					{
					//	printf(" no boundary data in second packet\n");
						;
					}
	      	
					memcpy(sock->boundary, btp, strlen(btp));

			}
			
			cptr = http_t.post_buf;
			cptr = strstr(cptr,c_boundary);
			curr_ptr = strstr((cptr+9+strlen(sock->boundary)),sock->boundary);
			if(!curr_ptr)
			{
				sock->old_seq = sock->new_seq + nLength;
				sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, 0, sock);
				return;
			}
			curr_ptr -=2; //add '--' before start boundary,but add '--'boundary'--'
			tmp = http_t.curr_len - (curr_ptr - http_t.post_buf);

			http_t.curr_len = tmp; //strlen(curr_ptr);
			memcpy(http_t.post_buf, curr_ptr, http_t.curr_len);
		}
		else
		{
			memcpy((http_t.post_buf+http_t.curr_len), szData, nLength);
			http_t.curr_len += nLength;
		}
		
		sock->old_seq += nLength;
		sock->flag = TCP_CNTRL_ACK ;
		//curr_ptr = strstr((szData+nLength-80), sock->boundary);
		curr_ptr = (szData+nLength-(strlen(sock->boundary)+8));
			if(!(strstr(curr_ptr, sock->boundary))&&(http_t.curr_len < http_t.tol_len))
		{
				len = 0;
				sock->flag |= TCP_CNTRL_ACK;
				tx_http_packet(szBuf, len, sock);
		}
		else if((strstr(curr_ptr, sock->boundary))&&(http_t.curr_len == http_t.tol_len))
		{
			//printf("http_t.tol_len :(%x) http_t.curr_len:(%x) ",http_t.tol_len,http_t.curr_len);
			image_upgrade(sock);
				//http_reboot(sock);
			memset(http_t.post_buf, 0 , (BOARD_FLASH_SIZE+1024));
			memset(sock->boundary, 0 , 100);
				http_t.tol_len = 0;
				http_t.curr_len = 0;
				
		}
		else 
		{
			//if((http_t.curr_len > http_t.tol_len))
					printf("WOW unknow case: http_t.tol_len :(%x) http_t.curr_len:(%x) ",http_t.tol_len,http_t.curr_len);
					image_upgrade(sock);
					//http_reboot(sock);
				memset(http_t.post_buf, 0 , (BOARD_FLASH_SIZE+1024));
				memset(sock->boundary, 0 , 100);
				http_t.tol_len = 0;
				http_t.curr_len = 0;
			return;
		}
	}
	else if((sock->post_data == 0)&&(sock->new_seq>sock->old_seq))
	{							
		
		if (sscanf(szData, "%[^ ] %[^ ] %[^\r]", method, path, protocol) != 3)
		{
			http_error(400, "Bad Request", "Unable to prarse request.",sock);
			return;
		}
		if ((strncasecmp(method, "get") != 0) && (strncasecmp(method, "post") != 0))
		{
			http_error(501, "Not Implemented", "Method is not implemented.",sock);
			return;
		}
		if (strncasecmp(method, "post") == 0)
		{
			if(!http_t.post_buf)
			{
				http_t.post_buf = (char *)malloc(BOARD_FLASH_SIZE+1024);
				if (!http_t.post_buf)
				{
					printf(("http_t.post_buf : No free memory!\n"));
					return;
				}
			}
			memset(http_t.post_buf, 0 , (BOARD_FLASH_SIZE+1024));
			
			http_t.tol_len = 0;
			http_t.curr_len = 0;
			sock->post_data = 1;
			//check boundary
			memset(btp , 0 , 100);
			
			cptr = szData;
			curr_ptr = strstr(cptr,c_boundary);
			if(!curr_ptr)
			{
				http_t.curr_len = nLength; //strlen(curr_ptr);
				memcpy(http_t.post_buf, szData, http_t.curr_len);
				//printf(" no boundary data found in first packet.\n");
				sock->old_seq = sock->new_seq + nLength;
					sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, 0, sock);
				return;
			}

			startp = curr_ptr+9;
						
			//is packet include "Content-Length:" 
			cptr = szData;
			curr_ptr = strstr(cptr,c_len);
			if(!curr_ptr)
			{
				http_t.curr_len = nLength; //strlen(curr_ptr);
				memcpy(http_t.post_buf, szData, http_t.curr_len);
				sock->old_seq = sock->new_seq + nLength;
					sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, 0, sock);
				return;
			}
									
			cptr = szData;
			curr_ptr = strstr(cptr,c_len);
			curr_ptr += strlen(c_len);
			if (sscanf(curr_ptr, "%[^\n]", llen) != 1)
			{
				//printf("\nPost length error in first packet.\n");
				;
			}
			http_t.tol_len = str2decimal(llen);
			
			cptr = szData;
			curr_ptr = strstr(cptr,c_boundary);
			if(!curr_ptr)
			{
				http_t.curr_len = nLength; //strlen(curr_ptr);
				memcpy(http_t.post_buf, szData, http_t.curr_len);
				sock->old_seq = sock->new_seq + nLength;
					sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, 0, sock);
				return;
			}
			
			if (sscanf((curr_ptr+9), "%[^\r\n]", btp) != 1)
			{
				//printf(" no boundary data in first packet.\n");
				;
			}
      
			memcpy(sock->boundary, btp, strlen(btp));
			
			cptr = szData;
			cptr = strstr(cptr,c_host);
			curr_ptr = strstr(cptr,sock->boundary);
			if(!curr_ptr)
			{
				http_t.curr_len = nLength; //strlen(curr_ptr);
				memcpy(http_t.post_buf, szData, http_t.curr_len);				
				sock->old_seq = sock->new_seq + nLength;
					sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, 0, sock);
				return;
			}
			curr_ptr -=2; //add '--' before start boundary,but add '--'boundary'--' 
			tmp = nLength - (curr_ptr - szData);

			http_t.curr_len = tmp; //strlen(curr_ptr);
			memcpy(http_t.post_buf, curr_ptr, http_t.curr_len);

			sock->old_seq = sock->new_seq + nLength;

			//curr_ptr = strstr((szData+nLength-80), sock->boundary);
			curr_ptr = (szData+nLength-(strlen(sock->boundary)+8));

			if(!(strstr(curr_ptr, sock->boundary))&&(http_t.curr_len < http_t.tol_len))
			{		
					len = 0;
					sock->flag |= TCP_CNTRL_ACK;
					tx_http_packet(szBuf, len, sock);
			}
			else if((strstr(curr_ptr, sock->boundary))&&(http_t.curr_len == http_t.tol_len))
			{
				//printf("http_t.tol_len :(%x) http_t.curr_len:(%x) ",http_t.tol_len,http_t.curr_len);
				image_upgrade(sock);
					//http_reboot(sock);
				memset(http_t.post_buf, 0 , (BOARD_FLASH_SIZE+1024));
				memset(sock->boundary, 0 , 100);
				http_t.tol_len = 0;
				http_t.curr_len = 0;
			}
			else 
			{
				//if((http_t.curr_len > http_t.tol_len))
					printf("WOW unknow case: http_t.tol_len :(%x) http_t.curr_len:(%x) ",http_t.tol_len,http_t.curr_len);
					image_upgrade(sock);
					//http_reboot(sock);
				memset(http_t.post_buf, 0 , (BOARD_FLASH_SIZE+1024));
				memset(sock->boundary, 0 , 100);
				http_t.tol_len = 0;
				http_t.curr_len = 0;
			}
			return;
		}//end post

		if (path[0] != '/')
			{
				http_error(400, "Bad Request", "Bad filename.",sock);
				return;
			}
		file = &(path[1]);
		if (strlen(file) == 0)
			{
			http_response(sock);
			}
		else if ((strncasecmp(file, "index.html") == 0) || (strncasecmp(file, "index.htm") == 0))
			{
				http_response(sock);
			}
			else if ((strncasecmp(file, "upgrade.html") == 0) || (strncasecmp(file, "upgrade.htm") == 0))
			{
				http_upgrade(sock, 2, 0);
			}
			else if ((strncasecmp(file, "reboot.html") == 0) || (strncasecmp(file, "reboot.htm") == 0))
			{
				http_reboot(sock);
				printf("\nReboot....");
				delay_ticks = (BOARD_BOOT_TIMEOUT * BOARD_TPS);
				delay_time = sys_get_ticks() + delay_ticks;
				
				while (sys_get_ticks() < delay_time)
				{
					
				}
				hal_reset();
			}
			//else if ((strncasecmp(file, "error_bad.html") == 0) || (strncasecmp(file, "error_bad.htm") == 0))
			//{
			//	http_error(400, "Bad Request", "Unable to prarse request.",sock);
			//}
			//else if ((strncasecmp(file, "error_501.html") == 0) || (strncasecmp(file, "error_501.htm") == 0))
			//{
			//		http_error(501, "Not Implemented", "That method is not implemented.",sock);
			//}
		else
			{
				http_error(404, "Not Found", "File not found.",sock);
				return;
			}
		}  	
} 	

void http_response(TCP_SOCK_T *sock)
{
	int len;
	//sock->flag |= TCP_FLAG_FIN;
	phtmlBuf += sprintf(&htmlBuf[phtmlBuf], "<html><head><title>%s</title></head>", _HTTP_SERVER);
	memcpy(&htmlBuf[phtmlBuf],txt200,sizeof(txt200));
	phtmlBuf += sizeof(txt200);
	len = strlen(htmlBuf);
	http_header(200, "Ok", len);
	memcpy(&szBuf[pszBuf],htmlBuf,phtmlBuf);
	len = strlen(szBuf);
	tx_http_packet(szBuf, len, sock);
}

void http_upgrade(TCP_SOCK_T *sock, int type, int got_size)
{
	int len,ptime;
	int image_size;
	FIS_T	*img;
	//sock->flag |= TCP_FLAG_FIN;
	//http_header(200, "Ok", -1);
	phtmlBuf += sprintf(&htmlBuf[phtmlBuf], "<html><head><title>%s</title></head>", _HTTP_SERVER);
	memcpy(&htmlBuf[phtmlBuf],upgrade200,sizeof(upgrade200));
	phtmlBuf += sizeof(upgrade200);
	phtmlBuf --;
	if(got_size==0)
		got_size = BOARD_FLASH_SIZE;
#ifndef LOAD_FROM_IDE
#ifdef BOARD_SUPPORT_FIS
	switch(type)
	{		
		case 0: //boot
					img = fis_find_image(BOARD_FIS_BOOT_NAME);
			break;
		case 1: //kernel 			
					img = fis_find_image(BOARD_FIS_KERNEL_NAME);
			break;
		case 3: //ram disk
					img = fis_find_image(BOARD_FIS_RAM_DISK_NAME);		
			break;
		case 4: //application
					img = fis_find_image(BOARD_FIS_APPS_NAME);		
			break;
#ifdef BOARD_SUPPORT_TWO_CPU				
		case 5: // Kernel #1
					img = fis_find_image(BOARD_FIS_CPU2_NAME);					
			break;
		case 6: // RAM Disk #1
					img = fis_find_image(BOARD_FIS_RAM_DISK2_NAME);					
			break;
#endif				
	}
	if (!img)	
	{		
		printf("Image is not existed!(%s, %x)\n", BOARD_FIS_VCTL_NAME, img);		
		return;	
	}
	image_size = img->file.size;
#else
	
#endif	//#ifdef BOARD_SUPPORT_FIS
		switch(type)
	{		
		case 0: //boot
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_FLASH_BOOTIMG_SIZE;
#else
					image_size = BOARD_FLASH_BOOT_SIZE;
#endif
			break;
		case 1: //kernel 	
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_KERNELIMG_SIZE;
#else		
					image_size = BOARD_KERNEL_SIZE;
#endif
			break;
		case 3: //ram disk
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_RAM_DISKIMG_SIZE;
#else				
					image_size = BOARD_RAM_DISK_SIZE;		
#endif
			break;
		case 4: //application
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_APPSIMG_SIZE;
#else		
					image_size = BOARD_APPS_SIZE;		
#endif
			break;
#ifdef BOARD_SUPPORT_TWO_CPU				
		case 5: // Kernel #1
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_CPU2IMG_SIZE;
#else		
					image_size = BOARD_CPU2_SIZE;					
#endif
			break;
		case 6: // RAM Disk #1
#ifdef BOARD_NAND_BOOT		
					image_size = BOARD_CPU2_RDIMG_SIZE;
#else		
					image_size = BOARD_CPU2_RD_SIZE;					
#endif
			break;
#endif		
		default :
		
					image_size = BOARD_FLASH_SIZE;
	}
	
#endif//#ifndef LOAD_FROM_IDE
				
	ptime = ((((image_size/BOARD_FLASH_BLOCK_SIZE)*3)/5)+1)+((((got_size/BOARD_FLASH_BLOCK_SIZE)*17)/5)+1+20);

		
		len = sprintf(&htmlBuf[phtmlBuf], "%d",ptime);
		phtmlBuf += len;
	
	memcpy(&htmlBuf[phtmlBuf],upgrade200_1,sizeof(upgrade200_1));
	phtmlBuf += sizeof(upgrade200_1);
	len = strlen(htmlBuf);
	
	http_header(200, "Ok", len);
	memcpy(&szBuf[pszBuf],htmlBuf,phtmlBuf);
	len = strlen(szBuf);
	tx_http_packet(szBuf, len, sock);
	
}

void http_reboot(TCP_SOCK_T *sock)
{

	int len;
	//sock->flag |= TCP_FLAG_FIN;
	phtmlBuf += sprintf(&htmlBuf[phtmlBuf], "<html><head><title>%s</title></head>", _HTTP_SERVER);
	memcpy(&htmlBuf[phtmlBuf],reboot200,sizeof(reboot200));
	phtmlBuf += sizeof(reboot200);
	len = strlen(htmlBuf);
	http_header(200, "Ok", len);
	memcpy(&szBuf[pszBuf],htmlBuf,phtmlBuf);
	len = strlen(szBuf);
	tx_http_packet(szBuf, len, sock);
}

void http_error(int nStatus, char *szTitle, char *szText,TCP_SOCK_T *sock)
{
		int len;
	//	sock->flag |= TCP_FLAG_FIN;
	//printf("http message : %s : %s\n",szTitle,szText);
	phtmlBuf += sprintf(&htmlBuf[phtmlBuf], "<html><head><title>%s</title></head>", szTitle);
	phtmlBuf += sprintf(&htmlBuf[phtmlBuf], "<body>%s</body></html>", szText);
	len = strlen(htmlBuf);
	http_header(nStatus, szTitle, len);
	memcpy(&szBuf[pszBuf],htmlBuf,phtmlBuf);
	tx_http_packet(szBuf, strlen(szBuf), sock);
	
}

void http_header(int nStatus, char *szTitle, int nLength)
{
	pszBuf += sprintf(&szBuf[pszBuf], "%s %d %s\r\n", _HTTP_PROTOCOL, nStatus, szTitle);
	pszBuf += sprintf(&szBuf[pszBuf], "Server: %s\r\n", _HTTP_SERVER);
	pszBuf += sprintf(&szBuf[pszBuf], "Connection: close\r\n");
	pszBuf += sprintf(&szBuf[pszBuf], "Content-Type: text/html\r\n");
	if (nLength > 0)
		pszBuf += sprintf(&szBuf[pszBuf], "Content-Length: %d\r\n", nLength);
	pszBuf += sprintf(&szBuf[pszBuf], "\r\n");
}

int image_upgrade(TCP_SOCK_T *sock)
{
	unsigned char *curr_ptr, *cptr, *filep, *ctype;
	int got_size;
	
	
			cptr = http_t.post_buf;
			//curr_ptr = strstr(cptr,c_type);
			//ctype = curr_ptr;
			//cptr = curr_ptr;
			curr_ptr = strstr(cptr,c_headerend);
			filep = curr_ptr + 4;	
			cptr = filep;
			cptr = (http_t.post_buf + http_t.tol_len) - (strlen(sock->boundary)+8);
			got_size = (cptr - filep); /* need to - '\r\n--' length=4 */
			printf("");
	cptr = http_t.post_buf;
	curr_ptr = strstr(cptr, c_fname);
	sock->post_data = 0;
			
			
			cptr = strchr(curr_ptr+11,'"');
			memcpy(file_name, curr_ptr, (cptr - curr_ptr));
			curr_ptr = strrchr(file_name,'/');
			if(!curr_ptr)
			{
				curr_ptr = strrchr(file_name,'\\');
				if(!curr_ptr)
					curr_ptr = file_name;
			}
			//if(!http_t.progp)
			//{
			//	http_t.progp = (char *)malloc(got_size);
			//	if (!http_t.progp)
			//	{
			//		printf(("http_t.progp : No free memory!\n"));
			//		http_error(400, "Bad Request", "File allocate error.",sock);
			//		return 0;
			//	}
			//}
			//memcpy(http_t.progp, filep , got_size);
			
	if (sscanf(curr_ptr, "%[^\r\n]", file_name) != 1)
	{		
		
			//printf("\nFind file name error.\n");
			//if(http_t.progp)
			//	free(http_t.progp);
			http_error(400, "Bad Request", "Filename error.",sock);
				return 0;
	}
	
	
	if(strstr(file_name, c_slboot))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 0, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update cs-boot !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(0, got_size, http_t.post_buf);
	}
	else if(strstr(file_name, c_kernel))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 1, got_size);	
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update zImage !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(1, got_size, http_t.post_buf);
	}
	else if(strstr(file_name, c_rd))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 3, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update rd.gz !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			return web_upgrade(3, got_size, http_t.post_buf);
	}
#ifdef BOARD_SUPPORT_TWO_CPU		
	else if(strstr(file_name, c_kernel2))
	{		memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 5, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update zImage2 !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(5, got_size, http_t.post_buf);
	}
	else if(strstr(file_name, c_rd2))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 6, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update rd2.gz !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(6, got_size, http_t.post_buf);
	}
#endif	
	else if(strstr(file_name, c_ap))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 4, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update hddapp.bz2 !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(4, got_size, http_t.post_buf);
	}
	else if(strstr(file_name, c_flashimage))
	{
			memcpy(http_t.post_buf, filep , got_size);
			http_upgrade(sock, 2, got_size);		
			//memcpy(http_t.post_buf,filep,got_size);
			printf("update flashimage.bin !\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			 return web_upgrade(2, got_size, http_t.post_buf);
	}
	else
	{		
			
			printf("Image not support!\n");// size(%x) addr(%x) addr1(%x)\n", got_size, filep, http_t.post_buf);
			//if(http_t.progp)
			//	free(http_t.progp);
			
			http_error(400, "Bad Request", "Bad filename.",sock);
			
			return 0;
	}
	//if(http_t.progp)
				//free(http_t.progp);
			//	return 1;
}


#endif //#ifdef BOARD_SUPPORT_WEB