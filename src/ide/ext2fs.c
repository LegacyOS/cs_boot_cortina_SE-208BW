//==========================================================================
//
//      e2fs.c
//
//      RedBoot support for second extended filesystem
//
//==========================================================================
//####ECOSGPLCOPYRIGHTBEGIN####
// -------------------------------------------
// This file is part of eCos, the Embedded Configurable Operating System.
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Red Hat, Inc.
// Copyright (C) 2003 Gary Thomas <gary@mind.be>
//
// eCos is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 or (at your option) any later version.
//
// eCos is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with eCos; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
//
// As a special exception, if other files instantiate templates or use macros
// or inline functions from this file, or you compile this file and link it
// with other works to produce a work based on this file, this file does not
// by itself cause the resulting work to be covered by the GNU General Public
// License. However the source code for this file must still be made available
// in accordance with section (3) of the GNU General Public License.
//
// This exception does not invalidate any other reasons why a work based on
// this file might be covered by the GNU General Public License.
//
// Alternative licenses for eCos may be arranged by contacting Red Hat, Inc.
// at http://sources.redhat.com/ecos/ecos-license/
// -------------------------------------------
//####ECOSGPLCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    msalter
// Contributors: msalter
// Date:         2001-07-14
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================
#include <define.h>
#include <api.h>
#include <board_config.h>
#include <sl2312.h>

#ifdef BOARD_SUPPORT_IDE
#include "ide.h"
#include "ext2fs.h"

#define MAX_FILES			2
#define ext2fs_printf

// only one open file every time
FILE_T 				file_info[MAX_FILES];
static UINT32		blockbuf[EXT2_MAX_BLOCK_SIZE/sizeof(UINT32)];
static EXT2_DESC_T	ext2fs_desc;
FILE_T				*kernel_fp;
FILE_T				*initrd_fp;

FILE_T *ext2fs_open(char *filepath);
static int ext2fs_mount(IDE_PART_T *part, EXT2_DESC_T *desc);
static int ext2fs_get_gdesc(EXT2_DESC_T *ext2fs, UINT32 group_nr, EXT2_GROUP_T *gdesc);
static int ext2fs_get_inode(EXT2_DESC_T *ext2fs, int ino, EXT2_INODE_T *ip);
static int ext2fs_inode_block(EXT2_DESC_T *ext2fs, EXT2_INODE_T *inode,
		 						UINT32 bindex, UINT32 *pblknr);
static EXT2_DIR_T *search_dir_block(EXT2_DESC_T *ext2fs, UINT32 *blkbuf,
		 							char *name, int namelen, int type);
static EXT2_DIR_T * ext2fs_dir_lookup(EXT2_DESC_T *ext2fs, UINT32 dir_ino,
										char  *name, int namelen, int type);
static int ext2fs_follow_symlink(EXT2_DESC_T *ext2fs, UINT32 dir_ino, UINT32 sym_ino, 
								INODE_INFO_T *info);
static int ext2fs_inode_lookup(EXT2_DESC_T *ext2fs, UINT32 dir_ino,
			     				char *pathname, INODE_INFO_T *info, int type);
int ext2fs_read(void *fp, char *buf, UINT32 nbytes);

int ext2fs_close(void *fp);

#define __READ_BLOCK(n)                                          	\
	PARTITION_READ(ext2fs->part, EXT2_BLOCK_TO_SECTOR(ext2fs, (n)),    \
                   ext2fs->blocksize/IDE_SECTOR_SIZE, (UINT16 *)blockbuf)

