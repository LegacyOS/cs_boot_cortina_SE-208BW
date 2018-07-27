/****************************************************************************
* Copyright  Storlink Corp 2005.  All rights reserved.                
*--------------------------------------------------------------------------
* Name			: sys_malloc.c
* Description	: 
*		Handle simple dynamic memory allocation
*		First fit first allocate
*
* History
*
*	Date		Writer		Description
*	-----------	-----------	-------------------------------------------------
*	04/20/2005	Gary Chen	Create
*
****************************************************************************/
#include <define.h>

#define SYS_MEM_MIN_SIZE	(256 * 1024)
#define SYS_MEM_TAG			(('S' << 0) + ('t' << 8) + ('o' << 16) + ('r' << 24))
#define SYS_MEM_END_PTR		((void *)-1)

typedef struct {
	UINT32 	tag;		// for protection
	char    *next;
	char    *prev;
	UINT32	size;
	UINT32	used_cnt;	// 0: free, else using count
	UINT32	pad[3];		// for alignment 16
} SYS_MEM_BLK_T;

typedef struct {
	char 	*start_ptr;
	char	*end_ptr;
	UINT32	total_size;
	UINT32	max_alloc_size;	// max size to be allocated
	char	*max_alloc_ptr;
} SYS_MEM_INFO_T;

SYS_MEM_INFO_T sys_mem_info;

static void sys_mem_garbage_collect(SYS_MEM_BLK_T *blk);
void dump_malloc_list(void);

// #define _DEBUG_MALLOC
#ifdef _DEBUG_MALLOC
	void test_malloc(void);
	#define _DUMP_MALLOC_LIST()		dump_malloc_list()
#else
	#define _DUMP_MALLOC_LIST()
#endif

/*--------------------------------------------------------------
* 	sys_init_memory
---------------------------------------------------------------*/
int sys_init_memory(char *startp, int size)
{
	SYS_MEM_INFO_T *mem_info;
	SYS_MEM_BLK_T  *blk;
	
	mem_info = (SYS_MEM_INFO_T *)&sys_mem_info;
	
	if (size < SYS_MEM_MIN_SIZE)
	{
		dbg_printf(("Too smalle size for memory allocation area\n"));
		return;
	}
	mem_info->start_ptr = startp;
	mem_info->end_ptr   = startp + size - 1;
	mem_info->total_size = size;
	mem_info->max_alloc_size = size - sizeof(SYS_MEM_BLK_T);
	mem_info->max_alloc_ptr = startp;
	
	// init first block
	blk = (SYS_MEM_BLK_T *)startp;
	blk->tag = SYS_MEM_TAG;
	blk->next = SYS_MEM_END_PTR;
	blk->prev = SYS_MEM_END_PTR;
	blk->size = size - sizeof(SYS_MEM_BLK_T);
	blk->used_cnt = 0;
	
	_DUMP_MALLOC_LIST();
}

/*--------------------------------------------------------------
* 	sys_malloc
---------------------------------------------------------------*/
void *sys_malloc(int size)
{
	SYS_MEM_INFO_T *mem_info;
	SYS_MEM_BLK_T  *blk;
	
	size = (size + 15) & ~15;
	
	mem_info = (SYS_MEM_INFO_T *)&sys_mem_info;
	blk = (SYS_MEM_BLK_T *)mem_info->start_ptr;
	while (blk != SYS_MEM_END_PTR)
	{
		if (blk->tag != SYS_MEM_TAG)
		{
			dbg_printf(("Memory corrupt!\n"));
			return NULL;
		}
		if (!blk->used_cnt && (blk->size >= size))
		{
			// split if enough memory space
			if ((blk->size - size) > (2 * sizeof(SYS_MEM_BLK_T)))
			{
				SYS_MEM_BLK_T *new_blk;
				char *new_p;
				new_p = (char *)blk + size + sizeof(SYS_MEM_BLK_T);
				new_blk = (SYS_MEM_BLK_T *)new_p;
				new_blk->tag = SYS_MEM_TAG;
				new_blk->next = blk->next;
				new_blk->prev = (char *)blk;
				new_blk->size = blk->size - size - sizeof(SYS_MEM_BLK_T);
				new_blk->used_cnt = 0;
				sys_mem_garbage_collect(new_blk);
				
				blk->next = (char *)new_blk;
			}
			
			blk->used_cnt++;
			blk->size = size;
			_DUMP_MALLOC_LIST();
			return (void *)((char *)blk + sizeof(SYS_MEM_BLK_T));
		}
		blk = (SYS_MEM_BLK_T *)blk->next;
	}
	
	_DUMP_MALLOC_LIST();
	return NULL;
}

