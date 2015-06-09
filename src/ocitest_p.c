#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#include "ocitest.h"

int main(void)
{
  int fd;
  char buff[100];
  struct oci_struct ocis;

  fd=open("/dev/ocitest",O_RDWR);
  if(fd<0)
    { fprintf(stderr,"cannot open device\n"); return 1; }
  
  ioctl(fd,OCI_NOP);
  ioctl(fd,OCI_INT,12345);

  strcpy(buff,"ABCDEFGhijklmnOPQRSTUvwxyz0123456789");
  printf("ioctl(fd,OCI_STRING,\"%s\"); ->\n",buff);
  ioctl(fd,OCI_STRING,buff);
  printf("       -> %s\n",buff);

  ocis.p=buff;
  ioctl(fd,OCI_STRUCT,&ocis);
  printf("length of \"%s\"=%d\n",ocis.p,ocis.len);
  close(fd);
  return 0;
}