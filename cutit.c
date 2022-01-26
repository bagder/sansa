#include <stdio.h>

int main(int argc, char **argv)
{
  FILE *inf;
  FILE *outf;
  if(argc < 3) {
    printf("Usage: cutit <infile> <outfile>\n");
    return 1;
  }

  inf= fopen(argv[1], "r");
  if(inf) {
    unsigned char data[16384];
    int filesize;

    fseek(inf, 0x80200, SEEK_SET);
    printf("seek done\n");

    outf = fopen(argv[2], "w");
    if(!outf)
      return 2;
    fread(data, 0x14, 1, inf);
    fwrite(data, 0x14, 1, outf);

    fread(data, 0x4, 1, inf); /* this is the total firmware file size */
    filesize = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    fwrite(data, 0x4, 1, outf);

    printf("firmware size: %d bytes\n", filesize);

    filesize -= 0x18; /* we've already written this many bytes */

    while(filesize > 0) {
      size_t rc = fread(data, 1, sizeof(data), inf);

      filesize -= rc;
      if(filesize < 0) {
        /* we got more data than we need, cut off the end of this block */
        rc += filesize;
      }

      if(rc > 0) {
        size_t wc = fwrite(data, 1, rc, outf);
        printf("Wrote %d bytes\n", wc);
        if(wc < 0)
          return 3;
      }
    }
    fclose(inf);
    fclose(outf);
    printf("operation complete\n", filesize);
  }
}
