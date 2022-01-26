#include <stdio.h>
#include <string.h>

#define BUFFERSIZE 1024

int main(int argc, char **argv)
{
  FILE *f;
  if(argc < 3) {
    printf("Usage: xor <XOR data> <file>\n");
    return 1;
  }

  f = fopen(argv[2], "rb");
  if(f) {
    int xorpos=0;
    int xorlen;
    char buffer[BUFFERSIZE];
    int rc;
    char *xorstr = argv[1];
    char buf[3]={0,0,0};
    int x;
    xorlen = strlen(argv[1]);
    do {
      rc = fread(buffer, 1, BUFFERSIZE, f);
      if(rc > 0) {
        int i;
        for(i=0; i<rc; i++) {
          buf[0] = xorstr[xorpos++];
          buf[1] = xorstr[xorpos++];
          x = (int)strtol(buf, NULL, 16);

          if(xorpos > xorlen)
            xorpos = 0;

          fputc(buffer[i] ^ x, stdout);
        }
      }

    } while(rc == BUFFERSIZE);
    fclose(f);
  }
  else {
    printf("Couldn't open file '%s'\n", argv[2]);
    return 2;
  }
  return 0;
}