/*----------------------------------------------------------------------
*  ext2fs_init
*----------------------------------------------------------------------*/
int ext2fs_init(void)
{
	int			p;
	IDE_PART_T	*part;
	
	memset((void *)&file_info, 0, sizeof(file_info));
	memset((void *)blockbuf, 0, sizeof(blockbuf));
	
	// looking for zImage & rd.gz
	part = (IDE_PART_T *)&ide_partitions[0];
	for (p=0; p<ide_part_num; p++, part++)
	{
		if (!part->present) continue;
				
		if (part->part_id != 0)	// only handle first partition per disk
			 continue;
		
		// clear file_info if file not found in previous partition
		memset((void *)&file_info, 0, sizeof(file_info));
		if (ext2fs_mount(part, &ext2fs_desc) != 0)
		{
			ext2fs_printf(("Failed to mount!\n"));
			continue;
		}
		
		kernel_fp = NULL;
		initrd_fp = NULL;
#ifdef LOAD_FROM_IDE	
		if ((kernel_fp = ext2fs_open((char *)sys_get_kernel_name())) && 
			(initrd_fp = ext2fs_open((char *)sys_get_initrd_name())))
		{
			ext2fs_close(kernel_fp);
			ext2fs_close(initrd_fp);
			return 1;
		}
		if (kernel_fp) ext2fs_close(kernel_fp);
		if (initrd_fp) ext2fs_close(initrd_fp);
#endif		
	}
	
#ifdef LOAD_FROM_IDE	
	printf("Not found files: %s and %s\n", sys_get_kernel_name(), sys_get_initrd_name());
#endif
	
	return 0;		
}

/*----------------------------------------------------------------------
*  ext2fs_open
*	open a file in a desired partition
*----------------------------------------------------------------------*/
FILE_T *ext2fs_open(char *filepath)
{
	int		i;
	FILE_T	*file;
	
	// get a new file handler
	// currently only support one file at the same time
	file = (FILE_T *)&file_info;
	for (i=0; i<MAX_FILES; i++, file++)
	{
		if (!file->opened)
			break;
	}
	if (i == MAX_FILES)
		return NULL;
	
	memset((void *)file, 0, sizeof(FILE_T));
	file->opened = 1;
	
    // find file inode
    if (!ext2fs_inode_lookup(&ext2fs_desc, EXT2_ROOT_INO, filepath, &file->inode_info, 0))
    {
		ext2fs_printf(("ext2fs_inode_lookup failed\n"));
		return NULL;
    }

    // read inode
    if (!ext2fs_get_inode(&ext2fs_desc, file->inode_info.ino, &file->inode))
    {
		ext2fs_printf(("ext2fs_get_inode failed for ino[%d]\n", file->inode_info.ino));
		return NULL;
    }

	file->ext2fs_desc = &ext2fs_desc;
    file->file_size = SWAB_LE32(file->inode.size);
    file->file_pos  = 0;

    return file;
}

/*----------------------------------------------------------------------
*  ext2fs_mount
*----------------------------------------------------------------------*/
static int ext2fs_mount(IDE_PART_T *part, EXT2_DESC_T *desc)
{
    int			sb_block = 1;
    UINT32		sb_buf[EXT2_MIN_BLOCK_SIZE/sizeof(UINT32)];
    EXT2_SB_T	*sb = (EXT2_SB_T *)sb_buf;
    UINT16		magic;

    desc->part = part;

	// Read super block. Super Block is the 2nd block
    if (!PARTITION_READ(part,
    					sb_block*(EXT2_MIN_BLOCK_SIZE/IDE_SECTOR_SIZE),
						EXT2_MIN_BLOCK_SIZE/IDE_SECTOR_SIZE,
						(UINT16 *)sb))
	{
		return -1;
	}

    if (SWAB_LE16(sb->magic) != EXT2_SUPER_MAGIC)
    {
		ext2fs_printf(("Bad magic 0x%x\n", SWAB_LE16(magic)));
		return -1;
    }

    // save some stuff for easy access
    desc->blocksize = EXT2_BLOCK_SIZE(sb);
    desc->nr_ind_blocks = (desc)->blocksize / sizeof(UINT32);
    desc->nr_dind_blocks = desc->nr_ind_blocks * ((desc)->blocksize / sizeof(UINT32));
    desc->nr_tind_blocks = desc->nr_dind_blocks * ((desc)->blocksize / sizeof(UINT32));
    desc->blocks_per_group = SWAB_LE32(sb->blocks_per_group);
    desc->ngroups = (SWAB_LE32(sb->blocks_count) + desc->blocks_per_group - 1) /
	             desc->blocks_per_group;
    desc->inodes_per_group = SWAB_LE32(sb->inodes_per_group);

    // Find the group descriptors which follow superblock
    desc->gdesc_block = ((sb_block * EXT2_MIN_BLOCK_SIZE) / desc->blocksize) + 1;
    desc->gdesc_first = 0; // cache group 0 initially

    if (!PARTITION_READ(part, EXT2_BLOCK_TO_SECTOR(desc, desc->gdesc_block), 1,
			(UINT16 *)desc->gdesc_cache))
	{
		return -1;
	}

    return 0;
}

