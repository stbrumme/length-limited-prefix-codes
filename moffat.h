// //////////////////////////////////////////////////////////
// moffat.h
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#pragma once

/// compute prefix code lengths (Huffman codes) based on Moffat's in-place algorithm
/** - given a histogram of all used symbols, this function returns their optimal code length
 *  - this code length can be converted to prefix codes, e.g. canonical prefix codes
 *  - important: the histogram must be in ascending order and no entry must be zero
 *  - example: your data is "AADADCAA"
 *             => 5x A, no B, 1x C, 2x D
 *             => histogram: { 5, 0, 1, 2 }
 *             => sorted histogram: { 0, 1, 2, 5 } (representing B,C,D,A)
 *             => without zeros     { 1, 2, 5 }    (representing C,D,A)
 *             => code:
 *             int data = { 1, 2, 5 };
 *             int maxLength = moffatSortedInPlace(3, data);
 *             => data will be { 2, 2, 1 }
 *             => remember that the sorted histogram consisted of C,D,A (in that order)
 *             => thus encoding the single C with 2 bits, the two Ds with 2 bits and five As with one bit each
 *             => for a total of 5*1 + 0*0 + 1*2 + 2*2 = 11 bits for the whole data
 *  - if your data is larger than an integer then an internal overflow may occur
 *  => simply speaking: on a 32 bit system your data shouldn't exceed 2^31 bytes = 2 GByte
 *  @param  numCodes number of elements
 *  @param  A [in] how often each code/symbol was found [out] computed code lengths
 *  @result maximum code length, 0 if error
 */
unsigned char moffatSortedInPlace(unsigned int numCodes, unsigned int A[]);


// ---------- same algorithm with a more convenient interface ----------

/// same as before but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** @param  numCodes    number of codes, equals the array size of histogram and codeLength
 *  @param  histogram   how often each code/symbol was found
 *  @param  codeLengths (output) computed code lengths
 *  @result maximum code length, 0 if error
 */
unsigned char moffat(unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[]);
