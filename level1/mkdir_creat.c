/**** mkdir_creat.c file ****/
#ifndef MKDIR_CREAT_C
#define MKDIR_CREAT_C

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

void enter_name(MINODE *pip, int ino, char *name, int is_dir)
{
    INODE *ip = &pip->INODE;
    char buf[BLKSIZE], *cp;
    DIR *dp;
    int i, n_len, need_len, blk, rem_len;
    int found_space = 0;
    n_len = (int)strlen(name);
    need_len = ideal_rec_len(n_len); // a multiple of 4
    for (i = 0; i < 12; i++)
    {
        blk = ip->i_block[i];
        if (blk == 0)
            break;
        get_block(pip->dev, blk, buf);
        /* IDEAL_LEN = 4*[ (8 + name_len + 3)/4 ] */
        /*
        All DIR entries in a data block have rec_len = IDEAL_LEN, except the last
        entry. The rec_len of the LAST entry is to the end of the block, which may
        be larger than its IDEAL_LEN.
        |-4---2----2--|----|---------|--------- rlen ->------------------------|
        |ino rlen nlen NAME|.........|ino rlen nlen|NAME                       |
        |----------------------------------------------------------------------|
        */
        printf("step to LAST entry in data block %d\n", blk);
        dp = (DIR *)buf;
        cp = buf;
        while (cp + dp->rec_len < buf + BLKSIZE) {
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }

        // dp now points to last entry
        rem_len = dp->rec_len - ideal_rec_len(dp->name_len);
        printf("need %d, last rec_len %d, ideal last %d, rem %d\n",
               need_len, dp->rec_len, ideal_rec_len(dp->name_len), rem_len);
        if (rem_len >= need_len)
        {
            found_space = 1;
            /* trim current last entry to its ideal length */
            dp->rec_len = ideal_rec_len(dp->name_len);

            /* get pointer to start of leftover space for new DIR entry */
            cp += dp->rec_len;
            dp = (DIR *)cp;
            /* enter the new entry as the last entry in the block */
            dp->inode = ino;
            dp->rec_len = rem_len;
            dp->name_len = n_len;
            //dp->file_type = is_dir ? S_IF : S_IFREG;
            strncpy(dp->name, name, n_len);
            put_block(pip->dev, blk, buf);
            printf("found space, added new dir to end of blk=%d\n", blk);
            break;
        }
    }
    if (!found_space)
    {
        /* no space in existing data block(s) */
        int bno = balloc(pip->dev);
        ip->i_size += BLKSIZE;
        ip->i_block[i] = bno;

        dp = (DIR *)buf;
        dp->inode = ino;
        dp->rec_len = BLKSIZE;
        dp->name_len = n_len;
        dp->file_type = is_dir ? S_IFDIR : S_IFREG;
        strncpy(dp->name, name, n_len);
        put_block(pip->dev, bno, buf);
        printf("allocated space for dir\n");
    }
}

int make_entry(char *pathname, int is_dir)
{
    MINODE *mip, *pip;
    INODE *ip;
    int dev, ino, bno, i;
    char *dirc, *basec, *bname, *dname;

    dirc = strdup(pathname);
    basec = strdup(pathname);
    dname = dirname(dirc);
    bname = basename(basec);
    printf("dirname=%s basename=%s\n", dname, bname);

    if (pathname[0] == '/')
    { // root
        dev = root->dev;
    }
    else
    { // relative to cwd
        dev = running->cwd->dev;
    }

    /* get parent ino */
    int pino = getino(dname);
    if (pino == 0)
    {
        printf("Parent directory not found: %s\n", dname);
        return;
    }
    printf("parentino=%d\n", pino);
    /* get parent minode and check if it is a directory */
    pip = iget(dev, pino);
    ip = &pip->INODE;
    if (!S_ISDIR(ip->i_mode))
    {
        printf("%s parent is not a directory\n", dname);
        return 0;
    }
    /* check if child is not in the parent directory */
    if (search(pip, bname))
    {
        printf("%s already exists in directory %s\n", bname, dname);
        return 0;
    }

    /******************************************************************/
    dev = pip->dev;
    ino = ialloc(dev);
    bno = is_dir ? balloc(dev) : 0;

    mip = iget(dev, ino);
    ip = &mip->INODE;

    /*
    DIR 0x41ED 0100000111101101
    REG 0x81A4 1000000110100100
    LNK 0xA1A4 1010000110100100
    */
    ip->i_mode = is_dir ? 0x41ED : 0x81A4;
    ip->i_uid = (&running->cwd->INODE)->i_uid;
    ip->i_gid = (&running->cwd->INODE)->i_gid;
    ip->i_size = is_dir ? BLKSIZE : 0;
    ip->i_links_count = is_dir ? 2 : 1; // count=2 for dir because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = (u32)time(0L);
    ip->i_blocks = is_dir ? 2 : 0;
    ip->i_block[0] = bno;
    for (i = 1; i < 14; i++)
    {
        ip->i_block[i] = 0;
    }
    mip->dirty = 1;
    iput(mip);

    /*
    Write . and .. entries into a buf[ ] of BLKSIZE
    | entry .     | entry ..                                             |
    ----------------------------------------------------------------------
    |ino|12|1|.   |pino|1012|2|..                                        |
    ----------------------------------------------------------------------
    */
    if (is_dir)
    {
        char buf[BLKSIZE];
        char *cp = buf;
        DIR *dp = (DIR *)buf;
        /* write current directory . */
        dp->inode = ino;
        dp->rec_len = 12;
        dp->name_len = 1;
        dp->name[0] = '.';

        /* write parent directory .. */
        int rem_len = BLKSIZE - dp->rec_len;
        cp += dp->rec_len;
        dp = (DIR *) cp;
        dp->inode = pino;
        dp->rec_len = rem_len;
        dp->name_len = 2;
        dp->name[0] = dp->name[1] = '.';

        put_block(dev, bno, buf);
        printf("wrote . and ..\n");
    }

    /* enter name ENTRY into parent's directory */
    enter_name(pip, ino, bname, is_dir);
    /******************************************************************/

    if (is_dir)
        (&pip->INODE)->i_links_count++;
    (&pip->INODE)->i_atime = (u32)time(0L);
    pip->dirty = 1;
    iput(pip);
    return ino;
}

int make_dir(char *pathname)
{
    make_entry(pathname, 1);
}

int creat_file(char *pathname)
{
    make_entry(pathname, 0);
}

#endif
