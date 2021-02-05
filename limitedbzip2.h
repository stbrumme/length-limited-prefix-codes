// //////////////////////////////////////////////////////////
// limitedbzip2.h
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#pragma once

/// adjust bit lengths based on the algorithm found in bzip2's sources
/** - histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedBzip2(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[]);

// the main idea is to adjust the histogram until the standard Huffman algorithm produces suitable code lengths
// see https://github.com/Unidata/compression/blob/master/bzip2/huffman.c
// => the "histogram adjustment" can be found @ lines 142-146:
//    for (i = 1; i <= alphaSize; i++) {
//      j = weight[i] >> 8;
//      j = 1 + (j / 2);
//      weight[i] = j << 8;
//    }
// => shifting by 8 is a very fast way to get suitable length-limited prefix codes
//    but often gives worse compression efficiency compared to other algorithms
// => getting rid of the shift by 8 is still quite fast and produces much better results
