/**** alloc_dealloc.c file ****/
#ifndef ALLOC_DEALLOC_C
#define ALLOC_DEALLOC_C

#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern int nblocks, ninodes, bmap, imap, inode_start;

MINODE *mialloc()
{
    int i;
    for (i = 0; i < NMINODE; i++) {
        MINODE *mip = &minode[i];
        if (mip->refCount == 0) {
            mip->refCount = 1;
            return mip;
        }
    }
    printf("Out of minodes\n");
    return 0;
}

void midealloc(MINODE *mip)
{
    mip->refCount = 0;
}

/* Increment free inodes count in SUPER and GD. */
int inc_free_inodes(int dev)
{
    char buf[BLKSIZE];
    get_super_block(dev, buf);
    sp = (SUPER *) buf;
    sp->s_free_inodes_count++;
    put_super_block(dev, buf);

    get_groupdesc_block(dev, buf);  
    gp = (GD *) buf;
    gp->bg_free_inodes_count++;
    put_groupdesc_block(dev, buf);
}

/* Increment free blocks count in SUPER and GD. */
int inc_free_blocks(int dev)
{
    char buf[BLKSIZE];
    get_super_block(dev, buf);
    sp = (SUPER *) buf;
    sp->s_free_blocks_count++;
    put_super_block(dev, buf);

    get_groupdesc_block(dev, buf);  
    gp = (GD *) buf;
    gp->bg_free_blocks_count++;
    put_groupdesc_block(dev, buf);
}

/* Decrement free inodes count in SUPER and GD. */
int dec_free_inodes(int dev)
{
    char buf[BLKSIZE];
    get_super_block(dev, buf);
    sp = (SUPER *) buf;
    sp->s_free_inodes_count--;
    put_super_block(dev, buf);

    get_groupdesc_block(dev, buf);  
    gp = (GD *) buf;
    gp->bg_free_inodes_count--;
    put_groupdesc_block(dev, buf);
}

/* Decrement free blocks count in SUPER and GD. */
int dec_free_blocks(int dev)
{
    char buf[BLKSIZE];
    get_super_block(dev, buf);
    sp = (SUPER *) buf;
    sp->s_free_blocks_count--;
    put_super_block(dev, buf);

    get_groupdesc_block(dev, buf);  
    gp = (GD *) buf;
    gp->bg_free_blocks_count--;
    put_groupdesc_block(dev, buf);
}

/* Returns a FREE inode block number. */
int ialloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // read inode_bitmap block
    get_block(dev, imap, buf);
    for (i = 0; i < ninodes; i++) {
        if (tst_bit(buf, i) == 0) {
            set_bit(buf, i);
            put_block(dev, imap, buf);
            dec_free_inodes(dev);
            return i + 1;
        }
    }
    printf("Out of inodes\n");
    return 0;
}

int idalloc(int dev, int ino)
{
    char buf[BLKSIZE];
    get_block(dev, imap, buf);
    clr_bit(buf, ino - 1);
    put_block(dev, imap, buf);
    inc_free_inodes(dev);
}

/* Returns a FREE disk block number. */
int balloc(int dev)
{
    int i;
    char buf[BLKSIZE];
    // read block_bitmap block
    get_block(dev, bmap, buf);
    for (i = 0; i < nblocks; i++) {
        if (tst_bit(buf, i) == 0) {
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            dec_free_blocks(dev);
            return i + 1;
        }
    }
    printf("Out of blocks\n");
    return 0;
}

int bdalloc(int dev, int bno)
{
    char buf[BLKSIZE];
    get_block(dev, bmap, buf);
    clr_bit(buf, bno - 1);
    put_block(dev, bmap, buf);
    inc_free_blocks(dev);
}

void trucate_indirect_blocks(int dev, char *buf)
{
    int *up = (int *) buf;
    while (*up && up < buf + BLKSIZE) {
        bdalloc(dev, *up);
        up++;
    }
}

void itruncate(MINODE *mip)
{
    int i;
    INODE *ip = &mip->INODE;
    for (i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0)
            break;
        bdalloc(mip->dev, ip->i_block[i]);
    }
    char buf[BLKSIZE];
    /* Deallocaate indirect blocks */
    if (ip->i_block[12]) {
        get_block(mip->dev, ip->i_block[12], buf);
        trucate_indirect_blocks(mip->dev, buf);
    }
    /* Deallocate double indirect blocks */
    if (ip->i_block[13]) {
        /* i_block[13] may point to a disk block, which points 256 blocks, each
        of which points to 256 disk blocks. */
        get_block(dev, ip->i_block[13], buf);
        char indbuf[BLKSIZE];
        int *up = (int *) buf;
        while (*up && up < buf + BLKSIZE) {
            get_block(mip->dev, *up, indbuf);
            trucate_indirect_blocks(mip->dev, indbuf);
            up++;
        }
    }
    ip->i_atime = (u32) time(0L);
    ip->i_mtime = (u32) time(0L);
    ip->i_size = 0;
    mip->dirty = 1;
}

#endif
