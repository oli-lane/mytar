#include "mytar.h"

/*archives files following tar(1) standards. 
Command line options:
c : create archive
t : print table of contents of archive
x : extract the contents of archive
v : increases verbosity, prints more info on files
f : specifies archive filename, mandatory argument
S : enforces strict compliance to standard tar conventions */
int main(int argc, char *argv[]) {
  int i, fdtar, numfiles = 0;
  char reqopt = '\0'; /* c | t | x */
  int optopt[3] = {0}; /* v=optopt[0] S=optopt[1] f=optopt[2]*/
  char **files = NULL;
  char nullbuf[BLOCK * 2];
  /* check argv for errors */
  if (argc < 3) {
    fprintf(stderr, "incorrect number of arguments");
    exit(1);
  }
  /*parse options*/
  if (strlen(argv[1]) < 2 || strlen(argv[1]) > 4) {
    fprintf(stderr, "incorrect number of options");
    exit(1);
  }
  for (i = 0; i < strlen(argv[1]); i++) {
    switch(argv[1][i]) {
    case ('c'):
      if (reqopt == '\0') {
	reqopt = argv[1][i];
      } else {
	fprintf(stderr, "can only choose one of c, t, and x");
	exit(1);
      }
      break;
    case ('t'):
      if (reqopt == '\0') {
	reqopt = argv[1][i];
      } else {
	fprintf(stderr, "can only choose one of c, t, and x");
	exit(1);
      }
      break;
    case ('x'):
      if (reqopt == '\0') {
	reqopt = argv[1][i];
      } else {
	fprintf(stderr, "can only choose one of c, t, and x");
	exit(1);
      }
      break;
    case ('v'):
      if (optopt[0] == 0) {
	optopt[0] = 1;
      } else {
	fprintf(stderr, "can only choose 'v' once");
	exit(1);
      }
      break;
    case ('S'):
      if (optopt[1] == 0) {
	optopt[1] = 1;
      } else {
	fprintf(stderr, "can only choose 'S' once");
	exit(1);
      }
      break;
    case ('f'):
      if (optopt[2] == 0) {
	optopt[2] = 1;
      } else {
	fprintf(stderr, "can only choose 'f' once");
	exit(1);
      }
      break;
    default:
      fprintf(stderr, "invalid option");
      exit(1);
    }
  }
  if (optopt[2] == 0) {
    fprintf(stderr, "must choose 'f' as option");
    exit(1);
  }
  if (reqopt == '\0') {
    fprintf(stderr, "must choose c, t, or x");
    exit(1);
  }
  /*parse files*/
  if (argc > FIRST_FILE) {
    files = calloc(argc - FIRST_FILE, sizeof(char *));
    /* copy files into files to pass into other functions*/
    if (files == NULL) {
      perror("calloc failed");
      exit(1);
    }
    for (i = FIRST_FILE; i < argc; i++) {
      files[i - FIRST_FILE] = malloc(strlen(argv[i]) + 2);
      if (files[i-FIRST_FILE] == NULL) {
	perror("malloc files failed");
	exit(1);
      }
      strcpy(files[i - FIRST_FILE], argv[i]);
    }
    /*numfiles = 0 otherwise*/
    numfiles = argc - FIRST_FILE;
  }
  /*follow the required option*/
  if (reqopt == 'c') {
    /*open fdtar*/
    fdtar = open(argv[2], O_RDWR | O_CREAT | O_TRUNC,
		 S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fdtar == -1) {
      perror(argv[2]);
      exit(1);
    }
    if (numfiles == 0) {
      fprintf(stderr, "No files listed to archive");
      exit(1);
    }
    c_tar(fdtar, optopt, files, numfiles);
    /* write 2 null terminated blocks */
    for (i = 0; i < BLOCK * 2; i++) {
      nullbuf[i] = '\0';
    }
    if (-1 == write(fdtar, nullbuf, BLOCK * 2)) {
      perror("write failed");
      exit(1);
    }
  } else if (reqopt == 't') {
    /*open fdtar*/
    fdtar = open(argv[2], O_RDONLY,
		 S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fdtar == -1) {
      perror(argv[2]);
      exit(1);
    }
    t_tar(fdtar, optopt, files, numfiles);
    
  } else {
    /*open fdtar*/
    fdtar = open(argv[2], O_RDONLY,
		 S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    if (fdtar == -1) {
      perror(argv[2]);
      exit(1);
    }
    x_tar(fdtar, optopt, files, numfiles);
  }
  if (argc > FIRST_FILE) {
    free(files);
  }
  close(fdtar);
  return 0;
}
/*archive listing of fdtar input file, index 0 and 1 of optopt 
represent optional flags*/
void t_tar(int fdtar, int optopt[3], char **files, int numfiles) {
  int errcheck, errcheck2, hsum = 0, i, j, mode, block_offset, mask = 1;
  header *hbuf = calloc(1, sizeof(header));
  char *fullname, *owner_group, *ptr;
  long size;
  time_t mtime;
  struct tm *time_struct;
  char perms[FULL_PERMS];
  if (hbuf == NULL) {
    perror("hbuf calloc failed");
    exit(1);
  }
  /* parse file block by block */
  while((errcheck = read(fdtar, hbuf, BLOCK)) != 0) {
    if (errcheck == -1) {
      perror("read failed");
      exit(1);
    }
    /*end of archive case */
    if (!hbuf->name[0]) {
      break;
    }
    /*skip files we don't care about*/
    errcheck = 0;
    errcheck2 = 0;
    if (*(hbuf->prefix) == '\0') {
      fullname = malloc(strlen(hbuf->name) + 1);
      if (fullname == NULL) {
	perror("fullname malloc failed");
	exit(1);
      }
      strncpy(fullname, hbuf->name, strlen(hbuf->name));
      fullname[strlen(hbuf->name)] = '\0';
      /*100 long name edge case -> NULL terminate*/
      if (strlen(hbuf->name) >= NAME_LEN) {
	fullname = realloc(fullname, NAME_LEN + 1);
	if (fullname == NULL) {
	  perror("fullname realloc failed");
	  exit(1);
	}
	fullname[NAME_LEN] = '\0';
      }
      /*combine name and prefix*/
    } else {
      /* 100 long name edge case */
      if (strlen(hbuf->name) >= NAME_LEN) {
	fullname = malloc((NAME_LEN + 1) + strlen(hbuf->prefix) + 2);
	if (fullname == NULL) {
	  perror("fullname malloc failed");
	  exit(1);
	}
	strcpy(fullname, hbuf->prefix);
        strcat(fullname, "/");
	strncat(fullname, hbuf->name, NAME_LEN);
	strcat(fullname, "\0"); /* null terminate fullname */
      } else {
	fullname = malloc(strlen(hbuf->name) + strlen(hbuf->prefix) + 2);
	if (fullname == NULL) {
	  perror("fullname malloc failed");
	  exit(1);
	}
	strcpy(fullname, hbuf->prefix);
	strcat(fullname, "/");
	strcat(fullname, hbuf->name);
	strcat(fullname, "\0"); /* null terminate fullname */
      }
    }
    /*see if we need to skip*/
    if (numfiles > 0) {
      for (i = 0; i < numfiles; i++) {
	errcheck2 = 0;
	/*check path up until files[i]*/
        for(j = 0; j < strlen(files[i]); j++) {
	  if (files[i][j] != fullname[j]) {
	    /*not the same*/
	    errcheck2 = 1;
	  }
	}
	/*if match, errcheck is TRUE*/
	if (errcheck2 != 1) {
	  errcheck = 1;
	}
      }
      /*if errcheck is not TRUE, skip this iteration of while*/
      if (errcheck != 1) {
        continue;
      }
    }
    /* bad header check */
    hsum = 0;
    for (i = 0; i < BLOCK; i++) {
      if (i >= CHKSUM_POS && i < CHKSUM_POS + CHKSUM_LEN) {
	hsum += ' ';
      } else {
	hsum += (unsigned char) ((unsigned char *)(hbuf))[i];
      }
    }
    /* handle Strict option*/
    if (optopt[1] == 1) {
      if (strncmp("00", hbuf->version, VERSION_LEN) != 0 ||
	  hsum != strtol(hbuf->chksum, &ptr, OCTAL) ||
	  strncmp("ustar",hbuf->magic, MAGIC_LEN - 1) ||
	  hbuf->magic[MAGIC_LEN-1] != '\0') {
	fprintf(stderr, "Bad header | does not comply with 'S' option");
	exit(1);
      }
    } else {
      if (hsum != strtol(hbuf->chksum, &ptr, OCTAL) ||
	  strncmp("ustar", hbuf->magic, MAGIC_LEN - 1)) {
	fprintf(stderr, "Bad header");
	exit(1);
      }
    }
    /* handle verbose option */
    if (optopt[0] == 1) {
      /*handle permissions*/
      mask = 1;
      j = 0;
      if (*(hbuf->typeflag) == '5') { 
	perms[j++] = 'd';
      } else if (*(hbuf->typeflag) == '2') { 
	perms[j++] = 'l';
      } else {
	perms[j++] = '-';
      }
      mode = strtol(hbuf->mode, &ptr, OCTAL);
      mode = mode & ALL_PERM;
      j = NUM_PERMS;
      for (i = 0; i < OCTAL_PERMS; i++) {
	if (mode & mask) {
	  perms[j--] = 'x';
	} else {
	  perms[j--] = '-';
	}
	mask <<= 1;
	if (mode & mask) {
	  perms[j--] = 'w';
	} else {
	  perms[j--] = '-';
	}
	mask <<= 1;
	if (mode & mask) {
	  perms[j--] = 'r';
	} else {
	  perms[j--] = '-';
	}
	mask <<= 1; 
      }
      /*handle owner/group*/
      /* +2 for null terms*/
      owner_group = malloc(strlen(hbuf->uname) + strlen(hbuf->gname) + 2);
      if (!owner_group) {
	perror("owner_group malloc failed");
	exit(1);
      }
      strcpy(owner_group, hbuf->uname);
      strcat(owner_group, "/");
      strcat(owner_group, hbuf->gname);
      /*handle size*/
      size = strtol(hbuf->size, &ptr, OCTAL); /* stored as long */
      /*handle Mtime*/
      mtime = strtol(hbuf->mtime, &ptr, OCTAL); /* stored as long */
      time_struct = localtime(&mtime); /* stores in time struct*/
      /* print verbose stuff */
      printf("%s %-17s %8ld %d-%02d-%02d %02d:%02d ", perms, owner_group,
	     size, BASE_YEAR+(time_struct->tm_year),
	     time_struct->tm_mon + 1, time_struct->tm_mday,
	     time_struct->tm_hour, time_struct->tm_min);
    }
    /* reset for when not verbose */
    size = strtol(hbuf->size, &ptr, OCTAL);
    /*print name*/
    printf("%s\n", fullname);
    free(fullname);
    /*lseek to next header by block*/
    if (size > 0) { /* move past file contents*/
      block_offset = 1 + (size / BLOCK); /* file len in blocks */
      /* lseek num blocks */
      if (lseek(fdtar, BLOCK * block_offset, SEEK_CUR) == -1) {
	perror("lseek failed");
	exit(1);
      }
    }
  } /* end while */
}

/* prints filenames if verbose and lets c_tar_helper deal with files*/
void c_tar(int fdtar, int optopt[3], char **files, int numfiles) {
  int i;
  struct stat st;
  for (i = 0; i < numfiles; i++) {
    /* if verbose */
    if (stat(files[i], &st) == -1) {
      perror(files[i]);
      continue;
    }
    if (optopt[0] == 1) {
      printf("%s", files[i]);
      if (S_ISDIR(st.st_mode)) {
	printf("/");
      }
      printf("\n");
    }
    c_tar_helper(files[i], optopt, fdtar);
  }
}

/*recursively archives input, accepts file as path to
a file, directory, or symlink. optopt[0]: vflag, optopt[1]: Sflag*/
void c_tar_helper(char *file, int optopt[3], int fdtar) {
  int fsize, fd, i;
  struct stat st;
  struct stat stdir;
  struct stat stdirent;
  char *content = NULL;
  char *dirname;
  char *direntpath;
  struct dirent *curdirent;
  DIR *curdir;
  if (stat(file, &st) == -1) { /* stat for symlink too */
    perror(file);
    return;
  }
  if (S_ISDIR(st.st_mode)) { /* directory */
    /* build header for dir */
    build_header(fdtar, file, optopt);
    /*set dirname*/
    dirname = malloc(strlen(file) + 2);
    if (dirname == NULL) {
      perror("dirname malloc");
      exit(1);
    }
    strcpy(dirname, file);
    if (dirname[strlen(dirname) - 1] != '/') {
      strcat(dirname, "/");
    }
    if (stat(dirname, &stdir) == -1) {
      perror(dirname);
      return;
    }
    if ((curdir = opendir(dirname)) == NULL) {
      perror(dirname);
      return;
    }
    while ((curdirent = readdir(curdir)) != NULL) {
      /*check if curdirent is curdir*/
      direntpath = malloc(strlen(dirname) + strlen(curdirent->d_name) + 2);
      if (direntpath == NULL) {
	perror("direntpath malloc failed");
	exit(1);
      }
      strcpy(direntpath, dirname);
      strcat(direntpath, curdirent->d_name); /* cur path */
      if (stat(direntpath, &stdirent) == -1) {
	perror(direntpath);
	return;
      }
      if (stdir.st_ino==stdirent.st_ino && stdir.st_dev==stdirent.st_dev) {
	free(direntpath);
	continue;
      }
      if (strcmp(curdirent->d_name, "..") == 0) {
	free(direntpath);
	continue;
      }
      if (optopt[0] == 1) { /* verbose */
	printf("%s\n", direntpath);
      }
      c_tar_helper(direntpath, optopt, fdtar); /* recurse */
    } /* end while */
    closedir(curdir);
    free(dirname);
  } else if (S_ISREG(st.st_mode)) { /* regular file */
    if ((fsize = build_header(fdtar, file, optopt)) == -1) {
      return;
    }
    if (-1 == (fd = open(file, O_RDONLY))) {
      perror(file);
      return;
    }
    /* write file contents */
    content = malloc(BLOCK);
    if (content == NULL) {
      perror(file);
      return;
    }
    for (i = 0; i < fsize / BLOCK; i++) {
      if (read(fd, content, BLOCK) == -1) {
	perror(file);
	return;
      }
      if (write(fdtar, content, BLOCK) == -1) {
	perror(file);
	return;
      }
    }
    free(content);
    content = calloc(1, BLOCK);
    if (content == NULL) {
      perror("malloc failed");
      return;
    }
    if (read(fd, content, fsize % BLOCK) == -1) {
      perror(file);
      return;
    }
    if (write(fdtar, content, BLOCK) == -1) {
      perror(file);
      return;
    }
    free(content);
  } else if (S_ISLNK(st.st_mode)) {
    if (build_header(fdtar, file, optopt) == -1) {
      return;
    }
  } else {
    fprintf(stderr, "Invalid file option");
    return;
  }
  free(file);
}
/* Takes in file, directory, or symlink in path and creates a tar
header, writes it to fdtar. Returns size on success, -1 on failure
optopt[0] : vflag, optopt[1] : Sflag*/
int build_header(int fdtar, char *path, int optopt[3]) {
  header *hbuf = calloc(1, sizeof(header)); /* callocs 1 header */
  struct stat sbuf;
  int errcheck, i = 0, hsum = 0;
  char *cur;
  char *ptr;
  int size;
  if (hbuf == NULL) {
    perror("hbuf calloc failed");
    exit(1);
  }
  /* stat of path stored in sbuf */
  errcheck = lstat(path, &sbuf);
  if (errcheck == -1) {
    perror("stat failed");
    return -1;
  }
  /* Strict check for ID's */
  if (optopt[1] == 1) {
    if (sbuf.st_uid > MAX_ID || sbuf.st_gid > MAX_ID) {
      free(hbuf);
      return -1;
    }
  } 
  if (strlen(path) > NAME_LEN) { /* overflow to prefix */
    cur = path + (strlen(path) - OVERFLOW); 
    i = OVERFLOW;
    while ((*cur) != '/' && i > 0) {
      i--;
      cur++;
    }
    if ((*cur) != '/') {
      fprintf(stderr, "%s: file could not be archived", path);
      return -1;
    }
    strcpy(hbuf->name, ++cur);
    strncpy(hbuf->prefix, path, strlen(path) - i); 
  } else {
    strcpy(hbuf->name, path); /* dw about nul-term bc of calloc */
  }
  sprintf(hbuf->mode, "%07o", sbuf.st_mode & ALL_MODEPERM); 
  sprintf(hbuf->uid, "%07o", sbuf.st_uid);
  sprintf(hbuf->gid, "%07o", sbuf.st_gid);
  if (S_ISREG(sbuf.st_mode)) { /* reg file size */
    sprintf(hbuf->size, "%011o", (int) sbuf.st_size);
  } else { /* sym link or dir size = 0 */
    sprintf(hbuf->size, "%011o", 0);
  }
  sprintf(hbuf->mtime, "%07o", (int) sbuf.st_mtime);
  /* chksum at end, this is just placeholder */
  if (S_ISREG(sbuf.st_mode)) {
    *(hbuf->typeflag) = '0';
  } else if (S_ISLNK(sbuf.st_mode)) { /* only change linkname if S_ISLNK */
    *(hbuf->typeflag) = '2';
    readlink(path, hbuf->linkname, LINKNAME_LEN);
  } else if (S_ISDIR(sbuf.st_mode)) {
    *(hbuf->typeflag) = '5';
    hbuf->name[strlen(hbuf->name)] = '/';
  } else {
    fprintf(stderr, "must be a symlink, dir, or file");
    return -1;
  }
  strcpy(hbuf->magic, "ustar");
  strcpy(hbuf->version, "00");
  strcpy(hbuf->uname, (getpwuid(sbuf.st_uid))->pw_name);
  strcpy(hbuf->gname, (getgrgid(sbuf.st_gid))->gr_name);
  /*chksum */
  for (i = 0; i < BLOCK; i++) {
    if (i >= CHKSUM_POS && i < CHKSUM_POS + CHKSUM_LEN) {
      hsum += ' ';
    } else {
      hsum += (unsigned char)((unsigned char *)(hbuf))[i];
    }
  }
  sprintf(hbuf->chksum, "%07o", hsum);
  /* dev major: N/A */
  /* dev minor: N/A */
  errcheck = write(fdtar, hbuf, BLOCK);
  if (errcheck == -1) {
    perror("write header failed");
    exit(1);
  }
  size = strtol(hbuf->size, &ptr, OCTAL);
  free(hbuf);
  return size;
}

/* Extract from fdtar, either files listed in files, or all if numfiles
   equals 0. Creates directories, paths, and symlinks in appropriate
locations to place archive contents back in file system*/
void x_tar(int fdtar, int optopt[3], char **files, int numfiles) {
  int errcheck, hsum = 0, size, i, errcheck2, j;
  header *hbuf = malloc(sizeof(header));
  char *fullname, *ptr;
  time_t mtime;
  char *content = NULL;
  char *cur, *start, *tempdir;
  mode_t mode_perms;
  int new_fd;
  struct utimbuf *new_time = NULL;
  if (hbuf == NULL) {
    perror("hbuf malloc failed");
    exit(1);
  }
  /* parse file block by block */
  while((errcheck = read(fdtar, hbuf, BLOCK)) != 0) {
    if (errcheck == -1) {
      perror("read failed");
      exit(1);
    }
    /*end of archive case */
    if (!hbuf->name[0]) {
      break;
    }
    /*skip files we don't care about*/
    errcheck = 0;
    errcheck2 = 0;
    hsum = 0;
    if (hbuf->prefix[0] == '\0') {
      fullname = malloc(strlen(hbuf->name) + 1);
      if (fullname == NULL) {
	perror("fullname malloc failed");
	exit(1);
      }
      strcpy(fullname, hbuf->name);
      fullname[strlen(hbuf->name)] = '\0';
      /*100 long name edge case -> NULL terminate*/
      if (strlen(hbuf->name) >= NAME_LEN) {
	fullname = realloc(fullname, NAME_LEN + 1);
	if (fullname == NULL) {
	  perror("fullname realloc failed");
	  exit(1);
	}
	fullname[NAME_LEN] = '\0';
      }
      /*combine name and prefix*/
    } else {
      /* 100 long name edge case */
      if (strlen(hbuf->name) >= NAME_LEN) {
	fullname = malloc((NAME_LEN + 1) + strlen(hbuf->prefix) + 1);
	if (fullname == NULL) {
	  perror("fullname malloc failed");
	  exit(1);
	}
	strcpy(fullname, hbuf->prefix);
	strcat(fullname, "/");
	strncat(fullname, hbuf->name, NAME_LEN);
	strcat(fullname, "\0"); /* null terminate fullname */
      } else {
	fullname = malloc(strlen(hbuf->name) + strlen(hbuf->prefix) + 2);
	if (fullname == NULL) {
	  perror("fullname malloc failed");
	  exit(1);
	}
	strcpy(fullname, hbuf->prefix);
	strcat(fullname, "/");
        strcat(fullname, hbuf->name);
	strcat(fullname, "\0"); /* null terminate fullname */
      }
    }
    if (numfiles > 0) {
      for (i = 0; i < numfiles; i++) {
	errcheck2 = 0;
	/*check path up until files[i]*/
        for(j = 0; j < strlen(files[i]); j++) {
	  if (files[i][j] != fullname[j]) {
	    /*not the same*/
	    errcheck2 = 1;
	  }
	}
	/*if match, errcheck is TRUE*/
	if (errcheck2 != 1) {
	  errcheck = 1;
	}
      }
      /*if errcheck is not TRUE, skip this iteration of while*/
      if (errcheck != 1) {
        continue;
      }
    }
    /* bad header check */
    for (i = 0; i < BLOCK; i++) {
      if (i >= CHKSUM_POS && i < CHKSUM_POS + CHKSUM_LEN) {
	hsum += ' ';
      } else {
	hsum += (unsigned char) ((unsigned char *)(hbuf))[i];
      }
    }
    /* handle Strict option*/
    if (optopt[1] == 1) {
      if (strncmp("00", hbuf->version, VERSION_LEN) != 0 ||
	  hsum != strtol(hbuf->chksum, &ptr, OCTAL) ||
	  strncmp("ustar",hbuf->magic, MAGIC_LEN - 1) ||
	  hbuf->magic[MAGIC_LEN-1] != '\0') {
	fprintf(stderr, "Bad header | does not comply with 'S' option");
	exit(1);
      }
    } else {
      if (hsum != strtol(hbuf->chksum, &ptr, OCTAL) ||
	  strncmp("ustar", hbuf->magic, MAGIC_LEN - 1)) {
	fprintf(stderr, "Bad header");
	exit(1);
      }
    }
    /* handle verbose option */
    if (optopt[0] == 1) {
      printf("%s\n", fullname);
    }
    /*create permissions*/
    mode_perms = (strtol(hbuf->mode, &ptr, OCTAL) &
		  (S_IRWXU|S_IRWXG|S_IRWXO));
	      
    /*finish extraction*/
      /*if given path, create directories to get to file*/
    if (strchr(fullname, '/') != NULL) {
      cur = fullname;
      start = fullname;
      while (*cur != '\0') {
	if (*cur == '/') {
	  /* 2 for '/' and '\0' */
	  tempdir = malloc(cur - start + 2);
	  if (tempdir == NULL) {
	    perror(tempdir);
	    exit(1);
	  }
	  strncpy(tempdir, start, cur - start + 1);
	  tempdir[cur-start+1] = '\0';
	  /*don't errcheck mkdir bc will try to create every dir,
	    not an error if we already made it*/
	  mkdir(tempdir, ALL_PERM);
	  free(tempdir);
	}
	cur++;
      }
    }
    if (*(hbuf->typeflag) == '2') { /* symlink */
      symlink(hbuf->linkname, fullname);
    } else if (*(hbuf->typeflag) == '5') { /* dir */
      mkdir(fullname, mode_perms); /* need mode_t 2nd arg for this */
    } else if (*(hbuf->typeflag) == '0' || *(hbuf->typeflag) == '\0') {
      
      if ((new_fd = 
	   open(fullname, O_WRONLY |O_TRUNC | O_CREAT, mode_perms)) == -1) {
	perror(fullname);
	continue;
      }
      /* restore mtime */
      mtime = strtol(hbuf->mtime, &ptr, OCTAL);
      new_time = malloc(sizeof(struct utimbuf));
      if (new_time == NULL) {
	perror("new_time malloc failed");
	exit(1);
      }
      new_time->modtime = mtime;
      new_time->actime = mtime;
      if ((utime(fullname, new_time)) == -1) {
	perror(fullname);
	continue;
      }
      free(new_time);
      /* write file contents */
      size = strtol(hbuf->size, &ptr, OCTAL);
      content = malloc(BLOCK);
      if (content == NULL) {
        perror(fullname);
        continue;
      }
      for (i = 0; i < size / BLOCK; i++) {
        if (read(fdtar, content, BLOCK) == -1) {
	  perror(fullname);
	  continue;
        }
        if (write(new_fd, content, BLOCK) == -1) {
	  perror(fullname);
	  continue;
        }
      }
      free(content);
      content = malloc(size % BLOCK);
      if (content == NULL) {
        perror("malloc failed");
        continue;
      }
      if (read(fdtar, content, size % BLOCK) == -1) {
        perror(fullname);
        continue;
      }
      if (write(new_fd, content, size % BLOCK) == -1) {
        perror(fullname);
        continue;
      }
      lseek(fdtar, BLOCK - (size % BLOCK), SEEK_CUR);
      free(content);
    } else {
      fprintf(stderr, "must be a file, dir, or symlink");
      continue;
    } 
    free(fullname);
  }/* end while*/
}
