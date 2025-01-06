# BBHash vs Binary search
Benchmark for lookup in statically constructed BBHashMap vs binary search for relatively small sets (up to 10000 entries).

Measurement results for parameters 5000/1000 35 205
* 5000/1000 entries
* 35 full map iterations in different order
* repeated 205 times

| Algorithm | Time 5000 entries, us | Time 1000 entries, us |
|:-|:-:|:-:|
| BBhash | 7309 | 1370 |
| Bin Search | 11735 | 1890 |

BBhash is **1.3 times faster** for 1000 entries (1.6 for 5000). This is comes with a cost of increased memory size. However overhead is about 3 bits per key meaning additional 375 bytes per 1000 keys. This is negligible compared to 128000 bytes required to store keys and values themselves.