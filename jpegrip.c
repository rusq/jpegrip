
/*
 * JPEGRIP.C
 *
 * JPEG the Ripper
 * Programme for ripping out JPEG files out of a mess
 *
 * Copyright 2005 by Rustam Gilyazov (msdos.sys@bk.ru)
 * May be distributed under the GNU General Public License
 */

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 8192


// our lovely markers
#define SOJ 0xd8 //start of jpeg image
#define EOI 0xd9 //end of image


unsigned long int global_counter; // for extraction

int suspect;
int found; // in jpeg_file
int found_start; // found start
int found_end; // found end (ready for extraction);
int verbose;

//char* jname;

int findjpeg(const unsigned char* buffer, int bread);
int extract(off64_t start, off64_t end,const int infile);


int main(int argc, char **argv)
{
  int file;
  int bread; // Number of bytes read
  off64_t foffset = 0;
  off64_t startjpeg = 0;
  off64_t endjpeg = 0;

  unsigned char* buffer;
  int find_result;

  // Initializing ----------------------------------------
  global_counter = 0;
  verbose=0;
  suspect=0;

  buffer=(unsigned char*) malloc(BUF_SIZE);
  // -----------------------------------------------------  

  // Parsing parameters ----------------------------------
  if(argc <= 1) {
    printf("Usage: %s <mess.ext> [-v|-vv]\n",argv[0]);
    printf("\t-v\t- verbose mode\n\t-vv\t- paranoid verbose mode (%-6)\n");
    exit(1);
  }

  if(argc == 3)
    if (strncmp(argv[2],"-v",2) == 0) {
	verbose++;
      if(strncmp(argv[2],"-vv",3) == 0) {
	verbose++;
	fprintf(stderr,"\%-6 PPP: Paranoid mode activated.\n");
      }
    }
  // (Parsing parameters) --------------------------------

  fprintf(stderr,"Ripping file: %s...\n",argv[1]);

  if((file=open(argv[1],O_RDONLY|O_LARGEFILE))==-1) {
    perror(":-( open() Fucked up");
    free(buffer);
    abort();
  }

  if(verbose)
    printf(":-) File \"%s\" opened successfully.\n",argv[1]);
  // Main cycle ----------------------------------------------------
  do {
    bread = read(file,buffer,BUF_SIZE);
    if(bread == -1) {
      free(buffer);
      perror(":-( Read() fucked up");
      abort();
    }

    if(bread == 0) {
      fprintf(stderr,":-) EOF reached. Extraction finished.\n");
      if(global_counter)
	fprintf(stderr,":-) %lu JPEG files extracted.\n",global_counter);
      else
	fprintf(stderr,":-/ No JPEG files found in this file.\n");
      break;
    }

    find_result = findjpeg(buffer,bread);

    if((find_result) == -1) // Nothing found
      continue;
    else {
      foffset = lseek64(file,-find_result,SEEK_CUR);
      if(verbose>1)
	perror("\%-6 seek stat");
      if(found_start) {
	startjpeg = foffset - 1; //!!!!!!!!!!
	if(verbose>1)
	  fprintf(stderr,"%-6 START: Found jpeg_start @ 0x%.8X\n",startjpeg);
	found_start = 0;
      }
      else if(found_end) {
	endjpeg = foffset+1;
	if(verbose>1)
	  fprintf(stderr,"%-6 --- END: Found jpeg_end @ 0x%.8X\n",endjpeg - 1);
	found_end = 0;
	found = 0;
	if(extract(startjpeg,endjpeg,file)!=0) {
	  free(buffer);
	  close(file);
	  abort();
	}
      }

    }
		    
  } while(bread > 0);
  // (Main cycle) --------------------------------------------------

  close(file);
  
  free(buffer);

  return 0;
}

int findjpeg(const unsigned char *buffer,int bread)
{

  int ret_val = -1;

  int i=0;

  while (i <= (bread - 1)) {
    if (*buffer != 0xff) {
      i++;
      buffer++;
      continue;
    }
    i++; buffer++;
    if( i >= bread) {
      ret_val = 1;
      break;
    }
    if(!found) {
      if((!suspect) && (*buffer == 0xd8)) {
	suspect = 1;

	if(i >= (bread - 5)) {
	  suspect=0;
	  ret_val = 5;
	  break;
	}
	if((*(buffer+1) == 0xff) && \
	   (*(buffer+2) == 0xe0) && \
	   (*(buffer+3) == 0x00) && \
	   (*(buffer+4) == 0x10 )) {
	  found_start = found = 1;
	  suspect =0;
	  ret_val = bread - i;
	  break;
	}
	suspect = 0;
	break;
      }
    }
    if(found) {
      while (*buffer == 0xff) {
	i++;
	buffer++;
      }
      if(*buffer == 0xd9) {
	found_end = 1;
	ret_val = bread - i;
	break;
      }
      if(*buffer == 0xd8) {
	found_start = 1;
	ret_val = bread - i;
	break;
      }
    }
    buffer++; i++;
  }

  return ret_val;
}
	 
int extract(off64_t start, off64_t end, const int infile)
{
  int f;
  char* s;

  int numbuffs,remainer;
  char* buffer;
  int i;

  off64_t stored_pos;
  
  

  if((s = (char *) malloc(16))==NULL) {
    fprintf(stderr,">:-( extract(): Memory allocation error\n");
    return 1;
  }
  if((buffer=(char*) malloc(BUF_SIZE))==NULL) {
    fprintf(stderr,">:-( extract(): Buffer allocation error\n");
    free(s);
    return 1;
  }

  if(verbose>1)
    fprintf(stderr,"\%-6\tRip size: %d bytes\n",end-start);

  sprintf(s,"%s%08lu.jpg","jpg",global_counter);
  if(verbose)
    fprintf(stderr,"8-) Writing file: `%s'\n",s);

  numbuffs = (int) (end-start)/BUF_SIZE;
  remainer = (int) (end-start)%BUF_SIZE;

  if(verbose>1)
    fprintf(stderr,"\%-6\t\tnumbuffs=%d,remainer=%d\n",numbuffs,remainer);

  if((f=open(s,O_WRONLY|O_CREAT|O_TRUNC,S_IREAD|S_IWRITE|S_IRGRP|S_IROTH))==-1) {
    perror(":-( extract(): open()");
    free(buffer);free(s);
    return 1;
  }

  stored_pos=lseek64(infile,0,SEEK_CUR); //Savig file position
  // Saving ------------------------------------------------
  if(verbose>1)
    fprintf(stderr,"%-6\t\tWriting numbuffs...\n");
  lseek(infile,start,SEEK_SET);
  if(numbuffs) {
    for(i = 0; i <= numbuffs-1; i++) {
      if((read(infile, buffer, BUF_SIZE))==-1) {
	perror(":-( extract(): read()");
	close(f);free(buffer);free(s);
	return 1;
      }
      if((write(f,buffer,BUF_SIZE))==-1) {
	perror(":-( extract(): write()");
	close(f);free(buffer);free(s);
	return 1;
      }
    }
  }

  if(verbose>1)
    fprintf(stderr,"%-6\t\tWriting remainer...");
  read(infile,buffer,remainer);
  write(f,buffer,remainer);
  close(f);
  // (Saving) ----------------------------------------------
  global_counter++;

  lseek64(infile,stored_pos,SEEK_SET);

  free(buffer);
  free(s);

  return 0;
}
