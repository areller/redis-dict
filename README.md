# Redis-Dict

A Redis module that exposes a new data type - **sorted dictionary**.

## Sorted Dictionary vs Sorted Set

The need for a sorted dictionary data type arises from the fact that in sorted sets,

* You can't store similar members with different scores.
* You can have multiple members with the same score.

Sorted dictionary is not the same as a sorted set. It acts like a hash set, but sorts itself by key.

## Implementation Details

Sorted dictionaries work by combining sorted sets and hash sets.  
Since sorted dictionaries are in essence, dictionaries, the key and values of the dictionary are stored in an hash set and to maintain the order of the keys, we store the keys in a sorted set.

## Commands

For now you can find usage of all commands in [test_rdict.py](./tests/test_rdict.py)