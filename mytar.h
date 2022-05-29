/* header file for mytar.c */

#ifndef _MYTAR_
#define _MYTAR_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <utime.h>
#include <time.h>

#define NAME_LEN 100
#define MODE_LEN 8
#define UID_LEN 8
#define GID_LEN 8
#define SIZE_LEN 12
#define MTIME_LEN 12
#define CHKSUM_LEN 8
#define TYPEFLAG_LEN 1
#define LINKNAME_LEN 100
#define MAGIC_LEN 6
#define VERSION_LEN 2
#define UNAME_LEN 32
#define GNAME_LEN 32
#define DEVMAJ_LEN 8
#define DEVMIN_LEN 8
#define PREFIX_LEN 155
#define ZEROBUF_LEN 12

#define FIRST_FILE 3
#define BLOCK 512
#define OCTAL 8
#define CHKSUM_POS 148
#define MODE_START 2
#define MODE_END 5
#define SYMLINK 50
#define DIRECT 53
#define BASE_YEAR 1900
#define ALL_PERM 0777
#define RW_ALL 0666
#define NUM_PERMS 9
#define OCTAL_PERMS 3
#define ALL_MODEPERM 07777
#define MAX_ID 07777777
#define OVERFLOW 101
#define FULL_PERMS 10
#define BASE_TEN 10

typedef struct header {
  char name[NAME_LEN];
  char mode[MODE_LEN];
  char uid[UID_LEN];
  char gid[GID_LEN];
  char size[SIZE_LEN];
  char mtime[MTIME_LEN];
  char chksum[CHKSUM_LEN];
  char typeflag[TYPEFLAG_LEN];
  char linkname[LINKNAME_LEN];
  char magic[MAGIC_LEN];
  char version[VERSION_LEN];
  char uname[UNAME_LEN];
  char gname[GNAME_LEN];
  char devmaj[DEVMAJ_LEN];
  char devmin[DEVMIN_LEN];
  char prefix[PREFIX_LEN];
  /*allows us to read block by block without seeking 12 everytime*/
  char zerobuf[ZEROBUF_LEN];
} header;

/*function prototypes */
void c_tar(int fdtar, int optopt[3], char **files, int numfiles); 
void t_tar(int fdtar, int optopt[3], char **files, int numfiles);
void x_tar(int fdtar, int optopt[3], char **files, int numfiles);
int build_header(int fdtar, char *path, int optopt[3]);
void c_tar_helper(char *file, int optopt[3], int fdtar);

#endif
