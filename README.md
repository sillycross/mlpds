## High-Performance Data Structures via Memory Level Parallelism
Fun with data structures utilizing memory level parallelism (working in progress)

### Benchmark results

* 64-bit integer key. "Lookup (16M)" means lookup on a set containing 16 million elements, etc.

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|
|    Insert (16M)    | 5.70M op/s |      2.14M op/s     |     1.97M op/s    | 1.03M op/s |
|    Insert (80M)    | 5.56M op/s |      1.57M op/s     |     1.65M op/s    | 0.71M op/s |
|    Lookup (16M)    | 9.03M op/s |      3.77M op/s     |     2.60M op/s    | 0.95M op/s |
|    Lookup (80M)    | 8.33M op/s |      2.64M op/s     |     2.00M op/s    | 0.67M op/s |
|  Lower_Bound (16M) | 4.68M op/s |      3.38M op/s     |     No Support    | 0.96M op/s |
|  Lower_Bound (80M) | 4.36M op/s |      2.44M op/s     |     No Support    | 0.67M op/s |

※ For lookup, ~80% yield positive results.

* 64-bit integer key. Another data distribution.

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|
|    Insert (16M)    | 4.15M op/s |      2.07M op/s     |     1.57M op/s    | 1.01M op/s |
|    Insert (80M)    | 4.16M op/s |      1.56M op/s     |     1.21M op/s    | 0.73M op/s |
|    Lookup (16M)    | 7.00M op/s |      4.00M op/s     |     1.71M op/s    | 0.95M op/s |
|    Lookup (80M)    | 6.85M op/s |      2.61M op/s     |     1.31M op/s    | 0.67M op/s |
|  Lower_Bound (16M) | 4.22M op/s |      3.50M op/s     |     No Support    | 0.94M op/s |
|  Lower_Bound (80M) | 4.01M op/s |      2.39M op/s     |     No Support    | 0.67M op/s |

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

* [**GoogleTest**](https://github.com/abseil/googletest)
  * Framework for running all tests and benchmarks in this project.
  
* **fasttime.h** (Author: MIT 6.172 Staff)
  * Utility header for time measurement.

