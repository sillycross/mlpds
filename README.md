## High-Performance Data Structures via Memory Level Parallelism
Fun with data structures utilizing memory level parallelism (working in progress)

### Benchmark results

* The benchmarks are run on my laptop, with Intel Core i7-7700HQ CPU @ 2.80GHz (4 Core 8 Hyperthreads, although all benchmarks are single-threaded), and 2x16GB Corsair DDR4 2400MT DRAM. I measured that one DRAM round-trip takes 70 ns (14.3M op/s) when the system is in idle state.

* In this benchmark, we test 64-bit integer keys. "Lookup (16M)" means lookup on a set containing 16 million elements, etc.

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |  DenseHashSet<br> (Google) | [EBS](https://github.com/patmorin/arraylayout)<br> (JEA17) |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|:----------------:|:------------:|
|    Insert (16M)    | 6.57M op/s |      2.14M op/s     |     1.97M op/s    | 1.03M op/s |    Not Tested    |  Impossible  |
|    Insert (80M)    | 6.80M op/s |      1.57M op/s     |     1.65M op/s    | 0.71M op/s |   10.44M op/s    |  Impossible  |
|    Lookup (16M)    | 9.06M op/s |      3.77M op/s     |     2.60M op/s    | 0.95M op/s |    Not Tested    |  Not Tested  |
|    Lookup (80M)    | 9.03M op/s |      2.64M op/s     |     2.00M op/s    | 0.67M op/s |   12.50M op/s    |  3.72M op/s  |
|  Lower_Bound (16M) | 5.28M op/s |      3.38M op/s     |     No Support    | 0.96M op/s |    Impossible    |  Not Tested  |
|  Lower_Bound (80M) | 5.06M op/s |      2.44M op/s     |     No Support    | 0.67M op/s |    Impossible    |  3.70M op/s  |

※ What do the numbers mean? MlpSet is only taking ~2.1, ~1.6, ~2.8 DRAM roundtrip time respectively, to perform a Insert, Lookup, Lower_Bound operation on a set of 80 million elements. Note that even if you could know the address where the answer is stored for free, reading it will also need 1 DRAM roundtrip!

※ Meanwhile, unlike most of the known techniques to leverage MLP, which relies on batching/software pipelining multiple queries and can only process batched-read-only workloads, MlpSet does not make any of such assumptions. MlpSet can handle any mixed workload and does not require any "batching".

※ Actually, in the benchmarks, each query's text is XOR'ed with the previous query's correct answer. This means even the CPU cannot out-of-order execute across query boundaries, since it cannot know what the next query is until it correctly answers the previous one. MlpSet will be (slightly) even faster if this "forced boundary" is removed, but we believe this better reflects the reality where queries come in one by one.

* Still 64-bit integer key, but another data distribution. This is a harder distribution for us (the total number of trie tree nodes is closer to the theoretical upper bound of 2n), but an easier distribution for HOT. However, even in this case, on 80M data set, we are still having 3x advantage on Insert and Lookup, and 2x advantage on Lower_Bound over HOT. 

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |  DenseHashSet<br> (Google) | [EBS](https://github.com/patmorin/arraylayout)<br> (JEA17) |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|:----------------:|:------------:|
|    Insert (16M)    | 4.80M op/s |      2.07M op/s     |     1.57M op/s    | 1.01M op/s |    Not Tested    |  Impossible  |
|    Insert (80M)    | 4.93M op/s |      1.56M op/s     |     1.21M op/s    | 0.73M op/s |   10.37M op/s    |  Impossible  |
|    Lookup (16M)    | 7.46M op/s |      4.00M op/s     |     1.71M op/s    | 0.95M op/s |    Not Tested    |  Not Tested  |
|    Lookup (80M)    | 7.40M op/s |      2.61M op/s     |     1.31M op/s    | 0.67M op/s |   12.34M op/s    |  3.74M op/s  |
|  Lower_Bound (16M) | 4.79M op/s |      3.50M op/s     |     No Support    | 0.94M op/s |    Impossible    |  Not Tested  |
|  Lower_Bound (80M) | 4.69M op/s |      2.39M op/s     |     No Support    | 0.67M op/s |    Impossible    |  3.72M op/s  |

※ In both benchmarks, ~80% lookup yield positive results.

※ For, dense\_hash\_set, load factor is 60%, and huge page is used. Without huge page, the performance is about 8M op/s for both insert and lookup. 

* more to come...

### List of third-party libraries used in this project

* [**xxHash**](https://github.com/Cyan4973/xxHash) ([Author](https://github.com/Cyan4973))
  * A slightly modified version of XXH32 is used in MlpSet.

* [**HOT Trie**](https://github.com/speedskater/hot) ([Author](https://github.com/speedskater), [Paper](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf))
  * HOT: A Height Optimized Trie Index for Main-Memory Database Systems, R. Binna et al, SIGMOD 2018. 
  * Used as a benchmark rival.
  
* [**ART Trie**](https://github.com/armon/libart) ([Author](https://github.com/armon), [Paper](https://db.in.tum.de/~leis/papers/ART.pdf))
  * The Adaptive Radix Tree: ARTful Indexing for Main-Memory Databases, V. Leis et al, ICDE 2013.
  * Used as a benchmark rival.

* [**dense_hash_set**](https://github.com/sparsehash/sparsehash) 
  * Google dense\_hash\_set library.
  * Used as a benchmark baseline to show how well a hash table could do on insert and lookup.
  
* [**EBS**](https://github.com/patmorin/arraylayout)([Author](https://github.com/patmorin), [Paper](https://arxiv.org/pdf/1509.05053.pdf))
  * An extensively optimized binary search algorithm. 
  * Used as a benchmark baseline to show how well one could support lower_bound on a static data set via the binary search idea.
  * Among the various binary-search-based algorithms provided in the repository, 
    we used the one that is emperically fastest on our machine: 16-way implicit B-tree Eytzinger layout
    along with various low-level optimizations including conditional moves, prefetches (the "btree16_a" variant). 
  
* [**GoogleTest**](https://github.com/abseil/googletest)
  * Framework for running all tests and benchmarks in this project.

* **fasttime.h** (Author: MIT 6.172 Staff)
  * Utility header for time measurement.

