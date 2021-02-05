// //////////////////////////////////////////////////////////
// benchmark.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

// gcc benchmark.c packagemerge.c limited*.c moffat.c -o benchmark -Wall -O3

#include "packagemerge.h"
#include "moffat.h"
#include "limitedjpegdeflate.h"
#include "limitedbzip2.h"
#include "limitedkraft.h"
#include "limitedkraftheap.h"

#include <stdio.h>
#include <stdlib.h>

#define MAXSYMBOLS 256

// histogram of first 64k of enwik dataset from http://mattmahoney.net/dc/textdata.html
// created by histogram.c
unsigned int histogram[MAXSYMBOLS] = { 0,0,0,0,0,0,0,0,0,0,538,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8289,6,72,31,0,1,309,509,57,58,58,0,448,278,565,490,150,215,94,61,57,71,47,53,87,123,195,345,294,151,293,12,0,275,85,153,50,97,76,64,56,134,40,33,66,113,58,33,116,5,98,147,172,33,17,84,3,11,19,1172,0,1173,0,35,0,4125,472,1866,1424,4746,918,776,2091,4112,73,308,1796,1593,3528,3514,1109,177,3069,3334,4336,1288,513,535,179,670,58,64,171,64,3,0,6,0,5,2,5,3,0,0,2,1,3,0,2,0,0,0,4,0,0,1,2,2,1,2,4,2,0,2,1,1,0,1,4,1,3,0,1,1,2,2,1,15,2,2,0,2,0,2,4,1,2,7,2,0,0,4,17,2,3,1,3,3,0,1,0,0,0,25,2,1,0,0,0,0,0,0,0,0,0,0,19,7,0,0,0,0,0,7,10,6,0,1,0,0,0,0,14,0,3,5,2,1,2,0,0,0,0,1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

// histogram of first 64k of calgary/obj2: head -c65536 calgary/obj2 | ./histogram -
//unsigned int histogram[MAXSYMBOLS] = { 12987,1389,1275,416,749,560,562,320,642,179,361,72,547,138,244,49,521,96,180,85,121,62,103,46,167,77,111,75,83,65,131,288,1768,77,569,852,129,48,93,33,178,44,273,62,82,231,893,674,587,187,236,111,88,47,63,30,89,51,162,22,1140,253,144,1206,571,340,456,168,182,138,76,65,1530,65,281,61,230,64,1838,157,277,114,208,175,172,131,233,89,121,72,78,12,71,19,216,519,352,410,97,181,182,617,331,397,143,488,243,65,250,214,1759,424,340,30,405,310,645,352,55,105,148,67,148,13,119,7,29,31,284,40,52,19,30,12,25,36,189,13,67,8,74,29,38,295,114,83,48,21,24,7,77,38,115,100,130,13,57,9,66,92,468,139,68,10,54,7,57,40,222,760,167,5,30,901,87,19,93,13,49,9,45,7,14,4,52,4,94,13,64,2,34,7,236,87,52,41,38,7,56,13,47,11,34,8,40,17,117,48,247,157,73,51,58,10,37,24,193,9,41,3,77,19,70,102,96,19,201,42,62,108,146,89,80,15,116,102,61,75,136,77,652,28,116,25,41,16,118,6,182,36,353,151,506,200,663,2443 };

int main(int argc, char* argv[])
{
  // parse command-line
  if (argc < 3 || argc > 5)
  {
    printf("syntax: ./benchmark ALGORITHM BITS [REPEAT] [HISTOGRAMFILE]\n"
           " # ALGORITHM     => a number between 1 and 6: 1=Package-Merge, 2=MiniZ, 3=JPEG, 4=BZip2, 5=Kraft, 6=modified Kraft\n"
           " # BITS          => the upper code length limit\n"
           " # REPEAT        => repeat algorithm to get more precise timing, default=1000\n"
           " # HISTOGRAMFILE => read pre-computed histogram from a file\n");
    return 1;
  }

  // basic loop counter
  int i;

  // algorithm's name
  const char* name = "???";

  // upper code length limit
  int limitBits = atoi(argv[2]);
  if (limitBits == 0)
    return 2;

  // more accurate timing if repeating (default: 1000)
  int repeat = argc >= 4 ? atoi(argv[3]) : 0;
  if (repeat <= 0)
    repeat = 1000;

  // histogram
  if (argc == 5)
  {
    // open file or STDIN
    FILE* handle = stdin;
    const char* filename = argv[4];
    if (filename[0] != '-' || filename[1] != 0)
      handle = fopen(filename, "rb");

    if (!handle)
    {
      printf("can't open histogram %s\n", filename);
      return 2;
    }

    // read the first 256 values
    for (i = 0; i < MAXSYMBOLS; i++)
      if (feof(handle))
        histogram[i] = 0;
      else
        fscanf(handle, "%d", &histogram[i]);

    fclose(handle);
  }

  // parameters of length limiting algorithms
  unsigned int  numCodes  = MAXSYMBOLS;
  unsigned char codeLengths[MAXSYMBOLS];
  unsigned char maxBits;

  // choose an algorithm and run it repeatedly
  int algorithm = argv[1][0] - '0';
  switch (algorithm)
  {
    case 0:
      name = "moffat (ignores bit limit)";
      for (i = 0; i < repeat; i++)
        maxBits = moffat      (           numCodes, histogram, codeLengths);
      break;

    case 1:
      name = "packageMerge";
      for (i = 0; i < repeat; i++)
        maxBits = packageMerge(limitBits, numCodes, histogram, codeLengths);
      break;

    case 2:
      name = "limitedMiniz";
      for (i = 0; i < repeat; i++)
        maxBits = limitedMiniz(limitBits, numCodes, histogram, codeLengths);
      break;

    case 3:
      name = "limitedJpeg";
      for (i = 0; i < repeat; i++)
        maxBits = limitedJpeg(limitBits, numCodes, histogram, codeLengths);
      break;

    case 4:
      name = "limitedBzip2";
      for (i = 0; i < repeat; i++)
        maxBits = limitedBzip2(limitBits, numCodes, histogram, codeLengths);
      break;

    case 5:
      name = "limitedKraft";
      for (i = 0; i < repeat; i++)
        maxBits = limitedKraft(limitBits, numCodes, histogram, codeLengths);
      break;

    case 6:
      name = "limitedKraftHeap";
      for (i = 0; i < repeat; i++)
        maxBits = limitedKraftHeap(limitBits, numCodes, histogram, codeLengths);
      break;

    default:
      printf("invalid algorithm\n");
      return 2;
  }

  // failed ?
  if (maxBits == 0)
  {
    printf("BITS is too small (%d), no valid code possible\n", limitBits);
    return 3;
  }

  // count total size of encoded data (without overhead of Huffman tables)
  unsigned long long original   = 0;
  unsigned long long compressed = 0;
  for (i = 0; i < numCodes; i++)
  {
    original   +=       8        * histogram[i]; // one byte per symbol
    compressed += codeLengths[i] * histogram[i];
  }

  // compression ratio
  double percentage = 100.0 * compressed / (double) original;

  // check Kraft value (must not be greater than 1.0)
  unsigned long long one = 1ULL << maxBits;
  unsigned long long sum = 0;
  for (i = 0; i < numCodes; i++)
    if (codeLengths[i] > 0)
      sum += one >> codeLengths[i];
  double kraft = sum / (double) one;

  // output
  printf("algorithm: %s\n", name);
  printf("limit to %d bits (max. %d bits actually produced)\n", limitBits, maxBits);
  printf("%lld => %lld bits (%.2f%%)\n", original, compressed, percentage);
  printf("check Kraft value: %s (%.6f)\n", kraft <= 1 ? "ok" : "FAILED", kraft);
  printf("repeat %dx\n", repeat);

  // debug: show histogram and code lengths
  //for (i = 0; i < numCodes; i++)
  //  printf("%d=%d/%d\t", i, histogram[i], codeLengths[i]);

  return 0;
}
