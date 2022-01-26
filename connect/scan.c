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
  if(argc > 2) {
    int fde;
    int fdd;
    int c;
    int i;
    fde = open(argv[1], O_RDONLY);
    fdd = open(argv[2], O_RDONLY);
    if((fde != -1) && (fdd != -1)) {
      unsigned char buffer[1024];
      unsigned char bufferd[1024];
      ssize_t rc;
      ssize_t rc2;
      int counter;
      int *ptr;
      int *ptr2;

      c=0;
      while(1) { /* until complete */

        rc = read(fde, buffer, sizeof(buffer));
        rc2 = read(fdd, bufferd, sizeof(bufferd));

        if((rc != rc2) || (rc < 0)) {
          return 1;
        }


        ptr=(int *)buffer;
        ptr2=(int *)bufferd;

        for(i=0; i< rc / 4; i++) {
          printf("%08x %08x %08x\n", ptr[i] - ptr2[i], ptr[i], ptr2[i]);
          if(++c >= 8) {
            //printf("\n");
            c=0;
          }
        }
      }
    }
  }
  else
    printf("usage: scan <enc> <dec>\n");

  return 0;
}
