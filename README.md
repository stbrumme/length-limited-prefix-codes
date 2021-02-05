# Introduction

This is a collection of various algorithms to produce length-limited prefix codes.\
Code is written in plain C with tons of comments and there are no dependencies on third-party libraries - it just uses C standard stuff.

See my [homepage](https://create.stephan-brumme.com/length-limited-prefix-codes/) for more information.

All code is zlib-licensed.


# Overview

Algorithm      | Files                                                           | Reference
---------------|-----------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
Package-Merge  | [header](packagemerge.h)       / [source](packagemerge.c)       | [Larmore/Hirschberg's paper](https://dl.acm.org/doi/10.1145/79147.79150)
JPEG / MiniZ   | [header](limitedjpegdeflate.h) / [source](limitedjpegdeflate.c) | [JPEG Annex K.3](https://www.w3.org/Graphics/JPEG/itu-t81.pdf) and [MiniZ's source code](https://github.com/richgel999/miniz/blob/master/miniz_tdef.c#L197)
BZip2          | [header](limitedbzip2.h)       / [source](limitedbzip2.c)       | [BZip2's source code](https://sourceware.org/git?p=bzip2.git;a=blob;f=huffman.c;h=43a1899e4688e80a5b0027203426e319fda890ba;hb=HEAD#l142)
Kraft          | [header](limitedkraft.h)       / [source](limitedkraft.c)       | [Kraft-McMillan inequality](https://en.wikipedia.org/wiki/Kraft%E2%80%93McMillan_inequality) and [Charles Bloom's blog](http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html)
modified Kraft | [header](limitedkraftheap.h)   / [source](limitedkraftheap.c)   | my own Kraft encoder, runs much faster

To use an algorithm in your own project, just add its `.h` and `.c` file.\
JPEG / MiniZ and BZip2 need [`moffat.h`](moffat.h) and [`moffat.c`](moffat.c) for the shared interface because they lack a Huffman encoder.
If you have your own Huffman encoder then you can remove it.

There are short chapters in this document for each algorithm. Just scroll down.

In addition, there is a simple [benchmark](benchmark.c) program and a [histogram](histogram.c) tool
so that you can easily test these algorithms on your own hardware with your own data sets.
Scroll down for a description of the benchmark program.


# Basic usage

All algorithms share the same interface:

`unsigned char algorithmName(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])`

with three input parameters:

`maxLength` is the upper limit of the number of bits for each prefix code\
`numCodes` is the number of symbols (including unused symbols)\
`histogram` is an array of `numCodes` counters for each symbol

and one output parameter:

`codeLengths` will contain the bit length of each symbol

The return value is the longest bit length or zero if the algorithm failed.

However, this shared interface comes with a little bit of overhead - sometimes doubling code size and/or execution time.\
Therefore most files have a second public function which is more specialized for its algorithm but may have a few restrictions.\
A common case is that the histogram has to be sorted.


# Huffman codes

Package-Merge and both Kraft implementations generate prefix codes "from scratch".

All other routines consist of two steps:
1. an external function which produces Huffman codes
2. limiting the lengths of steps (if necessary)

Alistair Moffat's [in-place algorithm](https://people.eng.unimelb.edu.au/ammoffat/inplace.c) is a fast and compact [implementation](moffat.c)
and my choice for step 1.


# Package-Merge

Package-Merge always generates optimal code lengths. If speed is of no concern, then I recommend Package-Merge.

There's a [Wikipedia](https://en.wikipedia.org/wiki/Package-merge_algorithm) entry which is far more readable than the original
paper by [Larmore/Hirschberg's paper](https://dl.acm.org/doi/10.1145/79147.79150).

Calculation can be done in-place if the histogram is sorted in ascending order and there are no zeros.
My code uses bitmasks so that the maximum code length is 63.
For most practical applications a code limit of 31 may suffice so you should think about replacing these `unsigned long long` bitmasks by
`unsigned int` for a small performance gain.


# MiniZ / JPEG

These two in-place algorithms share the same `.h`/`.c` files because they are extremely similar:
1. they create a histogram of code lengths
2. "oversized" codes will be reduced until they fit into the given length limit
3. in order to still have a valid prefix code, some short codes must become longer

After step 1 we have a pretty small table where entry `x` contains the number for symbols with code length `x`.

The main difference between MiniZ and JPEG is step 2:\
the JPEG standard ( [Annex K.3](https://www.w3.org/Graphics/JPEG/itu-t81.pdf) ) defines a simple way to shrink/extend bit lengths one-at-a-time.\
MiniZ on the other side immediately reduces all oversized codes to the maximum allowed length and extends short codes until we have a valid prefix code.

MiniZ's approach is almost always faster. But frankly speaking, runtime is negligible in comparison to the Huffman code generation which runs beforehand.

[GZip](https://www.gzip.org/)'s approach to limiting prefix code lengths [looks a bit more complex](https://github.com/madler/zlib/blob/master/inftrees.c) but is essentially the same.


# BZip2

I found a super simple way to limit lengths in [BZip2](https://sourceware.org/bzip2/)'s sources:
1. create Huffman codes
2. if some of those codes exceed the limit then reduce symbols' frequencies and repeat step 1

The are various way to perform step 2. BZip2 is dividing each symbol's frequency by 2 (and clears the lowest 8 bits, see [its code](https://sourceware.org/git/?p=bzip2.git;a=blob;f=huffman.c;h=43a1899e4688e80a5b0027203426e319fda890ba;hb=HEAD#l142) ).
Care must be taken that no frequency becomes zero because that would indicate the symbol isn't used.

[My code](limitedbzip2.c) is a bit more flexible: you can specify a constant `DIVIDE_BY` (default: `2`) and a constant `EXTRA_SHIFT` (default: `0`).\
Setting `EXTRA_SHIFT` to `8` will make the code behave just like BZip2 but I found that `0` leads to better (= shorter) code lengths at no significant performance loss.

I encountered multiple input data sets where a higher `DIVIDE_BY`, e.g. `3` instead of `2`, actually improved code lengths AND made the algorithm run faster.

In general, performance varies wildly depending on the number of iterations.\
Step 1 clearly dominates execution time, step 2 is almost for free.


# Kraft codes

The [Kraft-McMillan inequality](https://en.wikipedia.org/wiki/Kraft%E2%80%93McMillan_inequality) must be true for each prefix code.

In mathematical terms: if `L1`,`L2`,...,`Ln` are code lengths then `sum(2^-Lx) <= 1` where `x`=`1`,`2`,...,`n` (I call it the Kraft sum)

The optimal code length for a symbol `x` is determined by its entropy: `Lx = -log2(px)` where `Lx` is the number of bits for symbol and `p` how likely the symbol is.\
For example, if every fifth symbol in a certain data set is `x`, then `px = 1/5 = 0.2` and should be encoded with `Lx = -log2(0.2) = 2.322...` bits.

There are multiple ways to adjust these bit lengths such that:
1. each bit length is an integer
2. the Kraft-McMillan inequality holds true
3. the code is close to optimal
4. (optional) fast execution time

I implementated two strategies, named A and B in this document.\
The code repository calls them `limitedKraft` and `limitedKraftHeap`

## Kraft / Strategy A

The [first](limitedkraft.c) strategy shares many concepts with [Charles Bloom's blog posting](http://cbloomrants.blogspot.com/2010/07/07-03-10-length-limitted-huffman-codes.html):
1. compute theoretical code length for each symbol
2. round to nearest integer
3. as long as the Kraft-McMillan inequality is violated, extend a few codes by one bit, thus reducing the Kraft sum
4. (optional) if step 3 "overshot" and the Kraft sum is below 1 then shrink a few codes by one bit

Charles avoided `log2` computation due to its performance implications.\
I found a fast `log2` approximation by [Laurent de Soras](https://www.flipcode.com/archives/Fast_log_Function.shtml).
His code is built on clever bit-twiddling tricks and about 7x faster than a call to the native `log2` function.\
Tweaking it for high accuracy at `.5` (while dramatically reducing accuracy on other fractions) made it even faster.\
(`.5` is a relevant threshold to decide whether to round up or down.)

I decided to have multiple iterations for step 3. The first iteration rounds up every symbol (= adds a bit) with a fraction between `.5 - 28/64` and `.5`.\
`28/64` was found to be a sweetspot in my tests but you may freely change that (`#define INITIAL_THRESHOLD`).\
Please note that due to the numerical inaccuracies of `fastlog2` the fraction is only approximately `28/64`.\
The next iteration lowers the threshold by `1/64` (see `#define STEP_THRESHOLD`).

If you increase those values then the algorithms runs faster at the cost of a slightly less efficient prefix code.

## Kraft / Strategy B

The [second](limitedkraftheap.c) strategy is built on the idea of a "gain".
If the theoretical code length is 2.32 bits and the rounded prefix code has just 2 bits then it "gained" 0.32 bits.

A max-heap is a fast way to sort all codes with the highest gain first.
Adjusted codes are re-inserted into the max-heap until the Kraft-McMillan inequality becomes true.
Similar to strategy A, there is a quick fix-up pass to shrink a few codes by a bit if the Kraft sum fell below 1.

Strategy B runs often about 3 times faster than strategy A but tends to have slightly less efficient prefix codes,
especially if the length limit gets close to the lower bound.
Strategy B outperforms Moffat's Huffman code generator in pretty much all practical use cases.


# Benchmark

The benchmark program computes a length-limited prefix code for a given histogram.\
By default it's the histogram of the first 64k of the [`enwik` data set](http://mattmahoney.net/dc/textdata.html).

The command-line syntax looks as follows:

`./benchmark ALGORITHM BITS [REPEAT] [HISTOGRAMFILE]`

Parameters:
* `ALGORITHM`
  * `1` - Package-Merge
  * `2` - MiniZ
  * `3` - JPEG
  * `4` - BZip2
  * `5` - Kraft
  * `6` - modified Kraft
  * `0` - "unlimited" Huffman codes / Moffat's in-place algorithm
* `BITS`
  * maximum number of bits per encoded symbol
  * if too low,  then it may fail
* `REPEAT` (optional parameter)
  * all algorithms are typically too fast to reliably measure execution time
  * therefore you can run the same algorithm multiple times
  * on my computer `100000` usually takes about a second
* `HISTOGRAMFILE` (optional parameter)
  * a text file with your own histogram, consisting of unsigned integers separated by a space
  * the [histogram.c](histogram.c) tool can create such a histogram
  * if the parameter is `-` then read from STDIN
  * if the parameter is omitted then switch to a pre-computed histogram of the first 64k of `enwik`

It's important to note that each iteration calls the function with the shared interface.
That means that, e.g. the JPEG length-limiting algorithm, sorts the symbol histogram each time instead of re-using it from a previous iteration.
In my eyes any other way would measure wrong execution time - the only reason `REPEAT` exists is that it's quite hard to time a single iteration.


# Results

Here are a few results from the first 64k bytes of `enwik`, measured on a Core i7 / GCC x64:

Syntax was:\
`time ./benchmark 0 12 100000`\
* where `0` is the algorithm's ID and was in `0`...`6`\
* each algorithm ran 100,000 times
* the unadjusted Huffman codes have up to 16 bits
* uncompressed data has 64k bytes = 524288 bits

algorithm      | ID | total size   | percentage | execution time
---------------|----|--------------|------------|----------------
Huffman        |  0 | 326,892 bits |   62.35%   |       0.54 s
Package-Merge  |  1 | 327,721 bits |   62.51%   |       1.17 s
MiniZ          |  2 | 328,456 bits |   62.65%   |       0.58 s
JPEG           |  3 | 328,456 bits |   62.65%   |       0.61 s
BZip2          |  4 | 328,887 bits |   62.73%   |       0.88 s
Kraft          |  5 | 327,895 bits |   62.54%   |       1.72 s
modified Kraft |  6 | 327,942 bits |   62.55%   |       0.41 s

The influence of length-limit (same data set, just showing percentage and execution time):

length limit | Package-Merge  | Kraft Strategy B
-------------|----------------|------------------
8 bits       | 70.47%, 0.96 s | 70.76%, 0.24 s
9 bits       | 65.30%, 1.02 s | 65.31%, 0.24 s
10 bits      | 63.49%, 1.07 s | 63.79%, 0.31 s
11 bits      | 62.80%, 1.14 s | 62.84%, 0.37 s
12 bits      | 62.51%, 1.17 s | 62.55%, 0.40 s
13 bits      | 62.40%, 1.22 s | 62.43%, 0.34 s
14 bits      | 62.36%, 1.25 s | 62.42%, 0.40 s
15 bits      | 62.35%, 1.29 s | 62.42%, 0.66 s
16 bits      | 62.35%, 1.35 s | 62.42%, 0.70 s
	
For comparison: Moffat's Huffman algorithm needs 0.55 seconds and its longest prefix code has 16 bits.

This data set was chosen pretty much at random (well, I knew `enwik` pretty well from my [smalLZ4](https://create.stephan-brumme.com/smallz4/) project).
I highly encourage you to collect some results for your own data sets with `./benchmark` (and `./histogram`).


# Limitations

* all algorithms are single-threaded
* if the convenience wrappers need to sort (histogram etc.) then it call C's `qsort` which might not be the fastest way to sort integers
* I haven't tested data sets with a huge number of symbols, however I doubt the actual need for more than 10^6 distinct symbols
* and heavily skewed/degenerated data sets were'nt analyzed as well
* in-depth code testing, such as fuzzying, wasn't done