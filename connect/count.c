#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

static int le2int(unsigned char *c)
{
  return c[0] | (c[1] << 8) | (c[2] << 16) | c[3] <<24;
}

static int table[256];

int main(int argc, const char *argv[])
{
  unsigned int sum=0;
  if(argc > 1) {
    int fd;
    int i;
    int min;
    int max;
    fd = open(argv[1], O_RDONLY);
    if(fd != -1) {
      unsigned char buffer[1024];
      ssize_t rc;
      int counter;

      while(1) { /* until complete */

        rc = read(fd, buffer, sizeof(buffer));
        if(rc <= 0)
          break;
        for(i=0; i< rc; i++)
          table[ buffer[i] ]++;
      }
    }
    min = 100000;
    max =0;
    for(i=0; i<256; i++) {
      printf("%d = %d\n", i, table[i]);
      if(min > table[i])
        min = table[i];
      if(max < table[i])
        max = table[i];
    }
    printf("Range: %d - %d\n", min, max);
  }
  else
    printf("usage: scan <enc> <dec>\n");



  return 0;
}