/*----------------------------------------------------------------------
*  ext2fs_get_gdesc
*----------------------------------------------------------------------*/
static int ext2fs_get_gdesc(EXT2_DESC_T *ext2fs, UINT32 group_nr, EXT2_GROUP_T *gdesc)
{
    UINT32 sec_nr;

    if (group_nr < ext2fs->gdesc_first ||
		group_nr >= (ext2fs->gdesc_first + EXT2_GDESC_CACHE_SIZE))
	{
		// cache miss
		sec_nr = EXT2_BLOCK_TO_SECTOR(ext2fs, ext2fs->gdesc_block);
		sec_nr += (group_nr / EXT2_GDESC_PER_SECTOR);

		if (!PARTITION_READ(ext2fs->part, sec_nr,
			    sizeof(ext2fs->gdesc_cache)/IDE_SECTOR_SIZE, (UINT16 *)ext2fs->gdesc_cache))
			return 0;

		ext2fs->gdesc_first = (group_nr / EXT2_GDESC_CACHE_SIZE) * EXT2_GDESC_CACHE_SIZE;
    }
	
	memcpy((void *)gdesc, (void *)&ext2fs->gdesc_cache[group_nr - ext2fs->gdesc_first], sizeof(EXT2_GROUP_T));

	return 1;
}

/*----------------------------------------------------------------------
*  ext2fs_get_inode
*----------------------------------------------------------------------*/
static int ext2fs_get_inode(EXT2_DESC_T *ext2fs, int ino, EXT2_INODE_T *ip)
{
	UINT32 offset, sec_nr, buf[IDE_SECTOR_SIZE/sizeof(UINT32)];
	EXT2_GROUP_T gdesc;
	
	// get descriptor for group which this inode belongs to
	if (!ext2fs_get_gdesc(ext2fs, (ino - 1) / ext2fs->inodes_per_group, &gdesc))
		return 0;
	
	if (gdesc.inode_table == 0)
		return 0;
	
	// byte offset within group inode table
	offset = ((ino - 1) % ext2fs->inodes_per_group) * sizeof(EXT2_INODE_T);
	
	// figure out which sector holds the inode
	sec_nr = EXT2_BLOCK_TO_SECTOR(ext2fs, SWAB_LE32(gdesc.inode_table));
	sec_nr += offset / IDE_SECTOR_SIZE;
	
	// and the offset within that sector.
	offset %= IDE_SECTOR_SIZE;
	
	if (!PARTITION_READ(ext2fs->part, sec_nr, 1, (UINT16 *)buf))
		return 0;
	
	memcpy((void *)ip, (char *)buf + offset, sizeof(EXT2_INODE_T));
	
	return 1;
}