/*--------------------------------------------------------------
* 	sys_calloc
---------------------------------------------------------------*/
void *sys_calloc(int size)
{
	char *datap;
	
	if ((datap = (char*)sys_malloc))
		memset(datap, 0, size);
}

/*--------------------------------------------------------------
* 	sys_free
---------------------------------------------------------------*/
void sys_free(void *datap)
{
	char *destp;
	SYS_MEM_BLK_T *blk, *prev;
	
	if (!datap)
		return;
		
	destp = datap;
	
	blk = (SYS_MEM_BLK_T *)(destp - sizeof(SYS_MEM_BLK_T));
	
	if (blk->tag != SYS_MEM_TAG)
	{
		dbg_printf(("Memory corrupt!\n"));
		_DUMP_MALLOC_LIST();
		return;
	}
	blk->used_cnt = 0;
	prev = (SYS_MEM_BLK_T *)blk->prev;
	
	if (prev!= SYS_MEM_END_PTR && prev->used_cnt == 0)
		sys_mem_garbage_collect((SYS_MEM_BLK_T *)prev);
	else
		sys_mem_garbage_collect((SYS_MEM_BLK_T *)blk);
	_DUMP_MALLOC_LIST();
}

/*--------------------------------------------------------------
* 	sys_mem_garbage_collect
---------------------------------------------------------------*/
static void sys_mem_garbage_collect(SYS_MEM_BLK_T *start_blk)
{
	SYS_MEM_BLK_T *blk;
	
	if (start_blk->used_cnt)
	{
		dbg_printf(("Illegal caller!\n"));
		return;
	}
		
	blk = (SYS_MEM_BLK_T *)start_blk->next;
	while (blk != SYS_MEM_END_PTR)
	{
		if (blk->tag != SYS_MEM_TAG)
		{
			dbg_printf(("Memory corrupt!\n"));
			return;
		}
		if (!blk->used_cnt)
		{
			start_blk->size += blk->size + sizeof (SYS_MEM_BLK_T);
			start_blk->next = blk->next;
			blk->tag = 0;
			blk = (SYS_MEM_BLK_T *)blk->next;
			if (blk != SYS_MEM_END_PTR)
				blk->prev = start_blk;
		}
		else
		{
			break;
		}
	}
}

/*--------------------------------------------------------------
* 	dump_malloc_list
---------------------------------------------------------------*/
void dump_malloc_list(void)
{
	SYS_MEM_INFO_T *mem_info;
	SYS_MEM_BLK_T  *blk;
	int i=1;
	
	mem_info = (SYS_MEM_INFO_T *)&sys_mem_info;
	blk = (SYS_MEM_BLK_T *)mem_info->start_ptr;
	printf("id   Location    Size        Used\n");              
	while (blk != SYS_MEM_END_PTR)
	{
		if (blk->tag != SYS_MEM_TAG)
		{
			dbg_printf(("Memory corrupt!\n"));
			return;
		}
		printf("%-3d  0x%08x  0x%08x  %d\n",
				i++, (void *)blk, blk->size, blk->used_cnt);
				
		blk = (SYS_MEM_BLK_T *)blk->next;
	}
}

#ifdef _DEBUG_MALLOC
/*--------------------------------------------------------------
* 	test_malloc
---------------------------------------------------------------*/
void test_malloc(void)
{
	char *buf1, *buf2, *buf3;
		
	buf1 = (char *)malloc(1024);
	printf("buf1=0x%x\n", buf1);
		
	buf2 = (char *)malloc(511);
	printf("buf2=0x%x\n", buf2);
		
	buf3 = (char *)malloc(1 * 1024 * 1024);
	printf("buf2=0x%x\n", buf3);
		
	free(buf2);
	printf("free(0x%x)\n", buf2);
	
	free(buf3);
	printf("free(0x%x)\n", buf3);
	
	free(buf1);
	printf("free(0x%x)\n", buf3);
}
#endif	// _DEBUG_MALLOC



