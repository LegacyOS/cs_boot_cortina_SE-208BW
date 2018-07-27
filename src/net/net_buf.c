/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: net_buf.c
* Description	: 
*		Handle network buffer
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/25/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>
#include <net.h>

static unsigned char netbuf[NET_BUF_NUM][NETBUF_SIZE];
NETBUF_QUEUE_T net_free_q;
NETBUF_QUEUE_T net_rcv_q;

static void net_insert_queue_first(NETBUF_QUEUE_T *queue, NETBUF_HDR_T *buf);
static void net_insert_queue_tail(NETBUF_QUEUE_T *queue, NETBUF_HDR_T *buf);
void net_show_buf_info(void);

/*----------------------------------------------------------------------
* net_init_buf
*----------------------------------------------------------------------*/
void net_init_buf(void)
{
	int i;
	NETBUF_HDR_T *hdr;
	
	// initialize net buffer queues
	memset((char *)&net_free_q, 0, sizeof(NETBUF_QUEUE_T));
	memset((char *)&net_rcv_q, 0, sizeof(NETBUF_QUEUE_T));
	
	for (i=0; i<NET_BUF_NUM; i++)
	{
		hdr = (NETBUF_HDR_T *)&netbuf[i];
		hdr->tag = NET_BUF_TAG;
		hdr->datap = (char *)hdr + NETBUF_HDR_SIZE;
		net_insert_queue_tail((NETBUF_QUEUE_T *)&net_free_q, (NETBUF_HDR_T *)hdr);
	}
}

/*----------------------------------------------------------------------
* net_insert_queue_first
*----------------------------------------------------------------------*/
static void net_insert_queue_first(NETBUF_QUEUE_T *queue, NETBUF_HDR_T *buf)
{
	if (!queue->first)
		queue->tail = buf;
		
	buf->next_buf = queue->first;
	queue->first  = buf;
	
	queue->total++;
}

/*----------------------------------------------------------------------
* net_insert_queue_tail
*----------------------------------------------------------------------*/
static void net_insert_queue_tail(NETBUF_QUEUE_T *queue, NETBUF_HDR_T *buf)
{
	if (queue->tail)
		queue->tail->next_buf = buf;
	else
		queue->first = buf;
		
	buf->next_buf = NULL;
	queue->tail = buf;
	queue->total++;
}

/*----------------------------------------------------------------------
* net_remove_queue_first
*----------------------------------------------------------------------*/
static NETBUF_HDR_T *net_remove_queue_first(NETBUF_QUEUE_T *queue)
{
	NETBUF_HDR_T *buf;
	
	if (queue->first)
	{
		buf = queue->first;
		queue->first = buf->next_buf;
		if (!queue->first)
			queue->tail = NULL;
		queue->total--;
		return buf;
	}
	else
		return NULL;
}

/*----------------------------------------------------------------------
* net_alloc_buf
*----------------------------------------------------------------------*/
NETBUF_HDR_T *net_alloc_buf(void)
{
	NETBUF_HDR_T *bufp;
	
	bufp = (NETBUF_HDR_T *)net_remove_queue_first(&net_free_q);
	
	// net_show_buf_info();
	
	return (NETBUF_HDR_T *)bufp;
}

/*----------------------------------------------------------------------
* net_free_buf
*----------------------------------------------------------------------*/
void net_free_buf(NETBUF_HDR_T *hdr)
{
	if (hdr->tag != NET_BUF_TAG)
	{
		dbg_printf(("Illegal Net Buffer header!\n"));
		return;
	}

	// net_show_buf_info();
	
	net_insert_queue_tail((NETBUF_QUEUE_T *)&net_free_q, (NETBUF_HDR_T *)hdr);
}

/*----------------------------------------------------------------------
* net_put_rcvq
*----------------------------------------------------------------------*/
void net_put_rcvq(NETBUF_HDR_T *hdr)
{
	if (hdr->tag != NET_BUF_TAG)
	{
		dbg_printf(("Illegal Net Buffer header!\n"));
		return;
	}
	
	net_insert_queue_tail((NETBUF_QUEUE_T *)&net_rcv_q, (NETBUF_HDR_T *)hdr);
}

/*----------------------------------------------------------------------
* net_get_rcvq
*----------------------------------------------------------------------*/
NETBUF_HDR_T *net_get_rcvq(void)
{
	NETBUF_HDR_T *bufp;
	
	bufp = (NETBUF_HDR_T *)net_remove_queue_first(&net_rcv_q);
	
	return (NETBUF_HDR_T *)bufp;
}

/*----------------------------------------------------------------------
* net_peek_rcvq
*----------------------------------------------------------------------*/
int net_peek_rcvq(void)
{
	return net_rcv_q.total;
}

/*----------------------------------------------------------------------
* net_show_buf_info
*----------------------------------------------------------------------*/
void net_show_buf_info(void)
{
	NETBUF_HDR_T 	*hdr;
	int 			total;
	int 			i;
	
	printf("Free net buffer  : %d\n", net_free_q.total);
	printf("Net Receive queue: %d\n", net_rcv_q.total);
	
	total = 0;
	hdr = (NETBUF_HDR_T *)net_free_q.first;
	for (i=0; i<net_free_q.total; i++)
	{
		if (hdr->tag != NET_BUF_TAG)
		{
			dbg_printf(("Illegal Net Buffer header of net_free_q!\n"));
			return;
		}
		total++;
		hdr = hdr->next_buf;
	}
	if (total != net_free_q.total)
		dbg_printf(("corrupt for net_free_q\n"));
		
	total = 0;
	hdr = (NETBUF_HDR_T *)net_rcv_q.first;
	for (i=0; i<net_rcv_q.total; i++)
	{
		if (hdr->tag != NET_BUF_TAG)
		{
			dbg_printf(("Illegal Net Buffer header of net_rcv_q!\n"));
			return;
		}
		total++;
		hdr = hdr->next_buf;
	}
	if (total != net_rcv_q.total)
		dbg_printf(("corrupt for net_rcv_q\n"));
		
}