/*----------------------------------------------------------------------
*  ext2fs_inode_block
*	Convert a block index into inode data into a block_nr.
*	If successful, store block number in pblknr and return non-zero.
*
*	NB: This needs some block/sector caching to be speedier. But
*		that takes memory and speed is not too bad now for files
*		small enough to avoid double and triple indirection.
*
*----------------------------------------------------------------------*/
static int ext2fs_inode_block(EXT2_DESC_T *ext2fs, EXT2_INODE_T *inode,
		 						UINT32 bindex, UINT32 *pblknr)
{
    if (bindex < EXT2_NR_DIR_BLOCKS)
    {
		*pblknr = SWAB_LE32(inode->block[bindex]);
		return 1;
    }
    bindex -= EXT2_NR_DIR_BLOCKS;

    if (bindex < ext2fs->nr_ind_blocks)
    {
		// Indirect block
		if (!__READ_BLOCK(SWAB_LE32(inode->block[EXT2_IND_BLOCK])))
			return 0;
		*pblknr = SWAB_LE32(blockbuf[bindex]);
		return 1;
	}
	bindex -= ext2fs->nr_ind_blocks;

	if (bindex < ext2fs->nr_dind_blocks)
	{
		// Double indirect block
		if (!__READ_BLOCK(SWAB_LE32(inode->block[EXT2_DIND_BLOCK])))
			return 0;
		if (!__READ_BLOCK(SWAB_LE32(blockbuf[bindex / ext2fs->nr_ind_blocks])))
			return 0;
		*pblknr  = SWAB_LE32(blockbuf[bindex % ext2fs->nr_ind_blocks]);
		return 1;
	}
	bindex -= ext2fs->nr_dind_blocks;

	// Triple indirect block
	if (!__READ_BLOCK(SWAB_LE32(inode->block[EXT2_TIND_BLOCK])))
		return 0;
	
	if (!__READ_BLOCK(SWAB_LE32(blockbuf[bindex / ext2fs->nr_dind_blocks])))
		return 0;
	
	bindex %= ext2fs->nr_dind_blocks;
	
	if (!__READ_BLOCK(SWAB_LE32(blockbuf[bindex / ext2fs->nr_ind_blocks])))
		return 0;
	
	*pblknr = SWAB_LE32(blockbuf[bindex % ext2fs->nr_ind_blocks]);
    
    return 1;
}


/*----------------------------------------------------------------------
*  search_dir_block
*----------------------------------------------------------------------*/
static EXT2_DIR_T *search_dir_block(EXT2_DESC_T *ext2fs, UINT32 *blkbuf,
		 							char *name, int namelen, int type)
{
    EXT2_DIR_T *dir;
    UINT16 reclen, len;
    UINT32 offset;
    UINT8 filename[60+1];

    offset = 0;
    while (offset < ext2fs->blocksize)
    {
		dir = (EXT2_DIR_T *)((char *)blkbuf + offset);
		reclen = SWAB_LE16(dir->reclen);
		offset += reclen;
		len = dir->namelen;

		// terminate on anything which doesn't make sense
		if (reclen < 8 || (len + 8) > reclen || offset > (ext2fs->blocksize + 1))
			return NULL;

		if (type == 0)
		{
			if (dir->inode && len == namelen && !strncmp(dir->name, name, len))
			 	return dir;
		}
		else
		{
			switch(dir->filetype)
			{
				case EXT2_FTYPE_REG_FILE:	printf(" [FILE]    ");	break;
				case EXT2_FTYPE_DIR:		printf(" [DIR]     ");	break;
				case EXT2_FTYPE_CHRDEV:		printf(" [ChrDev]  ");	break;
				case EXT2_FTYPE_BLKDEV:		printf(" [BlkDev]  ");	break;
				case EXT2_FTYPE_FIFO:		printf(" [FIFO]    ");	break;
				case EXT2_FTYPE_SOCK:		printf(" [SOCK]    ");	break;
				case EXT2_FTYPE_SYMLINK:	printf(" [SYM]     ");	break;
				case EXT2_FTYPE_UNKNOWN:	
				default:					printf(" [Unknown] ");	break;
			}
			if (len > 60)
				len = 60;
			memcpy(filename, dir->name, len);
			filename[len] = 0x00;
			printf(filename);
			printf("\n");
		}
	}
	return NULL;
}


/*----------------------------------------------------------------------
*  ext2fs_dir_lookup 
*----------------------------------------------------------------------*/
static EXT2_DIR_T * ext2fs_dir_lookup(EXT2_DESC_T *ext2fs, UINT32 dir_ino,
										char  *name, int namelen, int type)
{
    EXT2_INODE_T inode;
    EXT2_DIR_T *dir;
    UINT32 nblocks, last_block_size, i, block_nr, nbytes;

	if (!ext2fs_get_inode(ext2fs, dir_ino, &inode))
		return NULL;

	nbytes = SWAB_LE32(inode.size);
	nblocks = (nbytes + ext2fs->blocksize - 1) / ext2fs->blocksize;

	last_block_size = nbytes % ext2fs->blocksize;
	if (last_block_size == 0)
		last_block_size = ext2fs->blocksize;

    for (i = 0; i < nblocks; i++)
    {
		if (!ext2fs_inode_block(ext2fs, &inode, i, &block_nr))
			return NULL;

		if (block_nr)
		{
			if (!__READ_BLOCK(block_nr))
				return NULL;
		} else
			memset(blockbuf, 0, ext2fs->blocksize);

		dir = search_dir_block(ext2fs, blockbuf, name, namelen, type);

		if (dir != NULL)
			return dir;
	}
	return NULL;
}


