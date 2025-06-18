Checksum starts at 0

# Type Weight 10
Valid: 1
Invalid: 2
Dirty: 3
Clean: 4

+

# Cache Weight 100
The layer the cache is at (with L1 being 1)
If it's in an instruction cache, use layer 0

+

# Set Weight 1000
The set_index the operation is taking place at

+

# Line Weight 10000
The line_index the operation is taking place at


So then, in any one atomic action, you'd want to perform:
Checksum += Type * TypeWeight + CacheLayer * CacheWeight + SetIndex * SetWeight + LineIndex * LineWeight
(In C, the checksum is held in a uint64, so many sure the frontend uses mod MAXINT64)

The current value of the checksum should be added to the cache_log 