// //////////////////////////////////////////////////////////
// limitedjpeggzip.h
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#pragma once

// this file contains two very similar length-limiting algorithm:
// 1. the procedure described in JPEG Annex K.3
// 2. the technique found in MiniZ's source code
// both produce the same output while the latter is

/// adjust bit lengths based on the algorithm in JPEG Annex K.3 specification
/** - it's assumed that no value in histNumBits[] exceed 63
 *  - histNumBits[0] is unused and must be zero
 *  - modifications are performed in-place
 *  - maxLength must be a bit length where a prefix code exists
 *    => that means there are no more than 2^maxLength symbols
 *    => which is the same as sum(histNumBits) <= 2^maxLength
 *  - not much error checking, invalid input can easily crash the code
 *  - example: assume you generated a prefix code with the following bit lengths
 *             symbol A needs 1 bit, B isn't used (0 bits), C needs 2 bits, D 3 bits, E 4 bits, F and G need 5 bits each
 *             => histogram of the bit lengths of all used symbols is
 *                1x 0 bits (discarded), 1x 1 bits, 1x 2 bits, 1x 3 bits, 1x 4 bits and 2x 5 bits
 *             => { 0, 1, 1, 1, 1, 2 }
 *             => proper prefix code because its Kraft value is exactly 1.0 (1*2^-1 + 1*2^-2 + 1*2^-3 + 1*2^-4 + 2*2^-5 = 1.0)
 *             => code to reduce from max. 5 bits to max. 4 bits:
 *             int histNumBits[] = { 0, 1, 1, 1, 1, 2 };
 *             int newMaxLength = jpeg(4, 5, histNumBits); // enforce an upper limit of 4 bits (had up to 5 bits)
 *             => then histNumBits[] becomes { 0, 1, 1, 0, 4, 0 }
 *             => C moved from 2 to 3 bits while F and G were reduced from 5 to 4 bits
 *
 *             => code to reduce from max. 5 bits to max. 3 bits:
 *             int histNumBits[] = { 0, 1, 1, 1, 1, 2 };
 *             int newMaxLength = jpeg(3, 5, histNumBits); // enforce an upper limit of 3 bits (had up to 5 bits)
 *             => then histNumBits[] becomes { 0, 0, 2, 4, 0, 0 }
 *             => lots of changes: A one bit longer, E one bit shorter, F and G two bits shorter
 *  @param  oldMaxLength current maximum code length
 *  @param  newMaxLength desired new maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  histNumBits histogram of bit lengths [in] and [out]
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedJpegInPlace(unsigned char newMaxLength, unsigned char oldMaxLength, unsigned int histNumBits[]);

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
unsigned char limitedMinizInPlace(unsigned char newMaxLength, unsigned char oldMaxLength, unsigned int histNumBits[]);


// ---------- same algorithm with a more convenient interface ----------

/// same as limitedJpegInPlace but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** - the function rejects maxLength > 63 but I don't see any practical reasons you would need a larger limit ...
 *  @param  maxLength  maximum code length, e.g. 15 for JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedJpeg(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[]);

/// same as limitedMinizInPlace but histogram can be in any order and may contain zeros, the output is stored in a dedicated parameter
/** - the function rejects maxLength > 63 but I don't see any practical reasons you would need a larger limit ...
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedMiniz(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[]);
