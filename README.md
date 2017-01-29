The Absentminded Crypto Kit
=====

This project seeks to develop a comprehensive Cryptographic SDK for use in secure multi-party computation. It supports a growing number of useful mathematical cryptographic primitives with efficient implementations based upon recent research, simplifying the development of complex applications in a secure multi-party context. In addition, it includes a number of useful non-cryptographic algorithms, such as Stable Matching and Breadth-first Search. It also includes a comprehensive set of test cases and benchmarks. The ACK library is written in [obliv-c](https://github.com/samee/obliv-c/), a c-derived language for secure multiparty computation. Primary development is undertaken by Jack Doerner. Thanks also to Samee Zahur and Xiao Wang for their contributions.


Features
=====

* Batcher Sorting and Merging - based upon _[Sorting Networks and their Applications](http://www.cs.kent.edu/~batcher/sort.pdf)_
* Big Integer math
	* Division via Algorithm D - based upon _The Art of Computer Programming_ Vol 2, Section 4.3.1 (Knuth, 1969)
	* Karatsuba-Comba Multiplication
	* Square Root via Newton's Method
* Binary Search
* Graph Algorithms
	* Breadth First Search (Naive Method)
* Hash Functions
	* Scrypt - based upon _[Stronger Key Derivation via Sequential Memory-hard Functions](https://www.tarsnap.com/scrypt/scrypt.pdf)_
	* SHA256 and SHA512
* Oblivious Data Structures
	* Queue - based upon _[Circuit Structures for Improving Efficiency of Security and Privacy Tools](http://www.ieee-security.org/TC/SP2013/papers/4977a493.pdf)_
* ORAM
	* Circuit ORAM - based upon _[Circuit Oram: On Tightness of the Goldreich-Ostrovsky Lower Bound](https://eprint.iacr.org/2014/672.pdf)_; code contributed by Xiao Wang
	* FSS Linear ORAM - based upon _Gigabyte-scale Scanning ORAMs for Secure Computation_ (forthcoming)
	* Linear Scan
	* Square Root ORAM - based upon _[Revisiting Square-Root ORAM: Efficient Random Access in Multi-Party Computation](https://oblivc.org/docs/sqoram.pdf)_; code contributed by Samee Zahur
* Stable Matching
	* Gale-Shapley - based upon _[Secure Stable Matching at Scale](http://oblivc.org/docs/matching.pdf)_
	* Roth-Peranson - based upon _[Secure Stable Matching at Scale](http://oblivc.org/docs/matching.pdf)_
* Symmetric Encryption
	* AES128 - based upon _[A Small Depth-16 Circuit for the AES S-Box](https://eprint.iacr.org/2011/332.pdf)_
	* ChaCha20 - based upon _[ChaCha, a Variant of Salsa20](https://cr.yp.to/chacha/chacha-20080128.pdf)_
	* Salsa20 - based upon _[The Salsa20 Family of Stream Ciphers](https://cr.yp.to/snuffle/salsafamily-20071225.pdf)_


Installing
=====

1. You must first build [obliv-c](https://github.com/samee/obliv-c/), though it need not be installed in any particular location. In addition to obliv-c, you will need to install the package `openssl-dev` (or your distribution's equivalent). This is necessary only for test cases for SHA256, SHA512, and AES.

2. To compile ACK, set the path to obliv-c's main project directory via `export OBLIVC_PATH=<path to obliv-c>`, then run `make`.


Project Organization
=====

Source for this project is divided into two directories: `src` contains code for the primary library, while `tests` contains code for tests and benchmarks. The library will be compiled to `build/lib/liback.a`, and all testing and benchmarking binaries are found in `build/tests`.