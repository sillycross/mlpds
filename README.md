## High-Performance Data Structures via Memory Level Parallelism
Fun with data structures utilizing memory level parallelism (working in progress)

### Benchmark results

* 64-bit integer key. 

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|
|    Insert (20M)    | 5.39M op/s |      2.08M op/s     |     2.09M op/s    | 1.00M op/s |
|    Insert (80M)    | 5.06M op/s |      1.55M op/s     |     1.62M op/s    | 0.66M op/s |
|    Lookup (20M)    | 8.06M op/s |      3.68M op/s     |     2.57M op/s    | 1.01M op/s |
|    Lookup (80M)    | 7.46M op/s |      2.58M op/s     |     1.76M op/s    | 0.67M op/s |
|  Lower_Bound (20M) | 4.35M op/s |      3.32M op/s     |     No Support    | 0.96M op/s |
|  Lower_Bound (80M) | 4.06M op/s |      2.39M op/s     |     No Support    | 0.67M op/s |

※ For lookup, ~80% yield positive results.

* 64-bit integer key. Another data distribution.

|                    |   MlpSet   | [HOT Trie](https://dbis-informatik.uibk.ac.at/sites/default/files/2018-06/hot-height-optimized.pdf)<br> (SIGMOD18) | [ART Trie](https://db.in.tum.de/~leis/papers/ART.pdf)<br> (ICDE13) |  std::set  |
|--------------------|:----------:|:-------------------:|:-----------------:|:----------:|
|    Insert (20M)    | 3.70M op/s |      2.01M op/s     |     1.52M op/s    | 1.01M op/s |
|    Insert (80M)    | 3.60M op/s |      1.53M op/s     |     1.19M op/s    | 0.71M op/s |
|    Lookup (20M)    | 6.00M op/s |      3.95M op/s     |     1.68M op/s    | 0.93M op/s |
|    Lookup (80M)    | 5.83M op/s |      2.57M op/s     |     1.35M op/s    | 0.67M op/s |
|  Lower_Bound (20M) | 3.77M op/s |      3.44M op/s     |     No Support    | 0.94M op/s |
|  Lower_Bound (80M) | 3.56M op/s |      2.33M op/s     |     No Support    | 0.67M op/s |

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

