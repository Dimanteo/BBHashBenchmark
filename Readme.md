# BBHash vs Binary search
Benchmark for lookup in statically constructed BBHashMap vs binary search for relatively small sets (up to 10000 entries).

Measurement results for parameters 5000 35 205
* 5000 entries
* 35 full map iterations in different order
* repeated 205 times

| Algorithm | Time, us |
|:-|:-:|
| BBhash | 7309 |
| Bin Search | 11735 |

> BBhash is **1.6 times faster**. This is however comes with a cost of increased memory size. However overhead is about 3 bit per key meaning additional 375 bytes per 1000 keys. This is negligible compared to 128000 bytes required to store keys and values themselves.