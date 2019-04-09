/**** cd_ls_pwd.c file ****/
#ifndef CD_LS_PWD_C
#define CD_LS_PWD_C

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <sys/stat.h>
#include "../type.h"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

#define OWNER 000700
#define GROUP 000070
#define OTHER 000007

int change_dir(char *pathname)
{
    if (!pathname || strlen(pathname) == 0) {
        running->cwd = root;
        printf("Changed directory to /\n");
        return;
    }
    int ino = getino(pathname);
    if (ino == 0) {
        printf("%s not found\n", pathname);
        return;
    }
    printf("cd found ino %d\n", ino);
    MINODE *mip = iget(dev, ino);
    if (S_ISDIR((&mip->INODE)->i_mode)) {
        iput(running->cwd);
        running->cwd = mip;
        printf("Changed directory to %s\n", pathname);
    } else {
        printf("%s is not a directory\n", pathname);
    }
}

const char *t = "xwrxwrxwr";
void ls_file(int ino, char *name)
{
    int i;
    char ftime[64];

    MINODE *mip = iget(dev, ino);
    INODE *ip = &(mip->INODE);

    if (S_ISDIR(ip->i_mode)) {
        printf("d");
    } else if (S_ISREG(ip->i_mode)) {
        printf("-");
    } else if (S_ISLNK(ip->i_mode)) {
        printf("l");
    }

    for (i = 8; i >= 0; i--) {
        if (ip->i_mode & (1 << i)) {
            printf("%c", t[i]);
        } else {
            printf("-");
        }
    }

    printf("%4hu ", ip->i_links_count);
    printf("%4hu ", ip->i_gid);
    printf("%4hu ", ip->i_uid);
    printf("%8u ", ip->i_size);
    
    time_t mtime = (time_t) ip->i_mtime;
    strcpy(ftime, ctime(&mtime));
    ftime[strlen(ftime) - 1] = 0;
    printf("%s ", ftime);

    printf("%s", basename(name));

    if (S_ISLNK(ip->i_mode)) {
        char linkname[64];
        readlink(name, linkname, 64);
        printf(" -> %s", linkname);
    }
    printf("\n");
    iput(mip);
}

int ls_dir(char *pathname)
{
    int ino = getino(pathname);
    if (ino == 0) {
        printf("%s not found\n", pathname);
        return;
    }
    
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    if (!S_ISDIR(ip->i_mode)) {
        printf("%s is not a directory\n", pathname);
        return;
    }

    char buf[BLKSIZE], tmp[255];
    DIR *dp = (DIR *) buf;
    char *cp = buf;
    get_block(dev, ip->i_block[0], buf);
    while (cp < buf + BLKSIZE) {
        strncpy(tmp, dp->name, dp->name_len);
        tmp[dp->name_len] = 0;
        ls_file(dp->inode, tmp);
        cp += dp->rec_len;
        dp = (DIR *) cp;
    }
    iput(mip);
}

void rpwd(MINODE *wd)
{
    if (wd == root)
        return;
    INODE *ip = &wd->INODE;
    char buf[BLKSIZE], tmp[256];
    int ino, parentino;

    ino = wd->ino;
    get_block(wd->dev, ip->i_block[0], buf);
    
    /* get parent ino */
    char *cp = buf;   
    DIR *dp = (DIR *) cp; 
    cp += dp->rec_len;
    dp = (DIR *) cp;
    parentino = dp->inode;

    /* find name of current ino in parent dir */
    MINODE *pip = iget(wd->dev, parentino);
    get_block(wd->dev, (&pip->INODE)->i_block[0], buf);
    dp = (DIR *) buf;
    cp = buf;
    while (cp < buf + BLKSIZE) {
        if (dp->inode == ino) {
            strncpy(tmp, dp->name, dp->name_len);
            tmp[dp->name_len] = 0;
            break;
        }
        cp += dp->rec_len;
        dp = (DIR *) cp;
    }
    rpwd(pip);
    iput(pip);
    printf("/%s", tmp);
}

int pwd()
{
    MINODE *wd = running->cwd;
    if (wd == root) {
        printf("/");
    } else {
        rpwd(wd);
    }
    putchar('\n');
}

#endif
