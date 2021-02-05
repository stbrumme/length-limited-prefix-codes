// //////////////////////////////////////////////////////////
// limitedkraftheap.h
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#pragma once

/// create prefix code lengths solely by optimizing the Kraft inequality
/**
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedKraftHeap(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[]);
