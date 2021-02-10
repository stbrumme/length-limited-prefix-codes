// //////////////////////////////////////////////////////////
// limitedjpegdeflate.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#include "limitedjpegdeflate.h"

#include "moffat.h" // compute unlimited Huffman code lengths for limitedImpl()
#include <stdlib.h> // malloc/free/qsort


/// adjust bit lengths based on the algorithm in JPEG Annex K.3 specification
/** - it's assumed that no value in histNumBits[] exceed 63
 *  - histNumBits[0] is unused and must be zero
 *  - modifications are performed in-place
 *  - maxLength must be a bit length where a prefix code exists
 *  - not much error checking, invalid input can easily crash the code
 *  @param  oldMaxLength current maximum code length
 *  @param  newMaxLength desired new maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  histNumBits histogram of bit lengths [in] and [out]
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedJpegInPlace(unsigned char newMaxLength, unsigned char oldMaxLength, unsigned int histNumBits[])
{
  // see https://www.w3.org/Graphics/JPEG/itu-t81.pdf
  // Annex K.3 (page 147)

  // the basic idea is as follows:
  // - for each proper prefix code the longest bit length always has an even number of symbols
  // - let's pick two symbols x and y having that longest bit length
  // - the canonical Huffman code of x and y will be almost identical:
  //   x ends with 0 and y ends with 1
  // - let's call their shared prefix P
  // - we remove x's trailing 0, so that x becomes P, being one bit shorter
  // - however, y is invalid now
  // - let's pick a third symbol z which is shorter than the new x
  //   (that means at least two bits shorter than the still invalid y)
  // - appending a 0 to z makes it one bit longer
  // - appending a 1 to z creates a completely new code
  //   which can be used for y

  // what did we achieve ?
  // 1. we made 1 symbol shorter by 1 bit (x)
  // 2. we made 1 symbol shorter by at least 1 bit (y)
  // 3. we made 1 symbol longer  by 1 bit (z)

  // from a mathematical point of view the sum of all Kraft values must be <= 1.0
  // - Kraft value of symbol s is K(s) = 2^-bits(s)
  // - we know that bits(x) = bits(y)
  // - if bits(z) = bits(x) - 2 then sum(Kraft) = sum(Kraft')
  //   because bits(x') = bits(x) - 1 and bits(y') = bits(y) - 1 and bits(z') = bits(z) + 1
  //   so that bits(z') = bits(x') = bits(y')
  // - let's forget about all symbols except x, y and z so that we have:
  //   sum(K)  =     K(x)   +     K(y)   +     K(z)
  //           = 2^-bits(x) + 2^-bits(y) + 2^-bits(z)
  //           = 2^-bits(x) + 2^-bits(x) + 2^-(bits(x) - 2)
  //           =       2 * 2^-bits(x)    + 4 * -bits(x)
  //           = 6 * 2^-bits(x)
  //   sum(K') =     K(x')  +     K(y')  +     K(z')
  //           = 3 * K(x')
  //           = 3 * 2^-bits(x')
  //           = 3 * 2^(-bits(x) - 1)
  //           = 6 * 2^-bits(x)
  //           = sum(K)
  // - therefore the sum of all Kraft values remains unchanged after the transformation

  // each step reduces the bit lengths of just two symbols by typically one bit
  // => processing a huge alphabet with very large bit lengths can be quite slow
  //    (however, this situation is impossible with the small JPEG alphabet)

  if (newMaxLength <= 1)
    return 0;
  if (newMaxLength >  oldMaxLength)
    return 0;
  if (newMaxLength == oldMaxLength)
    return newMaxLength;

  // iterate over all "excessive" bit lengths, beginning with the longest
  unsigned char i = oldMaxLength;
  while (i > newMaxLength)
  {
    // no codes at this bit length ?
    if (histNumBits[i] == 0)
    {
      i--;
      continue;
    }

    // look for codes that are at least two bits shorter
    unsigned char j = i - 2;
    while (j > 0 && histNumBits[j] == 0)
      j--;

    // change bit length of 2 of the longest codes
    histNumBits[i] -= 2;
    // one code becomes one bit shorter
    // (using the joint prefix of the old two codes)
    histNumBits[i - 1]++;

    // the other code has length j+1 now
    // but another, not-yet-involved, code with length j
    // is moved to bit length j+1 as well
    histNumBits[j + 1] += 2;
    histNumBits[j]--;
  }

  // return longest code length that is still used
  while (i > 0 && histNumBits[i] == 0)
    i--;

  // JPEG Annex K.3 specifies an extra line:
  // histNumBits[i]--;
  // => because JPEG needs a special symbol to avoid 0xFF in its output stream

  return i;
}


/// adjust bit lengths based on the algorithm found in MiniZ's sources
/** - it's assumed that no value in histNumBits[] exceed 63
 *  - histNumBits[0] is unused and must be zero
 *  - modifications are performed in-place
 *  - maxLength must be a bit length where a prefix code exists
 *  - not much error checking, invalid input can easily crash the code
 *  @param  oldMaxLength current maximum code length
 *  @param  newMaxLength desired new maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  histNumBits histogram of bit lengths [in] and [out]
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedMinizInPlace(unsigned char newMaxLength, unsigned char oldMaxLength, unsigned int histNumBits[])
{
  // see https://github.com/richgel999/miniz/blob/master/miniz_tdef.c#L197
  // code reformatted and commented by Stephan Brumme

  // the idea is the same as explained in limitedJpegInPlace()
  // but instead of slowly arriving at newMaxLength it immediately starts there

  // the MiniZ code runs faster than the JPEG algorithm

  if (newMaxLength <= 1)
    return 0;
  if (newMaxLength >  oldMaxLength)
    return 0;
  if (newMaxLength == oldMaxLength)
    return newMaxLength;

  // my allround variable for various loops
  unsigned int i;

  // move all oversized code lengths to the longest allowed
  for (i = newMaxLength + 1; i <= oldMaxLength; i++)
  {
    histNumBits[newMaxLength] += histNumBits[i];
    histNumBits[i] = 0;
  }

  // compute Kraft sum
  // (using integer math: everything is multiplied by 2^newMaxLength)
  unsigned long long total = 0;
  for (i = newMaxLength; i > 0; i--)
    total += histNumBits[i] << (newMaxLength - i);

  // iterate until Kraft sum doesn't exceed 1 anymore
  unsigned long long one = 1ULL << newMaxLength;
  while (total > one)
  {
    // select a code with maximum length, it will be moved
    histNumBits[newMaxLength]--;

    // find a second code with shorter length
    for (i = newMaxLength - 1; i > 0; i--)
      if (histNumBits[i] > 0)
      {
        histNumBits[i]--;
        // extend that shorter code by one bit
        // and assign the same code length to the selected one
        histNumBits[i + 1] += 2;

        // note: it's possible (and quite likely !) that the selected code
        //       gets assigned the same code it had before
        // example: - newMaxLength is 16 and you have codes with 15 bits, too
        //          - you select a 16 bit code and pick a second 15 bit code
        //          - that 15 bit code becomes a 16 bit code and
        //            the 16 bit code stays a 16 bit code

        break;
      }

    // moving these codes reduced the Kraft sum
    total--;
  }

  return newMaxLength;
}


// the following code has shares many parts with the function moffat() in moffat.c
// (I avoid calling moffat() because adjusting the sorted code length is much easier and faster)


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
// actual implementation (JPEG/GZIP)
typedef unsigned char (*LimitedInPlace)(unsigned char newMaxLength, unsigned char oldMaxLength, unsigned int histNumBits[]);


// code is for limitedJpeg and limitedGzip would be 99% identical, they just call a differenz in-place algorithm
unsigned char limitedImpl(LimitedInPlace algorithm, unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  // reject invalid input
  if (maxLength == 0 || maxLength > 63 || numCodes == 0)
    return 0;

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

  // run Moffat algorithm
  unsigned char maxLengthUnlimited = moffatSortedInPlace(numNonZero, sorted);
  // ----- until here the code was pretty much the same as moffat() -----

  // Huffman codes already match the maxLength requirement ?
  if (maxLengthUnlimited <= maxLength)
  {
    // restore original order
    for (i = 0; i < numNonZero; i++)
      codeLengths[mapping[i].value] = sorted[i];

    free(sorted);
    free(mapping);
    return maxLengthUnlimited;
  }

  // at most 63 bits
  if (maxLengthUnlimited > 63)
  {
    free(sorted);
    free(mapping);
    return 0;
  }

  // histogram of code lengths
  unsigned int histNumBits[64] = { 0 };
  for (i = 0; i < numNonZero; i++)
    histNumBits[sorted[i]]++;

  // now reduce code length with JPEG/GZIP algorithm
  unsigned char newMax = algorithm(maxLength, maxLengthUnlimited, histNumBits);

  // failed ?
  if (newMax == 0)
  {
    free(sorted);
    free(mapping);
    return 0;
  }

  // code lengths are in descending order, adjust them
  unsigned char reduce = newMax;
  for (i = 0; i < numNonZero; i++)
  {
    // find symbol associated to that code length
    unsigned int unsorted = mapping[i].value;
    // assign longest available code length
    codeLengths[unsorted] = reduce;

    // prepare next code length
    histNumBits[reduce]--;
    while (histNumBits[reduce] == 0)
      reduce--;
  }

  // let it go ...
  free(sorted);
  free(mapping);

  return newMax;
}


/// same as limitedJpegInPlace but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** - the function rejects maxLength > 63 but I don't see any practical reasons you would need a larger limit ...
 *  @param  maxLength  maximum code length, e.g. 15 for JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedJpeg(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  return limitedImpl(limitedJpegInPlace, maxLength, numCodes, histogram, codeLengths);
}


/// same as limitedMinizInPlace but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** - the function rejects maxLength > 63 but I don't see any practical reasons you would need a larger limit ...
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedMiniz(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  return limitedImpl(limitedMinizInPlace, maxLength, numCodes, histogram, codeLengths);
}