/*----------------------------------------------------------------------
*  ext2fs_follow_symlink 
*		Starting from the given directory, find the inode number, 
*		filetype, and parent inode for the file pointed to by the 
*		given symbolic link inode.
*		f successful, fills out INODE_INFO_T and return true.
*----------------------------------------------------------------------*/
static int ext2fs_follow_symlink(EXT2_DESC_T *ext2fs, UINT32 dir_ino, UINT32 sym_ino, INODE_INFO_T *info)
{
	#define MAX_SYMLINK_NAME 255
	char symlink[MAX_SYMLINK_NAME+1];
	int  pathlen;
	UINT32 block_nr;
	EXT2_INODE_T inode;

    if (!ext2fs_get_inode(ext2fs, sym_ino, &inode))
		return 0;

	pathlen = SWAB_LE32(inode.size);
	if (pathlen > MAX_SYMLINK_NAME)
		return 0;

	if (inode.blocks)
    {
		if (!ext2fs_inode_block(ext2fs, &inode, 0, &block_nr))
			return 0;
		if (block_nr)
		{
			if (!PARTITION_READ(ext2fs->part, EXT2_BLOCK_TO_SECTOR(ext2fs, block_nr),
								ext2fs->blocksize/IDE_SECTOR_SIZE, blockbuf))
				return 0;
			memcpy(symlink, blockbuf, pathlen);
		} else
			return 0;
	}
	else
	{
		// small enough path to fit in inode struct
		memcpy(symlink, (char *)&inode.block[0], pathlen);
    }
    symlink[pathlen] = 0;

    return ext2fs_inode_lookup(ext2fs, dir_ino, symlink, info, 0);
}

/*----------------------------------------------------------------------
*  ext2fs_inode_lookup
*----------------------------------------------------------------------*/
static int ext2fs_inode_lookup(EXT2_DESC_T *ext2fs, UINT32 dir_ino, char *pathname,
								INODE_INFO_T *info, int type)
{
    int len, pathlen;
    char *p;
    EXT2_DIR_T *dir = NULL;
    
    if (!pathname || (pathlen = strlen(pathname)) == 0)
		return 0;

    if (*pathname == '/')
    {
		if (--pathlen == 0)
		{
	    	info->ino = info->parent_ino = EXT2_ROOT_INO;
	    	info->filetype = EXT2_FTYPE_DIR;
	    	return 1;
		}
		++pathname;
		dir_ino = EXT2_ROOT_INO;
    }

    while (pathlen)
    {
		int is_file = 1;
		// find next delimiter in path.
		for (p = pathname, len = 0; len < pathlen; len++, p++)
		{
			// skip delimiter if found.
			if (*p == '/')
			{
				++p;
				--pathlen;
				is_file = 0;
				break;
			}
		}
		
		if (type == 0)
			dir = ext2fs_dir_lookup(ext2fs, dir_ino, pathname, len, type);
		else
			dir = ext2fs_dir_lookup(ext2fs, dir_ino, pathname, len, is_file);
		
		if (dir == NULL)
		    return 0;
    	
		pathlen -= len;
		pathname = p;
    	
		switch (dir->filetype)
		{
			case EXT2_FTYPE_SYMLINK:
				// follow the symbolic link (this will cause recursion)
				if (!ext2fs_follow_symlink(ext2fs, dir_ino, SWAB_LE32(dir->inode), info))
					return 0;
				if (pathlen == 0)
					return 1;
				// must be a dir if we want to continue
				if (info->filetype != EXT2_FTYPE_DIR)
					return 0;
				dir_ino = info->ino;
				break;
			
			case EXT2_FTYPE_DIR:
				if (pathlen)
					dir_ino = SWAB_LE32(dir->inode);
				break;
			
			case EXT2_FTYPE_REG_FILE:
				if (pathlen)
					return 0;  // regular file embedded in middle of path
				break;
    	
			case EXT2_FTYPE_UNKNOWN:
			case EXT2_FTYPE_CHRDEV:
			case EXT2_FTYPE_BLKDEV:
			case EXT2_FTYPE_FIFO:
			case EXT2_FTYPE_SOCK:
			default:
				return 0;
		}
	}
    
    info->ino = SWAB_LE32(dir->inode);
    info->parent_ino = dir_ino;
    info->filetype = dir->filetype;
    return 1;
}

