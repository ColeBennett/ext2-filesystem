/**** util.c file ****/
#ifndef UTIL_C
#define UTIL_C

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

int ideal_rec_len(int n_len)
{
    return 4 * ((8 + n_len + 3) / 4);
}

void get_block(int dev, int blk, char *buf)
{
    lseek(dev, (long) blk * BLKSIZE, 0);
    read(dev, buf, BLKSIZE);
}

void put_block(int dev, int blk, char *buf)
{
    lseek(dev, (long) blk * BLKSIZE, 0);
    write(dev, buf, BLKSIZE);
}

void get_super_block(int dev, char *buf)
{
    get_block(dev, 1, buf);
}

void get_groupdesc_block(int dev, char *buf)
{
    get_block(dev, 2, buf);
}

void put_super_block(int dev, char *buf)
{
    put_block(dev, 1, buf);
}

void put_groupdesc_block(int dev, char *buf)
{
    put_block(dev, 2, buf);
}

int tst_bit(char *buf, int bit)
{
    return buf[bit / 8] & (1 << (bit % 8));
}

void set_bit(char *buf, int bit)
{
    buf[bit / 8] |= (1 << (bit % 8));
}

void clr_bit(char *buf, int bit)
{
    buf[bit / 8] &= ~(1 << (bit % 8));
}

int get_blkno(int ino)
{
    return (ino - 1) / INODES_PER_BLOCK + inode_start;
}

int get_blkoffset(int ino)
{
    return (ino - 1) % INODES_PER_BLOCK;
}

void tokenize(char *pathname)
{
    strcpy(gpath, pathname);
    n = 0;
    char *tok = strtok(gpath, "/");
    while (tok) {
        name[n++] = tok;
        tok = strtok(0, "/");
    }
    name[n] = 0;
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
    int i;
    MINODE* mip;
    INODE* ip;
    char buf[BLKSIZE];
    int blk, offset;

    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        if (mip->dev == dev && mip->ino == ino) {
            mip->refCount++;
            printf("found [dev=%d ino=%d] as minode[%d] in core\n", dev, ino, i);
            return mip;
        }
    }

    for (i = 0; i < NMINODE; i++) {
        mip = &minode[i];
        if (mip->refCount == 0) {
            printf("allocating NEW minode[%d] for [dev=%d ino=%d]\n", i, dev, ino);
            mip->refCount = 1;
            mip->dev = dev;
            mip->ino = ino;

            // get INODE of ino to buf
            blk = get_blkno(ino);
            offset = get_blkoffset(ino);
            printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);
            
            get_block(dev, blk, buf);
            // copy INODE to mp->INODE
            mip->INODE = *((INODE *) buf + offset);
            return mip;
        }
    }
    printf("PANIC: no more free minodes\n");
    return 0;
}

void iput(MINODE *mip)
{
    if (mip == 0)
        return;
    
    int i, blk, offset;
    char buf[BLKSIZE];
    INODE* ip;

    mip->refCount--;
    if (mip->refCount > 0)
        return;
    if (!mip->dirty)
        return;

    /* write back */
    printf("iput: dev=%d ino=%d\n", mip->dev, mip->ino);

    blk = ((mip->ino - 1) / 8) + inode_start;
    offset = (mip->ino - 1) % 8;

    /* first get the block containing this inode */
    get_block(mip->dev, blk, buf);

    ip = (INODE*) buf + offset;
    *ip = mip->INODE;
    put_block(mip->dev, blk, buf);
}

int findname(MINODE *mip, int ino, char *dst)
{
    char buf[BLKSIZE];
    DIR *dp;
    char *cp;
    int i;
    INODE *ip = &mip->INODE;
    for (i = 0; i < 12; i++) { // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
            break;
        get_block(dev, ip->i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;
        while (cp < buf + BLKSIZE) {
            if (dp->inode == ino) {
                strncpy(dst, dp->name, dp->name_len);
                dst[dp->name_len] = 0;
                return 1;
            }
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }
    }
    return 0;
}

int search(MINODE *mip, char *name)
{
    char buf[BLKSIZE], tmp[256];
    DIR *dp;
    char *cp;
    int i;
    INODE *ip = &mip->INODE;
    for (i = 0; i < 12; i++) { // assume DIR at most 12 direct blocks
        if (ip->i_block[i] == 0)
            break;
        get_block(dev, ip->i_block[i], buf);
        dp = (DIR *) buf;
        cp = buf;
        while (cp < buf + BLKSIZE) {
            strncpy(tmp, dp->name, dp->name_len);
            tmp[dp->name_len] = 0;
            if (strcmp(tmp, name) == 0)
                return dp->inode;
            cp += dp->rec_len;
            dp = (DIR *) cp;
        }
    }
    return 0;
}

int getino(char *pathname)
{
    int i, ino = 0;
    INODE *ip;
    MINODE *mip;

    if (strcmp(pathname, "/") == 0)
        return 2;
    
    if (pathname[0] == '/') {
        mip = iget(dev, 2);
    } else {
        mip = iget(running->cwd->dev, running->cwd->ino);
    }
    
    tokenize(pathname);
    for (i = 0; i < n; i++) {
        ino = search(mip, name[i]);
        if (!ino) {
            iput(mip);
            printf("name %s does not exist\n", name[i]);
            return 0;
        }
        iput(mip);
        mip = iget(dev, ino);
    }
    iput(mip);
    return ino;
}

int getparentino(int ino)
{
    char buf[BLKSIZE];
    MINODE *mip = iget(dev, ino);
    get_block(mip->dev, (&mip->INODE)->i_block[0], buf);
    DIR *dp = (DIR *) buf;
    char *cp = buf + dp->rec_len;
    dp = (DIR *) cp;
    iput(mip);
    return dp->inode;
}

#endif
