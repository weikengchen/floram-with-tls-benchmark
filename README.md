Scaling ORAM for Secure Computation
=====

This repository provides code to accompany the paper Scaling ORAM for Secure Computation by Jack Doerner and abhi shelat, which appeared in the 24th ACM Conference on Computer and Communications Security (CCS) and is included here. This software is, in essence, a version of the [Absentminded Crypto Kit](https://bitbucket.org/jackdoerner/absentminded-crypto-kit) pared down until it contains only what is necessary to reproduce the results presented. As such, it takes the form of a library, along with a small number of testing and benchmarking applications. Further development of this software will take place in [the main ACK repository](https://bitbucket.org/jackdoerner/absentminded-crypto-kit), and it is recommended that anyone who wishes to use this library in their own applications should pull from there. The ACK library is written in [obliv-c](https://github.com/samee/obliv-c/), a c-derived language for secure multiparty computation.


Primary Features
=====
This distribution is intended to provide reference implementations of the Floram and Floram-CPRG Distributed ORAM schemes, which are described in the aforementioned paper. The implementations share all of their code excepting their FSS components (and the FSS components share an interface). Parallelism is provided via OpenMP, with an informal heuristic governing the number of threads based on the assumption that most CPU cores have about 1MB of cache. AES-NI hardware acceleration should be enabled automatically when compiled for an architecture with this insruction set available. Otherwise, Brian Gladman's a software AES implementation will be used (with some speed penalty). In addition to these ORAM implementations, this distribution contains code for a number of dependencies, alternatives, and applications, as listed in the following section.


Additional Features
=====

* Batcher Sorting and Merging - based upon _[Sorting Networks and their Applications](http://www.cs.kent.edu/~batcher/sort.pdf)_
* Binary Search
* Oblivious Data Structures
	* Queue - based upon _[Circuit Structures for Improving Efficiency of Security and Privacy Tools](http://www.ieee-security.org/TC/SP2013/papers/4977a493.pdf)_
* ORAM - a single interface providing the following RAM constructions
	* Circuit ORAM - based upon _[Circuit Oram: On Tightness of the Goldreich-Ostrovsky Lower Bound](https://eprint.iacr.org/2014/672.pdf)_; code contributed by Xiao Wang
	* Linear Scan
	* Square Root ORAM - based upon _[Revisiting Square-Root ORAM: Efficient Random Access in Multi-Party Computation](https://oblivc.org/docs/sqoram.pdf)_; code contributed by Samee Zahur
* Stable Matching
	* Gale-Shapley - based upon _[Secure Stable Matching at Scale](http://oblivc.org/docs/matching.pdf)_
	* Roth-Peranson - based upon _[Secure Stable Matching at Scale](http://oblivc.org/docs/matching.pdf)_
* Symmetric Encryption
	* AES128 - based upon _[A Small Depth-16 Circuit for the AES S-Box](https://eprint.iacr.org/2011/332.pdf)_


Installing
=====

1. You must first build [obliv-c](https://github.com/samee/obliv-c/), though it need not be installed in any particular location.

2. To compile ACK, set the path to obliv-c's main project directory via `export OBLIVC_PATH=<path to obliv-c>`, then run `make`.


Project Organization
=====

Source for this project is divided into two directories: `src` contains code for the primary library, while `tests` contains code for tests and benchmarks. The library will be compiled to `build/lib/liback.a`, and all testing and benchmarking binaries are found in `build/tests`.


Running Tests and Benchmarks Manually
=====

Each of our benchmarking and testing programs (that is, the binaries, not the benchmarking scripts described above) have individual options for adjusting the parameters relevant to that particular test or benchmark. These can be found by running the programs with the `-h` flag. In addition, there are a few standard parameters shared by all of the included programs, as well as the benchmarking scripts:

* `-h` prints a list of program-specific flags.
* `-p <number>` determines the port on which the program will listen (for servers) or connect (for clients). The default port is 54321.
* `-c <address>` instructs the program to run as a client, and connect to the server at `<address>`. By default, the program will run as a server.
* `-o <type>` forces the program to use `<type>` ORAMs. Valid types are `sqrt`, `linear`, and `circuit`.
* `-i <number>` (benchmarks only) instructs a benchmark to run for `<number>` iterations, and record results for all of them.


Building on this Work
=====

We encourage others to improve our work and to integrate it with their own applications. As such, we provide it under the 3-clause BSD license. For ease of integration, our code takes the form of a library, to which other software can link directly.