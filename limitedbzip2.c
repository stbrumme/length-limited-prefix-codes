// //////////////////////////////////////////////////////////
// limitedbzip2.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#include "limitedbzip2.h"

#include "moffat.h" // compute unlimited Huffman code lengths
#include <stdlib.h> // malloc/free/qsort


// the following code has shares many parts with the function moffat() in moffat.c
// (I avoid calling moffat() because it would keep sorting the same data again and again)


// helper struct for qsort()
struct KeyValue
{
  unsigned int key;
  unsigned int value;
};
// helper function for qsort()
static int compareKeyValue(const void* a, const void* b)
{
  struct KeyValue* aa = (struct KeyValue*) a;
  struct KeyValue* bb = (struct KeyValue*) b;
  return aa->key - bb->key; // negative if a < b, zero if a == b, positive if a > b
}


/// adjust bit lengths based on the algorithm found in bzip2's sources
/** - histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedBzip2(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  // my allround variable for various loops
  unsigned int i;

  // count non-zero histogram values
  unsigned int numNonZero = 0;
  for (i = 0; i < numCodes; i++)
    if (histogram[i] != 0)
      numNonZero++;

  // reject an empty alphabet because malloc(0) is undefined
  if (numNonZero == 0)
    return 0;

  // initialize output
  if (numNonZero < numCodes)
    for (i = 0; i < numCodes; i++)
      codeLengths[i] = 0;

  // allocate a buffer for sorting the histogram
  struct KeyValue* mapping = (struct KeyValue*) malloc(sizeof(struct KeyValue) * numNonZero);
  // copy histogram to that buffer
  unsigned int storeAt = 0;
  for (i = 0; i < numCodes; i++)
  {
    // skip zeros
    if (histogram[i] == 0)
      continue;

    mapping[storeAt].key   = histogram[i];
    mapping[storeAt].value = i;
    storeAt++;
  }
  // now storeAt == numNonZero

  // invoke C standard library's qsort
  qsort(mapping, numNonZero, sizeof(struct KeyValue), compareKeyValue);

  // extract ascendingly ordered histogram
  unsigned int* sorted = (unsigned int*) malloc(sizeof(unsigned int) * numNonZero);
  for (i = 0; i < numNonZero; i++)
    sorted[i] = mapping[i].key;

  // run Moffat algorithm ...
  unsigned char result = moffatSortedInPlace(numNonZero, sorted);
  // ... until a proper maximum code length is found
  while (result > maxLength)
  {
    // more or less divide each weight by two while avoiding zero
    for (i = 0; i < numNonZero; i++)
    {
      unsigned int weight = mapping[i].key;

      // bzip2 "clears" the lowest 8 bits of the histogram
      // to reach the length limit in less iterations
      // but sacrifices lots of efficiency
      // if you set EXTRA_SHIFT to 0 then the code may need more iterations
      // but finds much better code lengths
      //#define EXTRA_SHIFT 8
      #define EXTRA_SHIFT 0

      // sometimes dividing the weight by a bigger integer (e.g. 3)
      // may lead to more efficient prefix codes
      #define DIVIDE_BY   2

      // the shifts do more harm than good in my opinion
      weight >>= EXTRA_SHIFT;

      // adding 1 avoids zero
      weight   = 1 + (weight / DIVIDE_BY);
      weight <<= EXTRA_SHIFT;

      // sorted was overwritten with code lengths
      mapping[i].key = sorted[i] = weight;
    }

    // again: run Moffat algorithm
    result = moffatSortedInPlace(numNonZero, sorted);
  }

  // restore original order
  for (i = 0; i < numNonZero; i++)
    codeLengths[mapping[i].value] = sorted[i];

  // let it go ...
  free(sorted);
  free(mapping);

  return result;

}
