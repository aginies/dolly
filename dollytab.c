#include "dollytab.h"

/* Parses the config-file. The path to the file is given in dollytab */
void parse_dollytab(FILE *df) {
  char str[256];
  char *sp, *sp2;
  unsigned int i;
  int me = -2;
  int hadmynodename = 0; /* Did this node already get its node name? */
  char *mnname = NULL;

  /* Read the parameters... */
  
  /* Is there a MYNODENAME? If so, use that for the nodename. There
     won't be a line like this on the server node, but there should always
     be a first line of some kind
  */
  mnname = getenv("MYNODENAME");
  if(mnname != NULL) {
    (void)strcpy(myhostname, mnname);
    (void)fprintf(stderr,
		  "\nAssigned nodename %s from MYNODENAME environment variable\n",
		  myhostname);
    hadmynodename = 1;
  }
  mnname = getenv("HOST");
  if(mnname != NULL) {
    (void)strcpy(myhostname, mnname);
    (void)fprintf(stderr,
		  "\nAssigned nodename %s from HOST environment variable\n",
		  myhostname);
    hadmynodename = 1;
  }
  
  /* First we want to know the input filename */
  if(!dummy_mode) {
    if(fgets(str, 256, df) == NULL) {
      fprintf(stderr, "errno = %d\n", errno);
      perror("fgets for infile");
      exit(1);
    }
    sp2 = str;
    if(strncmp("compressed ", sp2, 11) == 0) {
      compressed_in = 1;
      sp2 += 11;
    }
    if(strncmp("infile ", sp2, 7) != 0) {
      fprintf(stderr, "Missing 'infile ' in config-file.\n");
      exit(1);
    }
    sp2 += 7;
    if(sp2[strlen(sp2)-1] == '\n') {
      sp2[strlen(sp2)-1] = '\0';
    }
    if((sp = strchr(sp2, ' ')) == NULL) {
      sp = sp2 + strlen(sp2);
    }
    if(compressed_in && (strncmp(&sp2[sp - sp2 - 3], ".gz", 3) != 0)) {
      char tmp_str[256];
      strncpy(tmp_str, sp2, sp - sp2);
      tmp_str[sp - sp2] = '\0';
      fprintf(stderr,
	      "WARNING: Compressed outfile '%s' doesn't end with '.gz'!\n",
	      tmp_str);
    }
    strncpy(infile, sp2, sp - sp2);
    sp++;
    if(strcmp(sp, "split") == 0) {
      input_split = 1;
    }
    
    /* Then we want to know the output filename */
    if(fgets(str, 256, df) == NULL) {
      perror("fgets for outfile");
      exit(1);
    }
    sp2 = str;
    if(strncmp("compressed ", sp2, 11) == 0) {
      compressed_out = 1;
      sp2 += 11;
    }
    if(strncmp("outfile ", sp2, 8) != 0) {
      fprintf(stderr, "Missing 'outfile ' in config-file.\n");
      exit(1);
    }
    sp2 += 8;
    if(sp2[strlen(sp2)-1] == '\n') {
      sp2[strlen(sp2)-1] = '\0';
    }
    if((sp = strchr(sp2, ' ')) == NULL) {
      sp = sp2 + strlen(sp2);
    }
    if(compressed_out && (strncmp(&sp2[sp - sp2 - 3], ".gz", 3) != 0)) {
      char tmp_str[256];
      strncpy(tmp_str, sp2, sp - sp2);
      tmp_str[sp - sp2] = '\0';
      fprintf(stderr,
	      "WARNING: Compressed outfile '%s' doesn't end with '.gz'!\n",
	      tmp_str);
    }
    strncpy(outfile, sp2, sp - sp2);
    sp++;
    if(strncmp(sp, "split ", 6) == 0) {
      unsigned long long size = 0;
      char *s = sp+6;
      while(isdigit(*s)) {
	size *= 10LL;
	size += (unsigned long long)(*s - '0');
	s++;
      }
      switch(*s) {
      case 'T': size *= 1024LL*1024LL*1024LL*1024LL; break;
      case 'G': size *= 1024LL*1024LL*1024LL; break;
      case 'M': size *= 1024LL*1024LL; break;
      case 'k': size *= 1024LL; break;
      default:
	fprintf(stderr, "Unknown multiplier '%c' for split size.\n", *s);
	break;
      }
      output_split = size;
      str[sp - str - 1] = '\0';
    }
  } else {
    /* Dummy Mode: Get the size of to transfer data */ 
    if(fgets(str, 256, df) == NULL) {
      perror("fgets on dummy");
      exit(1);
    }
    if(strncmp("dummy ", str, 6) == 0) {
      if(str[strlen(str)-1] == '\n') {
	str[strlen(str)-1] = '\0';
      }
      sp = strchr(str, ' ');
      if(sp == NULL) {
        fprintf(stderr, "Error dummy line.\n");
      }
      if(atoi(sp + 1) == 0) {
	exitloop = 1;
      }
      dummysize = atoi(sp + 1);
      dummysize = 1024*1024*dummysize;
    } else if(strcmp("dummy\n", str) == 0) {
      dummysize = 0;
    }
  }
  
  /* Get the optional TCPMaxSeg size */ 
  if(fgets(str, 256, df) == NULL) {
    perror("fgets on segsize or fanout");
    exit(1);
  }
  if(strncmp("segsize ", str, 8) == 0) {
    if(str[strlen(str)-1] == '\n') {
      str[strlen(str)-1] = '\0';
    }
    sp = strchr(str, ' ');
    if(sp == NULL) {
      fprintf(stderr, "Error segsize line.\n");
      exit(1);
    }
    segsize = atoi(sp + 1);
    if(fgets(str, 256, df) == NULL) {
      perror("fgets after segsize");
      exit(1);
    }
  }

  /* Get the optional extra network interfaces */
  /* Form of the line: add <nr_extra_interfaces>:<postfix>{:<postfix>} */
  if((strncmp("add ", str, 4) == 0) || (strncmp("add2 ", str, 5) == 0)) {
    char *s1, *s2;
    int max = 0, i;

    if(strncmp("add ", str, 4) == 0) {
      add_mode = 1;
    }
    if(strncmp("add2 ", str, 5) == 0) {
      add_mode = 2;
    }
    if(add_mode == 0) {
      fprintf(stderr,
	      "Bad add_mode: Choose 'add' or 'add2' in config-file.\n");
      exit(1);
    }
    
    s1 = str + 4;
    s2 = s1;
    while((*s2 != ':' && *s2 != '\n' && *s2 != 0)) s2++;
    if(*s2 == 0) {
      fprintf(stderr, "Error in add line: First colon missing.\n");
      exit(1);
    }
    *s2 = 0;
    max = atoi(s1);
    if(max < 0) {
      fprintf(stderr, "Error in add line: negative number.\n");
      exit(1);
    }
    if(max >= MAXFANOUT) {
      fprintf(stderr, "Error in add line: Number larger than MAXFANOUT.\n");
      exit(1);
    }
    if (max==0) {
      /* change names of primary interface */
      add_primary = 1;
      s1 = s2 + 1;
      s2++;
      while((*s2 != ':' && *s2 != '\n' && *s2 != 0)) s2++;
      if(*s2 == 0) {
	fprintf(stderr, "Error in add line: Preliminary end.\n");
	exit(1);
      }
      *s2 = 0;
      strcpy(add_name[0], s1);
    } else {
      for(i = 0; i < max; i++) {
	s1 = s2 + 1;
	s2++;
	while((*s2 != ':' && *s2 != '\n' && *s2 != 0)) s2++;
	if(*s2 == 0) {
	  fprintf(stderr, "Error in add line: Preliminary end.\n");
	  exit(1);
	}
	*s2 = 0;
	strcpy(add_name[i], s1);
      }
    }
    add_nr = max;
    if(fgets(str, 256, df) == NULL) {
      perror("fgets after add");
      exit(1);
    }
  }
  
  /* Get the fanout (1 = linear list, 2 = binary tree, ... */
  if(strncmp("fanout ", str, 7) == 0) {
    if(str[strlen(str)-1] == '\n') {
      str[strlen(str)-1] = '\0';
    }
    sp = strchr(str, ' ');
    if(sp == NULL) {
      fprintf(stderr, "Error in fanout line.\n");
      exit(1);
    }
    fanout = atoi(sp + 1);
    if(fgets(str, 256, df) == NULL) {
      perror("fgets after fanout");
      exit(1);
    }
  }

  /*
   * The parameter "hyphennormal" means that the hyphen '-' is treated
   * as any other character. The default is to treat it as a separator
   * between base hostname and the number of the node.
   */
  if(strncmp("hyphennormal", str, 12) == 0) {
    hyphennormal = 1;
    if(fgets(str, 256, df) == NULL) {
      perror("fgets after hyphennormal");
      exit(1);
    }
  }
  if(strncmp("hypheninterface", str, 12) == 0) {
    hyphennormal = 1;
    if(fgets(str, 256, df) == NULL) {
      perror("fgets after hypheninterface");
      exit(1);
    }
  }
  
  /* Get our own hostname */
  if(!hadmynodename){
    if(gethostname(myhostname, 63) == -1) {
      perror("gethostname");
    }
  }
  /* Get the server's name. */
  if(strncmp("server ", str, 7) != 0) {
    fprintf(stderr, "Missing 'server' in config-file.\n");
    exit(1);
  }
  if(str[strlen(str)-1] == '\n') {
    str[strlen(str)-1] = '\0';
  }
  sp = strchr(str, ' ');
  if(sp == NULL) {
    fprintf(stderr, "Error in firstclient line.\n");
    exit(1);
  }
  strncpy(servername, sp+1, strlen(sp));

  /* 
     disgusting hack to make -S work.  If the server name
     specified in the file is wrong bad things will probably happen!
  */
  
  if(meserver == 2){
    (void) strcpy(myhostname,servername);
    meserver = 1;
  }

  if(!(meserver ^ (strcmp(servername, myhostname) != 0))) {
    fprintf(stderr,
	    "Commandline parameter -s and config-file disagree on server!\n");
    fprintf(stderr, "  My name is '%s'.\n", myhostname);
    fprintf(stderr, "  The config-file specifies '%s'.\n", servername);
    exit(1);
  }
  
  /* We need to know the FIRST host of the ring. */
  /* (Do we still need the firstclient?)         */
  if(fgets(str, 256, df) == NULL) {
    perror("fgets for firstclient");
    exit(1);
  }
  if(strncmp("firstclient ", str, 12) != 0) {
    fprintf(stderr, "Missing 'firstclient ' in config-file.\n");
    exit(1);
  }
  if(str[strlen(str)-1] == '\n') {
    str[strlen(str)-1] = '\0';
  }
  sp = strchr(str, ' ');
  if(sp == NULL) {
    fprintf(stderr, "Error in firstclient line.\n");
    exit(1);
  }

  /* We need to know the LAST host of the ring. */
  if(fgets(str, 256, df) == NULL) {
    perror("fgets for lastclient");
    exit(1);
  }
  if(strncmp("lastclient ", str, 11) != 0) {
    fprintf(stderr, "Missing 'lastclient ' in config-file.\n");
    exit(1);
  }
  if(str[strlen(str)-1] == '\n') {
    str[strlen(str)-1] = '\0';
  }
  sp = strchr(str, ' ');
  if(sp == NULL) {
    fprintf(stderr, "Error in lastclient line.\n");
    exit(1);
  }
  if(strcmp(myhostname, sp+1) == 0) {
    melast = 1;
  } else {
    melast = 0;
  }

  /* Read in all the participating hosts. */
  if(fgets(str, 256, df) == NULL) {
    perror("fgets for clients");
    exit(1);
  }
  if(strncmp("clients ", str, 8) != 0) {
    fprintf(stderr, "Missing 'clients ' in config-file.\n");
    exit(1);
  }
  hostnr = atoi(str+8);
  if((hostnr < 1) || (hostnr > 10000)) {
    fprintf(stderr, "I think %d numbers of hosts doesn't make much sense.\n",
	    hostnr);
    exit(1);
  }
  hostring = (char **)malloc(hostnr * sizeof(char *));
  for(i = 0; i < hostnr; i++) {
    if(fgets(str, 256, df) == NULL) {
      char errstr[256];
      sprintf(errstr, "gets for host %d", i);
      perror(errstr);
      exit(1);
    }
    if(str[strlen(str)-1] == '\n') {
      str[strlen(str)-1] = '\0';
    }
    hostring[i] = (char *)malloc(strlen(str)+1);
    strcpy(hostring[i], str);

    /* Try to find next host in ring */
    /* if(strncmp(hostring[i], myhostname, strlen(myhostname)) == 0) { */
    if(strcmp(hostring[i], myhostname) == 0) {
      me = i;
    } else if(!hyphennormal) {
      /* Check if the hostname is correct, but a different interface is used */
      if((sp = strchr(hostring[i], '-')) != NULL) {
        if(strncmp(hostring[i], myhostname, sp - hostring[i]) == 0) {
          me = i;
        }
      }
    }
  }

  if(!meserver && (me == -2)) {
    fprintf(stderr, "Couldn't find myself '%s' in hostlist.\n",myhostname);
    exit(1);
  }
  
  /* Build up topology */
  nr_childs = 0;
  
  for(i = 0; i < fanout; i++) {
    if(meserver) {
      if(i + 1 <= hostnr) {
        nexthosts[i] = i;
        nr_childs++;
      }
    } else {
      if((me + 1) * fanout + 1 + i <= hostnr) {
        nexthosts[i] = (me + 1) * fanout + i;
        nr_childs++;
      }
    }
  }
  /* In a tree, we might have multiple last machines. */
  if(nr_childs == 0) {
    melast = 1;
  }

  /* Did we reach the end? */
  if(fgets(str, 256, df) == NULL) {
    perror("fgets for endconfig");
    exit(1);
  }
  if(strncmp("endconfig", str, 9) != 0) {
    fprintf(stderr, "Missing 'endconfig' in config-file.\n");
    exit(1);
  }
  if((nr_childs > 1) && (add_nr > 0)) {
    fprintf(stderr, "Currently dolly supports either a fanout > 1\n"
	    "OR the use of extra network links, but not both.\n");
    exit(1);
  }
  if(flag_v) {
    fprintf(stderr, "done.\n");
  }
  if(flag_v) {
    if(!meserver) {
      fprintf(stderr, "I'm number %d\n", me);
    }
  }
}

