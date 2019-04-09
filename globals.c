#include "type.h"

MINODE minode[NMINODE];
MINODE *root;
PROC proc[NPROC], *running;

char gpath[256]; // global for tokenized components
char *name[64]; // assume at most 64 components in pathname

int n; // number of component strings
int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start;
char line[256], cmd[32], pathname[256];
