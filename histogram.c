// //////////////////////////////////////////////////////////
// histogram.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

// gcc histogram.c -o histogram -Wall -O3
// ./histogram [filename]
// if filename is "-" then the program reads from STDIN

// count how often each byte is found in a file
// output is their frequency delimited by a whitespace
// if a symbol doesn't occur then its frequency is zero

#include <stdio.h>
#include <stdlib.h>

// read 64k at once
#define BUFFERSIZE (64*1024)

int main(int argc, char** argv)
{
  // needs exactly one command-line parameter
  if (argc != 2)
  {
    printf("syntax: ./histogram [filename]\n"
           "if filename is - then read from STDIN\n");
    return 1;
  }

  // open file (or STDIN)
  FILE* handle = stdin;
  const char* filename = argv[1];
  if (filename[0] != '-' || filename[1] != 0)
    handle = fopen(argv[1], "rb");

  // bad file ?
  if (!handle)
  {
    printf("cannot open %s\n", filename);
    return 2;
  }

  // for various loops
  size_t i;

  // histogram
  unsigned int histogram[256] = { 0 };

  // read 64k chunks and adjust histogram
  unsigned char buffer[BUFFERSIZE];
  while (!feof(handle))
  {
    // read
    size_t numRead = fread(buffer, 1, BUFFERSIZE, handle);
    if (numRead == 0)
      break;

    // histogram
    for (i = 0; i < numRead; i++)
      histogram[buffer[i]]++;
  }

  if (handle != stdin)
    fclose(handle);

  // show histogram
  printf("%d", histogram[0]);
  for (i = 1; i < 256; i++)
    printf(" %d", histogram[i]);
  printf("\n");

  return 0;
}
