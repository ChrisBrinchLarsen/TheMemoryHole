[Delete all lines covered in square brackets like this]
[Now specify how many levels of cache you intend to have]
3
[Then specify some name of a top level cache]
L1
[Next, specify a 'p' and a 'q', such that the formula 2^p * q gives the amount of bytes in the cache]
10
4
[Next, specify a 'k' in the formula 2^k for how many bytes are in a given block for this cache]
3
[Then specify the associativity]
8
[Then continue the last four instructions for each level of cache you want]
L2
10
16
4
16
L3
11
2
5
32

Then if we were to describe the following architecture:
On a 32bit address machine, with three caches L1, L2, L3.
The L1 cache has 4kb of space (2^10 * 4), has 8 bytes in each block (2^3), and is 8 way associative.
The L2 cache has 16kb of space (2^10 * 16), has 16 bytes in each block (2^4), and is 16 way associative.
The L3 cache has 2mb of space (2^20 * 2), has 32 bytes in each block (2^5), and is 32 way associative.
We could express this in an architecture file as:
3
L1
10
4
3
8
L2
10
16
4
16
L3
20
2
5
32