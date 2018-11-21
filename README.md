## High-Performance Data Structures via Memory Level Parallelism
Fun with data structures utilizing memory level parallelism (working in progress)

### Benchmark results

* 64-bit integers, 16M insertion followed by 20M existence query (~80% yield positive results)
  * **MlpSet**: 3.70M insertion/sec, 6.00M query/sec
  * **libart**: 1.52M insertion/sec, 1.68M query/sec
  * **std::set**: 1.01M insertion/sec, 0.93M query/sec

* Same setup as above, but 80M data set 
  * **MlpSet**: 3.60M insertion/sec, 5.83M query/sec
  * **libart**: 1.19M insertion/sec, 1.35M query/sec
  * **std::set**: 0.71M insertion/sec, 0.67M query/sec

* more to come...

### List of third-party libraries used in this project

* [**xxHash**](https://github.com/Cyan4973/xxHash) ([Author](https://github.com/Cyan4973))
  * A slightly modified version of XXH32 is used in MlpSet.

* [**libart**](https://github.com/armon/libart) ([Author](https://github.com/armon), [Paper](https://db.in.tum.de/~leis/papers/ART.pdf))
  * As a benchmark rival.
  
* [**GoogleTest**](https://github.com/abseil/googletest)
  * Framework for running all tests and benchmarks in this project.
  
* **fasttime.h** (Author: MIT 6.172 Staff)
  * Utility header for time measurement.

