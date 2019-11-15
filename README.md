# Some benchmarks on Floram

This is a fork of Jack Doerner's implementation of Floram. The original repository is [here](https://gitlab.com/neucrypt/floram).

The main difference is that this repository uses the Obliv-C with TLS. Such changes are aimed for doing some benchmark.

## Running benchmarks

We only focus on the following scripts, which are for benchmarking the write performance of three different types of ORAM implementations designed for RAM-model secure computation.

- Circuit ORAM: [tools/bench/circuit_access_4.sh](tools/bench/circuit_access_4.sh)
- Square-root ORAM: [tools/bench/sqrt_access_4.sh](tools/bench/sqrt_access_4.sh)
- Floram (with the CPRG technique): [tools/bench/fssl_cprg_access_4.sh](tools/bench/fssl_cprg_access_4.sh) 

In each of the scripts, there are three parameters for our adjustment.
```
BENCH_PROG_ARGS="-s 1024 -o fssl_cprg "
ELCTS=(1048576)
ITERS=(128)
```

Here, the `-s 1024` means that each block in the ORAM should have `1024 * 4B = 4KB`, `ELCTS=(1048576)` means that there are `1048576` blocks in the ORAM, and `ITERS=(128)`means that the benchmark writes `128` times to the ORAM after the initialization.

There are three remarks:

1. The script can *schedule multiple tasks*. Similar to the original Floram's script, one can put more than one pair of `ELCTS` and `ITERS`. The script will run the benchmark for each pair.

2. The script requires *network buffer size configuration*.
    - To run the script, one needs to provide parameter `y` and parameter `z`.
    - The value `y` is the first party's sending buffer size.
    - The value `z` is the second party's sending buffer size.
    - The code that handles this configuration is in [tests/test_main.c](tests/test_main.c).
    
3. To measure the average time for square-root ORAM and floram, one must set the `ITERS` properly since these two ORAM schemes have "cycles". See the comments in the script for how to set a proper iteration number.

## License
The original Floram implementation is under the 3-clause BSD license.

See the [LICENSE](LICENSE) file for more information.
