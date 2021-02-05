// //////////////////////////////////////////////////////////
// moffat.c
// written by Alistair Moffat, modified by Stephan Brumme
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#include "moffat.h"
#include <stdlib.h> // malloc/free/qsort


/// compute prefix code ("Huffman code") based on Moffat's in-place algorithm
/** histogram must be in ascending order and no entry must be zero
 *  @param  numCodes number of elements
 *  @param  A [in]   how often each code/symbol was found [out] computed code lengths
 *  @result maximum code length, 0 if error
 */
unsigned char moffatSortedInPlace(unsigned int numCodes, unsigned int A[])
{
  // based on page 61/90: https://www.cs.brandeis.edu/~dcc/Programs/Program2015KeynoteSlides-Moffat.pdf
  // see also             https://people.eng.unimelb.edu.au/ammoffat/inplace.c

  // handle two pathological cases
  if (numCodes <= 0)
    return 0;
  if (numCodes == 1)
  {
    A[0] = 1; // Moffat's code sets A[0] = 0
    return 1;
  }

  // phase 1
  unsigned int leaf = 0;
  unsigned int root = 0;
  unsigned int next;
  for (next = 0; next < numCodes - 1; next++)
  {
    // first child (assign to A[next])
    if (leaf >= numCodes || (root < next && A[root] < A[leaf]))
    {
      A[next] = A[root];
      A[root] = next;
      root++;
    }
    else
    {
      A[next] = A[leaf];
      leaf++;
    }

    // second child (add to A[next])
    if (leaf >= numCodes || (root < next && A[root] < A[leaf]))
    {
      A[next] += A[root];
      A[root] = next;
      root++;
    }
    else
    {
      A[next] += A[leaf];
      leaf++;
    }
  }

  // phase 2
  A[numCodes - 2] = 0;
  int j;
  for (j = numCodes - 3; j >= 0; j--)
    A[j] = A[A[j]] + 1;

  // phase 3
  unsigned int  avail = 1;
  unsigned int  used  = 0;
  unsigned char depth = 0;

  int root2 = (int)numCodes - 2;
  next = numCodes - 1;
  while (avail > 0)
  {
    while (root2 >= 0 && A[root2] == depth)
    {
      used++;
      root2--;
    }
    while (avail > used)
    {
      A[next] = depth;
      next--;
      avail--;
    }

    avail = 2 * used;
    depth++;
    used = 0;
  }

  // code length is in descending order, thus the first element is the longest
  return A[0];
}


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


/// same as before but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** @param  numCodes    number of codes, equals the array size of histogram and codeLength
 *  @param  histogram   how often each code/symbol was found
 *  @param  codeLengths (output) computed code lengths
 *  @result maximum code length, 0 if error
 */
unsigned char moffat(unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  // my allround variable for various loops
  unsigned int i;

  // count non-zero histogram values
  unsigned int numNonZero = 0;
  for (i = 0; i < numCodes; i++)
    if (histogram[i] == 0)
      codeLengths[i] = 0;
    else
      numNonZero++;

  // reject an empty alphabet because malloc(0) is undefined
  if (numNonZero == 0)
    return 0;

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

  // invoke C standary libary's qsort
  qsort(mapping, numNonZero, sizeof(struct KeyValue), compareKeyValue);

  // extract ascendingly ordered histogram
  unsigned int* sorted = (unsigned int*) malloc(sizeof(unsigned int) * numNonZero);
  for (i = 0; i < numNonZero; i++)
    sorted[i] = mapping[i].key;

  // run Moffat algorithm
  unsigned char result = moffatSortedInPlace(numNonZero, sorted);

  // restore original order
  for (i = 0; i < numNonZero; i++)
    codeLengths[mapping[i].value] = sorted[i];

  // let it go ...
  free(sorted);
  free(mapping);

  return result;
}
