#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

static int le2int(unsigned char *c)
{
  return c[0] | (c[1] << 8) | (c[2] << 16) | c[3] <<24;
}

int main(int argc, const char *argv[])
{
  unsigned int sum=0;
  if(argc > 1) {
    int fd;
    fd = open(argv[1], O_RDONLY);
    if(fd != -1) {
      unsigned char buffer[1024];
      ssize_t rc=1;
      int counter;

      if(0x20 != read(fd, buffer, 0x20)) /* get the first header */
        return 2;

      /* get counter from the header at index 0x0c */
      counter = le2int(&buffer[0xc]);

      lseek(fd, 0x400, SEEK_SET); /* skip header */

      while((counter > 0) && (rc > 0)) {
        int get = counter > sizeof(buffer)?sizeof(buffer):counter;
        rc = read(fd, buffer, get);
        if(rc > 0) {
          int i;
          unsigned int *ptr = (unsigned int *)buffer;
          for(i=0; i< rc/4; i++) {
            sum += ptr[i];
          }
          counter -= rc;
        }
      }
      printf("%08x\n", sum);
    }
  }

  return 0;
}
