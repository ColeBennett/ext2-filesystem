/**** stat.c file ****/
#ifndef STAT_C
#define STAT_C

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <libgen.h>
#include <pwd.h>
#include <grp.h>
#include "../type.h"
#include "../alloc_dealloc.c"

extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC proc[NPROC], *running;
extern char gpath[256];
extern char *name[64];
extern int n;
extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, inode_start;
extern char line[256], cmd[32], pathname[256];

const char *t = "xwrxwrxwr";
int fsstat(char *pathname)
{
    int ino = getino(pathname);
    if (!ino) {
        printf("%s not found\n", pathname);
        return;
    }
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    /*
    File: main.c
    Size: 4329      	Blocks: 16         IO Block: 4096   regular file
    Device: 53h/83d	Inode: 8609807955  Links: 1
    Access: (0644/-rw-r--r--)  Uid: (    0/    root)   Gid: (    0/    root)
    Access: 2019-03-31 11:09:14.000000000 +0000
    Modify: 2019-03-31 10:59:20.000000000 +0000
    Change: 2019-03-31 10:59:20.000000000 +0000
    */
    char *basec, *bname;
    basec = strdup(pathname);
    bname = basename(basec);
    printf("File: %s\n", bname);
    printf("Size: %-10u Blocks: %-10u IO Block: %-6u ",
     ip->i_size, ip->i_blocks, BLKSIZE);
    if (S_ISDIR(ip->i_mode)) {
        printf("directory");
    } else if (S_ISREG(ip->i_mode)) {
        printf("regular file");
    } else if (S_ISLNK(ip->i_mode)) {
        printf("link file");
    }
    printf("\nDevice: %u Inode: %u Links: %u\n", mip->dev, mip->ino, ip->i_links_count);

    printf("Access: (%o/", ip->i_mode);
    if (S_ISDIR(ip->i_mode)) {
        printf("d");
    } else if (S_ISREG(ip->i_mode)) {
        printf("-");
    } else if (S_ISLNK(ip->i_mode)) {
        printf("l");
    }
    for (int i = 8; i >= 0; i--) {
        if (ip->i_mode & (1 << i)) {
            printf("%c", t[i]);
        } else {
            printf("-");
        }
    }

    /* Print uid and username */
    printf(")  Uid: (%5d/", ip->i_uid);
    struct passwd *pw;
    pw = getpwuid(ip->i_uid);
    printf("%8s)", pw ? pw->pw_name : "");
    
    /* Print gid and group name */
    printf("  Gid: (%5d/", ip->i_gid);
    struct group *g;
    g = getgrgid(ip->i_gid);    
    printf("%8s)\n", g ? g->gr_name : "");
    
    time_t iatime = (time_t) ip->i_atime;
    time_t imtime = (time_t) ip->i_mtime;
    time_t ictime = (time_t) ip->i_ctime;
    printf("Access: %s", ctime(&iatime));
    printf("Modify: %s", ctime(&imtime));
    printf("Change: %s", ctime(&ictime));
    iput(mip);
}

#endif