/*
 * Clients read the parameters from the control-socket.
 * As they are already parsed from the config-file by the server,
 * we don't do much error-checking here.
 */
void getparams(int f) {
  size_t readsize;
  ssize_t writesize;
  int fd, ret;
  FILE *dolly_df = NULL;
  char tmpfile[32] = "/tmp/dollytmpXXXXXX";

  if(flag_v) {
    fprintf(stderr, "Trying to read parameters...");
    fflush(stderr);
  }
  dollybuf = (char *)malloc(T_B_SIZE);
  if(dollybuf == NULL) {
    fprintf(stderr, "Couldn't get memory for dollybuf.\n");
    exit(1);
  }

  readsize = 0;
  do {
    ret = read(f, dollybuf + readsize, T_B_SIZE);
    if(ret == -1) {
      perror("read in getparams while");
      exit(1);
    } else if((unsigned int) ret == T_B_SIZE) {  /* This will probably not happen... */
      fprintf(stderr, "Ups, the transmitted config-file seems to long.\n"
	      "Please rewrite dolly.\n");
      exit(1);
    }
    readsize += ret;
  } while(ret == 1448);
  dollybufsize = readsize;

  /* Write everything to a file so we can use parse_dollytab(FILE *)
     afterwards.  */
#if 0
  tmpfile = tmpnam(NULL);
  fd = open(tmpfile, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
  fd = mkstemp(tmpfile);
  if(fd == -1) {
    perror("Opening temporary file 'tmp' in getparams");
    exit(1);
  }
  writesize = write(fd, dollybuf, readsize);
  if(writesize == -1) {
    perror("Writing temporary file 'tmp' in getparams");
    exit(1);
  }
  dolly_df = fdopen(fd, "r");
  if(dolly_df == NULL) {
    perror("fdopen in getparams");
    exit(1);
  }
  rewind(dolly_df);
  if(flag_v) {
    fprintf(stderr, "done.\n");
  }
  if(flag_v) {
    fprintf(stderr, "Parsing parameters...");
    fflush(stderr);
  }
  parse_dollytab(dolly_df); 
  fclose(dolly_df);
  close(fd);
  unlink(tmpfile);
}

