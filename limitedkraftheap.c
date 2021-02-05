// //////////////////////////////////////////////////////////
// limitedkraftheap.c
// written by Stephan Brumme, 2021
// see https://create.stephan-brumme.com/length-limited-prefix-codes/
//

#include "limitedkraftheap.h"
#include <stdlib.h> // malloc/free
#include <stdint.h> // int32_t


// related websites:
// https://cbloomrants.blogspot.com/2018/04/engel-coding-and-length-limited-huffman.html
// https://github.com/JoernEngel/joernblog/blob/master/engel_coding.md


// ----- a basic max-heap storing key/value pairs -----

typedef float        Key;
typedef unsigned int Value;

/// the heap is stored as two flat arrays (one for keys, one for values)
typedef struct
{
  Key*   keys;
  Value* values;
  size_t size;
} Heap;

/// comparison function (akin std::less)
static int heap_isLess(const Heap* heap, size_t posA, size_t posB)
{
  // max-heap: <
  // min-heap: >
  return (heap->keys[posA] < heap->keys[posB]) ? 1 : 0;
}

/// swap two heap items
static void heap_swap(Heap* heap, size_t posA, size_t posB)
{
  Key tmpKey         = heap->keys[posA];
  heap->keys[posA]   = heap->keys[posB];
  heap->keys[posB]   = tmpKey;

  Value tmpValue     = heap->values[posA];
  heap->values[posA] = heap->values[posB];
  heap->values[posB] = tmpValue;
}

/// move last item towards the beginning
static void heap_bubbleUp(Heap* heap)
{
  size_t current = heap->size;
  size_t parent  = (current - 1) / 2;
  while (current > 0 && heap_isLess(heap, parent, current))
  {
    heap_swap(heap, current, parent);
    current = parent;
    parent  = (parent - 1) / 2;
  }
}

/// move smaller items towards the end of the underlying array
static void heap_sinkDown(Heap* heap, size_t current)
{
  // iterate until current item is bigger than its children
  for (;;)
  {
    // assume the current item is already the biggest
    size_t biggest  = current;

    // compare to left  child
    size_t leftPos  = 2*current + 1;
    if (leftPos  < heap->size && heap_isLess(heap, current, leftPos))
      biggest = leftPos;

    // compare to right child
    size_t rightPos = 2*current + 2; // same as leftPos + 1
    if (rightPos < heap->size && heap_isLess(heap, biggest, rightPos))
      biggest = rightPos;

    // positioned at correct location ?
    if (biggest == current)
      return;

    // restore order
    heap_swap(heap, biggest, current);

    // continue with next item
    current = biggest;
  }
}

/// erase largest key (top of the heap) and its values
static void heap_removeTop(Heap* heap)
{
  // shrink by one
  heap->size--;

  // restore heap order
  if (heap->size > 0)
  {
    // move smallest item to the top (where usually the largest is found)
    heap->keys  [0] = heap->keys  [heap->size];
    heap->values[0] = heap->values[heap->size];

    // and let it sink down to its correct spot
    heap_sinkDown(heap, 0);
  }
}

/// add an item to the heap
static void heap_insert(Heap* heap, Key key, Value value)
{
  // append at the end
  heap->keys  [heap->size] = key;
  heap->values[heap->size] = value;

  // move it closer to the root if big enough
  heap_bubbleUp(heap);

  // and of course: heap grew by one
  heap->size++;
}

// ----- end of max-heap code -----



// ----- local helper function -----

// compute log(x), about 7x faster than C's log2 and no need for math.h / link -lm
static float fastlog2(float x)
{
  // see https://www.flipcode.com/archives/Fast_log_Function.shtml
  // posted by Laurent de Soras

  // error is 4.26 * 10^-3 according to http://www1.icsi.berkeley.edu/~fractor/papers/friedland_84.pdf
  // invalid input such as infinite or NaN leads to undefined results

  // float format: SIGN | EXPONENT | MANTISSA
  // where SIGN has  1 bit
  //       EXPONENT  8 bits
  //       MANTISSA 23 bits

#define FASTLOG2_SHIFT  23
#define FASTLOG2_MASK   ((1 << 8) - 1)
  // constants if x is double instead of float:
  // shift = 52 and mask = 0x7FF
  // (need to change all data types from float to double and int32_t to int64_t, too)

#define FASTLOG2_BIAS   (FASTLOG2_MASK >> 1)

  // map float and int to the same 32 bit memory location
  union
  {
    float f;
    int32_t i;
  } alias = { x };

  // get exponent
  int32_t exponent = alias.i >> FASTLOG2_SHIFT;
  exponent &= FASTLOG2_MASK; // exclude sign bit
  exponent -= FASTLOG2_BIAS; // exponent's bias is 127

  // now we already have the integer part
  float result = exponent;

  // let's set to exponent to 0 so that we get a number between 1 and 2
  // clear exponent's bits (due to bias that represents -127 now / that's the bias)
  alias.i &= ~(FASTLOG2_MASK << FASTLOG2_SHIFT);
  // add 127 to the exponent so that it's zero now
  alias.i +=   FASTLOG2_BIAS << FASTLOG2_SHIFT;

  // there are three options for the final step:

  // version A
  // fast approximation of log2(m) for m between 1 and 2
  // log2(x) ~ (x^2)/-3 + 2x - 5/3
  //result += (alias.f * -0.333333333f + 2) * alias.f - 1.666666667f;

  // for comparison:
  // https://www.wolframalpha.com/input/?i=%28x%2F-3%2B2%29x+-+5%2F3+between+1+and+2
  // https://www.wolframalpha.com/input/?i=log2%28x%29+between+1+and+2

  // version B
  // the average error of the following function is higher
  // but there's zero error at the important threshold f(1.5) = log2(1.5)
  //result += (alias.f * -0.33985f + 2.01955f) * alias.f - 1.6797f;
  // => it's the only quadratic curve passing through (1,0), (1.5,log2(1.5)) and (2,1)

  // version C
  // for our use case, even this simplified approximation works (zero error at f(1.5), too):
  result += 0.5849625f * alias.f; // log2(1.5) = 0.5849625
  //result += 0.389975f * alias.f;

#undef FASTLOG2_SHIFT
#undef FASTLOG2_MASK
#undef FASTLOG2_BIAS

  return result;
  // the code at https://github.com/romeric/fastapprox/blob/master/fastapprox/src/fastlog.h
  // seems to be marginally faster but I can't fully understand its magic
}


