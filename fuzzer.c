// //////////////////////////////////////////////////////////
// fuzzer.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

// afl-gcc fuzzer.c limited*.c packagemerge.c moffat.c -o fuzzer
// to be used by afl-gcc
// afl-fuzz -i afl-testcases -o afl-findings

// it's histogram.c + running a length-limiting algorithm

// settings (hard-coded):
#define LIMIT_BITS 8
#define ALGORITM   packageMerge


#include "packagemerge.h"
#include "moffat.h"
#include "limitedjpegdeflate.h"
#include "limitedbzip2.h"
#include "limitedkraft.h"
#include "limitedkraftheap.h"

#include <stdio.h>
#include <stdlib.h>

// read 64k at once
#define BUFFERSIZE (64*1024)
// 256 codes
#define MAXSYMBOLS 256

// cause AFL to detect a crash
int crash(int code)
{
  // deliberately cause a SEGFAULT for afl-fuzz
  char* bad = NULL;
  *bad = code;
  exit(code);
}


int main(int argc, char** argv)
{
  // open STDIN
  FILE* handle = stdin;

  // for various loops
  size_t i;

  // histogram
  unsigned int histogram[MAXSYMBOLS] = { 0 };

  // read 64k chunks and adjust histogram
  unsigned char buffer[BUFFERSIZE];
  size_t totalBytes = 0;
  while (!feof(handle))
  {
    // read
    size_t numRead = fread(buffer, 1, BUFFERSIZE, handle);
    if (numRead == 0)
      break;

    totalBytes += numRead;

    // histogram
    for (i = 0; i < numRead; i++)
      histogram[buffer[i]]++;
  }

  unsigned char codeLengths[MAXSYMBOLS];
  unsigned char maxBits = ALGORITM(LIMIT_BITS, MAXSYMBOLS, histogram, codeLengths);

  if (maxBits == 0)
    crash(1);

  // check Kraft value (must not be greater than 1.0)
  unsigned long long one = 1ULL << maxBits;
  unsigned long long sum = 0;
  unsigned int  numUsedCodes = 0;
  for (i = 0; i < MAXSYMBOLS; i++)
    if (codeLengths[i] > 0)
    {
      sum += one >> codeLengths[i];
      numUsedCodes++;
    }
  double kraft = sum / (double) one;
  if (kraft > 1)
    crash(2);

  return 0;
}
