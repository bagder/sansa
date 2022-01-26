#include <stdio.h>
#include <string.h>

#define BUFFERSIZE 1024

int distro[256];

int main(int argc, char **argv)
{
  FILE *f;
  if(argc < 2) {
    printf("Usage: ana <file>\n");
    return 1;
  }

  f = fopen(argv[1], "rb");
  if(f) {
    unsigned char buffer[BUFFERSIZE];
    int rc;
    int i;
    do {
      rc = fread((char *)buffer, 1, BUFFERSIZE, f);
      if(rc > 0) {
        for(i=0; i<rc; i++) {
          distro[ buffer[i] ]++;
        }
      }

    } while(rc == BUFFERSIZE);
    fclose(f);

    for(i=0; i<256; i++)
      printf("%d %d\n", i, distro[i]);
  }
  else {
    printf("Couldn't open file '%s'\n", argv[1]);
    return 2;
  }
  return 0;
}