// ----- and now externally visible code -----


/// create prefix code lengths solely by optimizing the Kraft inequality
/**
 *  @param  maxLength  maximum code length, e.g. 15 for DEFLATE or JPEG
 *  @param  numCodes   number of codes, equals the array size of histogram and codeLength
 *  @param  histogram  how often each code/symbol was found
 *  @param  codeLength [out] computed code lengths
 *  @result actual maximum code length, 0 if error
 */
unsigned char limitedKraftHeap(unsigned char maxLength, unsigned int numCodes, const unsigned int histogram[], unsigned char codeLengths[])
{
  // my allround variable for various loops
  unsigned int i;

  // total number of symbols
  unsigned long long sumHistogram = 0;
  for (i = 0; i < numCodes; i++)
    sumHistogram += histogram[i];

  // 1/sumHistogram is needed multiple times lateron, let's replace division by multiplication
  float invSumHistogram = 1.0f / sumHistogram;

  // Kraft sum must not exceed 1
  // I try avoiding floating-point due to numerical instabilities
  // therefore instead of coping with 2^(-codeLength[i]) I switched to 2^(maxLength - codeLength[i])
  // which is always an integer >= 1
  unsigned long long one   = 1 << maxLength;
  // portion of "one" already consumed
  unsigned long long spent = 0;

  Heap heap;
  heap.size = 0;
  heap.keys   = malloc(numCodes * sizeof(float));
  heap.values = malloc(numCodes * sizeof(unsigned int));

  // start with rounded optimal code length
  for (i = 0; i < numCodes; i++)
  {
    // ignore unused
    if (histogram[i] == 0)
    {
      codeLengths[i] = 0;
      continue;
    }

    // compute theoretical number of bits
    float entropy = -fastlog2(histogram[i] * invSumHistogram);
    // and round to next integer
    unsigned char rounded = (unsigned char)(entropy + 0.5f);

    // at least one bit
    if (rounded == 0)
      rounded = 1;
    // and never more than the length limit
    if (rounded > maxLength)
      rounded = maxLength;

    // assign code length
    codeLengths[i] = rounded;
    // accumulate Kraft sum
    spent += one >> rounded;

    if (rounded < maxLength)
    {
      float gain = entropy - rounded;
      heap_insert(&heap, gain, i);
    }
  }

  // Kraft sum is most likely above 1 now, so we need to make a few codes one bit longer
  // to shrink the Kraft below 1

  // initially pick those codes that were "lucky" and rounded down
  // i.e. they got a shorter code

  // iterate until Kraft inequality is satisfied
  while (spent > one)
  {
    // extract code with largest gain (theoretical entropy minus code length)
    float     gain = heap.keys  [0];
    unsigned int i = heap.values[0];
    heap_removeTop(&heap);

    // all valid codes except those already at maximum length
    if (codeLengths[i] == 0 || codeLengths[i] >= maxLength)
      continue;

    // extend code by one more bit
    codeLengths[i]++;
    // reduce Kraft sum accordingly
    spent -= one >> codeLengths[i];
    // exit early if done
    if (spent <= one)
      break;

    // re-insert into the heap
    gain--;
    heap_insert(&heap, gain, i);
  }

  // optional: Kraft sum is below one, therefore a few codes might become shorter
  // this step can be skipped, we already have created a (suboptimal) prefix code
  while (spent < one && heap.size > 0)
  {
    // extract code with largest gain (theoretical entropy minus code length)
    unsigned int i = heap.values[0];
    heap_removeTop(&heap);

    // check if removing one bit still preserves Kraft inequality
    unsigned long long have = one >> codeLengths[i];
    if (one - spent >= have)
    {
      // yes, adjust this code
      codeLengths[i]--;
      spent += have;
    }
  }

  // don't be leaking ...
  free(heap.keys);
  free(heap.values);

  // find longest code
  unsigned char result = 0;
  for (i = 0; i < numCodes; i++)
    if (result < codeLengths[i])
    {
      result = codeLengths[i];
      if (result == maxLength)
        break;
    }

  return result;
}