/*----------------------------------------------------------------------
*  ext2fs_read
*----------------------------------------------------------------------*/
int ext2fs_read(void *fp, char *buf, UINT32 nbytes)
{
    FILE_T *info = fp;
    EXT2_DESC_T *ext2fs;
    UINT32 nread = 0, rem, block_nr, bindex, to_read;

    if ((info->file_pos + nbytes) > info->file_size)
		nbytes = info->file_size - info->file_pos;

    ext2fs = info->ext2fs_desc;
    
    // see if we need to copy leftover data from last read call
    rem = ext2fs->blocksize - (info->file_pos % ext2fs->blocksize);
    if (rem != ext2fs->blocksize)
    {
		char *p = (char *)blockbuf + ext2fs->blocksize - rem;

		if (rem > nbytes)
			rem = nbytes;

		memcpy(buf, p, rem);

		nread += rem;
		buf  += rem;
		info->file_pos += rem;
    }
    
	// now loop through blocks if we're not done
	bindex = info->file_pos / ext2fs->blocksize;
	while (nread < nbytes)
	{
		if (!ext2fs_inode_block(ext2fs, &info->inode, bindex, &block_nr))
			return -1;

		if (block_nr)
		{
			if (!PARTITION_READ(ext2fs->part, EXT2_BLOCK_TO_SECTOR(ext2fs, block_nr),
								ext2fs->blocksize/IDE_SECTOR_SIZE, blockbuf))
				return 0;
		} else
			memset(blockbuf, 0, ext2fs->blocksize);
	
		to_read = nbytes - nread;
		if (to_read > ext2fs->blocksize)
			to_read = ext2fs->blocksize;

		memcpy(buf, blockbuf, to_read);

		nread += to_read;
		buf += to_read;
		info->file_pos += to_read;
		++bindex;
    }

    return nread;
}

/*----------------------------------------------------------------------
*  ext2fs_close
*----------------------------------------------------------------------*/
int ext2fs_close(void *fp)
{
    FILE_T *info = fp;
	
	info->opened = 0;
}	

/*----------------------------------------------------------------------
*  ext2fs_file_size
*----------------------------------------------------------------------*/
int ext2fs_file_size(void *fp)
{
    FILE_T *info = fp;
	
	return info->file_size;
}

/*----------------------------------------------------------------------
*  ext2fs_ls
*	List file
*----------------------------------------------------------------------*/
void ext2fs_ls(char *pathname)
{
	INODE_INFO_T inode_info;
	ext2fs_inode_lookup(&ext2fs_desc, EXT2_ROOT_INO, pathname, &inode_info, 1);
}


/*----------------------------------------------------------------------
*  ext2fs_ls_cmd
*	CLI command to list file
*----------------------------------------------------------------------*/
void ext2fs_ls_cmd(char argc, char *argv[])
{
	char pathname[81];
	
#if 1	
	if (argc <= 1)
		strcpy(pathname, "/*");
	else if (strlen(argv[1]) <= 80)
		sprintf(pathname, "%s/*", argv[1]);
	else
		printf("Pathname is too long! (>= 80)\n");

#else
	strcpy(pathname, "/*");
#endif

	ide_init();
	ext2fs_init();
	
	ext2fs_ls(pathname);
		
}

#endif // BOARD_SUPPORT_IDE
