/**** rmdir.c file ****/
#ifndef RMDIR_C
#define RMDIR_C

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include "../type.h"
#include "../alloc_dealloc.c"
#include "../util.c"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

int fsrmdir(char *pathname)
{
    int ino = getino(pathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }

    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    if (!S_ISDIR(ip->i_mode)) {
        printf("%s is not a directory\n", pathname);
        iput(mip);
        return;
    }
    if (mip->refCount > 1) {
        printf("%s is busy, refcnt=%d\n", pathname, mip->refCount);
        iput(mip);
        return;
    }

    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int i, count = 0;
    for (i = 0; i < 12; i++) {
        if (ip->i_block[i] == 0)
            break;
        get_block(dev, ip->i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;
        while (cp < buf + BLKSIZE) {
            if (++count > 2) {
                printf("Directory %s is not empty\n", pathname);
                return;
            }
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }
    }
    printf("dir is empty, count = %d\n", count);

    int blk;
    for (i = 0; i < 12; i++) {
        blk = ip->i_block[i];
        if (blk == 0)
            break;
        bdalloc(mip->dev, blk);
    }
    idalloc(mip->dev, mip->ino);
    iput(mip);

    int pino = getparentino(ino);
    MINODE *pmip = iget(mip->dev, pino);
    char name[256];
    findname(pmip, ino, name);
    rm_child(pmip, name);

    (&pmip->INODE)->i_links_count--;
    (&pmip->INODE)->i_atime = (&pmip->INODE)->i_mtime = (u32) time(0L);
    pmip->dirty = 1;
    iput(pmip);
}

/* Removes the dir entry from the parent directory inode. */
void rm_child(MINODE *pmip, char *name)
{
    printf("rm_child %s\n", name);
    INODE *ip = &pmip->INODE;
    char buf[BLKSIZE], tmp[256];
    DIR *dp = (DIR *) buf;
    DIR *prevdp = 0;
    char *cp = buf; 
    int i, j, blk;
    for (i = 0; i < 12; i++) { // assume DIR at most 12 direct blocks
        blk = ip->i_block[i]; 
        if (blk == 0)
            break;
        get_block(pmip->dev, blk, buf);
        dp = (DIR *) buf;
        cp = buf;
        while (cp < buf + BLKSIZE) {
            strncpy(tmp, dp->name, dp->name_len);
            tmp[dp->name_len] = 0;
            if (strcmp(tmp, name) == 0) {
                int ideal_len = ideal_rec_len(dp->name_len);         
                if (dp->rec_len == BLKSIZE) { // first and only entry in the data block
                    (&pmip->INODE)->i_size -= BLKSIZE;

                    bzero(buf, BLKSIZE);
                    put_block(pmip->dev, ip->i_block[i], buf);
                    bdalloc(pmip->dev, ip->i_block[i]);

                    /* Remove element and shift i_block[] array to the left
                    before | 1 | 2 | 3 | x | 5 | 6 | 0 | 0 | 0 | 0 | 0 | 0 |
                    after  | 1 | 2 | 3 | 5 | 6 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | */
                    for (int j = i; j < 12-1; j++)
                        ip->i_block[j] = ip->i_block[j + 1];
                    printf("rmchild : removed entire block (was first and only entry)\n");
                } else if (dp->rec_len != ideal_len) { // last entry in the data block
                    prevdp->rec_len += dp->rec_len;
                    printf("rmchild : absorb rec_len in new last entry (last entry in block)\n");
                } else { // entry is first but not the only entry or in the middle of a block
                    /* Find last entry in block */
                    char *tcp = cp;
                    DIR *tdp = (DIR *) dp;
                    int subseq_len = 0;
                    while (tcp + tdp->rec_len < buf + BLKSIZE) {
                        tcp += dp->rec_len;
                        subseq_len += dp->rec_len;
                        tdp = (DIR *) tcp;
                    }
                    /* Add deleted rec_len to the last entry's rec_len */
                    tdp->rec_len += dp->rec_len;
                    /* Move all entries after the deleted entry to start
                    where the deleted entry was located in the block */
                    memcpy(cp, cp + dp->rec_len, subseq_len);
                    printf("rmchild : moved subsequent entries to overwrite deleted entry (first but not only or in middle of block)\n");
                }
                put_block(pmip->dev, blk, buf);
                pmip->dirty = 1;
                return;
            }
            cp += dp->rec_len;
            prevdp = dp;
            dp = (DIR *) cp;
        }
    }
}

#endif
