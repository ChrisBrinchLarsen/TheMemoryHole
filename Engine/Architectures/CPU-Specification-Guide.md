[Delete all lines covered in square brackets like this]
[First specify whether the system is 32-bit or 64-bit]
32
[Then specify how many bytes are in a word on this machine (1 would be a byte addressable machine)]
4
[Then specify how many words are in each block]
2
[Then specify some name of a top level cache]
L1
[Then specify the size in bytes of this level of cache]
1024
[Then specify the associativity]
8
[Then continue the last three instructions for each level of cache you want]
L2
2048
8
L3
4896
12

[As such, a final file might look something like this:
32
4
2
L1
1024
8
L2
2048
8
L3
4896
12
]