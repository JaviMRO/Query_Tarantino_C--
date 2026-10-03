# Benchmarks

How to run the data structure benchmarks of Stage 1 with the C++ implementation and build the comparison
report, step by step. What each structure is and why it is compared is in the
[README](README.md#data-structure-comparisons); how every metric is measured, and the rules that keep results
comparable across languages, are in [SPEC.md](SPEC.md#11-benchmarks).

Every command runs from the **project root**.

## Quick reference

```bash
scripts/fill_cache.sh                                       # 1. dataset, once
cmake --preset release-mongo                                # 2. build
cmake --build --preset release-mongo
TARANTINO_BENCHMARK_QUICK=true TARANTINO_BENCHMARK_BOOKS=20 \
    cmake --build --preset release-mongo --target benchmarks  # 3. check the setup in a few minutes
cmake --build --preset release-mongo --target benchmarks    # 4. full run (Linux: systemd-inhibit --what=idle:sleep, macOS: caffeinate -i)
python3 scripts/compare_results.py                          # 5. comparison report
```

The report is `benchmarks/report/comparison.md`.

## 1. Prepare the machine

- A C++20 compiler, CMake 3.21+ and Python 3.10+, as in the [README](README.md#requirements). The presets
  build in release mode (`-O2 -DNDEBUG`), as SPEC §11 requires; a debug build says so in
  `benchmarks/<service>/cpp-environment.txt`, and its results are not comparable.
- **MongoDB 7.0** running on the machine itself, not inside a virtual machine, with its cache fixed at 1 GB
  (SPEC §11). The [README](README.md#requirements) shows how to install it and set the cache on Linux,
  macOS and Windows. Stop the development container first (`docker compose down`) if it is not the one you
  set up, since both use port 27017. To run without MongoDB, set `TARANTINO_BENCHMARK_SKIP_MONGO=true` or
  build with the `release` preset, which has no MongoDB support unless the driver is installed (see
  [Variables](#variables)).
- **matplotlib**, optionally, to add charts to the report: `python3 -m pip install matplotlib`.
- **Linux or macOS.** SPEC §11 runs the benchmarks on Unix systems, Linux being the reference. They also run
  on Windows, but there the file system and the antivirus dominate every benchmark that writes or reads many
  files (see [Notes](#notes-for-comparing-languages)).

## 2. Fill the dataset cache

Benchmarks read the books from a local cache so the network is never measured. Fill it once; it can be
interrupted and resumes where it stopped:

```bash
scripts/fill_cache.sh
```

It follows `workload/book_ids.txt` until `benchmarks/cache/` holds 2 800 books with Gutenberg markers (ids
missing or without them are listed in `benchmarks/cache/skipped.txt`). By default it copies the candidates
from `rsync.ibiblio.org::gutenberg-epub` in one rsync transfer, into `benchmarks/mirror/`, and moves the valid
ones to the cache. Where the rsync port (873) is blocked, download them over HTTP from the official mirror
instead:

```bash
TARANTINO_CACHE_SOURCE=http scripts/fill_cache.sh
```

| Variable                   | Effect                                                              |
|----------------------------|---------------------------------------------------------------------|
| `TARANTINO_CACHE_BOOKS`    | Books to cache (default 2800)                                       |
| `TARANTINO_CACHE_SOURCE`   | `rsync` (default) or `http`                                         |
| `TARANTINO_RSYNC_SOURCE`   | rsync module (default `rsync.ibiblio.org::gutenberg-epub`)          |
| `TARANTINO_CACHE_PARALLEL` | Simultaneous HTTP downloads (default 16)                            |

The dataset is about 1.3 GB (2 800 books of 470 KB on average). The cache is the same for every language: a
cache filled for the Java or Python implementation serves as it is. The benchmarks need at least the first
1 100 books (the dataset of 1 000 and the 100 new books of the update and detection benchmarks).

On Windows run the script from Git Bash or WSL.

## 3. Build

```bash
cmake --preset release-mongo
cmake --build --preset release-mongo
```

This builds the services, their tests and one benchmark executable per service: `crawler_benchmarks`,
`indexer_benchmarks` and `query_benchmarks`, in `build/release-mongo/bin/`. The first build also downloads
and builds the MongoDB C++ driver, which takes a few minutes. Without MongoDB, use the `release` preset and
`build/release/` everywhere below. Build again after changing any code, but never while a benchmark is
running: it replaces the executables the run is using.

## 4. Check the setup with a quick run

Before a full run, check that the dataset, MongoDB and every benchmark work. A quick run takes 1 process, 1
warm-up and 1 measured iteration per configuration, so its numbers are not meaningful, only its errors:

```bash
TARANTINO_BENCHMARK_QUICK=true TARANTINO_BENCHMARK_BOOKS=20 cmake --build --preset release-mongo --target benchmarks
```

It overwrites the results of an earlier run in `benchmarks/results/`; keep a copy of them if you need them.
With 100 books it took 1 minute on Linux (an Intel Core Ultra 9 275HX laptop under WSL 2, measured), MongoDB
structures included.

## 5. Run the benchmarks

```bash
cmake --build --preset release-mongo --target benchmarks    # every service: crawler, indexer, query
build/release-mongo/bin/indexer_benchmarks                  # a single service
```

The `benchmarks` target runs the three executables one after the other from the project root, and stops at
the first that fails. Keep the machine as SPEC §11 requires: plugged in, out of any power-saving mode, with
nothing else running, and awake for the whole run. Wrap the command so the system does not sleep until it
ends:

```bash
systemd-inhibit --what=idle:sleep cmake --build --preset release-mongo --target benchmarks   # Linux
caffeinate -i cmake --build --preset release-mongo --target benchmarks                       # macOS
```

Laptops, above all fanless ones, lower their clock speed under sustained load, so a cool machine on a hard
surface gives steadier results. The scratch files go under `benchmarks/tmp.noindex/`, which Spotlight skips
on macOS; on a Linux desktop with KDE Baloo, which indexes the whole home directory, exclude `benchmarks/` in
its settings.

Each service runs its benchmarks in **two passes** of one process per configuration (SPEC §11); the second
runs every parameter in reverse order, so whatever drifts during a run, such as the temperature, weighs alike
on every structure and size. A service writes its results file only when both passes end: an interrupted
service writes nothing, and running it again, alone, replaces only its own results. A benchmark whose
structure gives a wrong result (a term count, a book not found, a query answered differently from the books
themselves) fails the run, which then writes no results.

Most of the time goes to building and updating the `folders` index, one file per term, so it depends mostly on
the disk. With 100 books only (`TARANTINO_BENCHMARK_BOOKS=100`) a full run of the three services took about 6
minutes on the machine above (measured: 1 minute 43 seconds for the crawler, 2 minutes 11 for the indexer
and 2 minutes for the query service).

| Comparison               | Structures                 | Metrics                                                                  | Benchmarks                                                          |
|--------------------------|----------------------------|--------------------------------------------------------------------------|---------------------------------------------------------------------|
| Datalake (PDF 3.1)       | `time`, `book`, `batch`    | write throughput, lookup, new books detection, recovery and leftover files, files, disk | crawler `DatalakeWriteBenchmark`, `DatalakeLookupBenchmark`, `NewBooksDetectionBenchmark`, `RecoveryScenario` |
| Inverted index (PDF 4.2) | `json`, `folders`, `mongo` | full build, incremental update book by book and in batch, index open, query, build and index memory, allocations, terms, disk | indexer `FullIndexBuildBenchmark`, `IncrementalUpdateBenchmark`, query `IndexOpenBenchmark`, `QueryTimeBenchmark` |
| Metadata (PDF 4.1)       | `sqlite`, `mongo`          | bulk insertion, book by id, books by author                              | indexer `MetadataInsertionBenchmark`, query `MetadataQueryBenchmark` |

### Variables

| Variable                         | Effect                                                        |
|----------------------------------|---------------------------------------------------------------|
| `TARANTINO_BENCHMARK_BOOKS`      | Sizes to run, e.g. `100,300` (default `100,300,1000`)          |
| `TARANTINO_BENCHMARK_QUICK`      | `true`: 1 pass of 1 process, 1 warm-up and 1 measured iteration, to check the setup |
| `TARANTINO_BENCHMARK_SKIP_MONGO` | `true`: skip the `mongo` index and metadata structures        |
| `TARANTINO_MONGO_URI`            | MongoDB server (default `mongodb://localhost:27017`)          |
| `TARANTINO_BENCHMARKS`           | Directory of the cache, results and report (default `benchmarks`) |
| `TARANTINO_WORKLOAD`             | Directory of the workload files (default `workload`)          |

## 6. Build the comparison report

```bash
python3 scripts/compare_results.py
```

It prints `Comparison written to .../benchmarks/report/comparison.md`. It reads every CSV in
`benchmarks/results/` and writes, for each comparison (datalake, inverted index, metadata), the best structure
of each metric, one table per metric with the mean ± the half-width of its 95% confidence interval, and one
chart per metric if matplotlib is installed.

**Comparing languages.** Copy the `java-*.csv` and `python-*.csv` results of the other implementations, run
on the same machine, into `benchmarks/results/` and run the script again: every table then has a row per
language and structure, and the winners are chosen per language.

## Output files

Everything is under `benchmarks/`, which is not versioned:

| Path                                         | Content                                                        |
|----------------------------------------------|----------------------------------------------------------------|
| `cache/<id>.txt`, `cache/skipped.txt`        | The dataset, and the candidate ids left out of it              |
| `mirror/`                                    | rsync copy of the candidates, before they move to the cache    |
| `results/cpp-<service>.csv`                  | Results shared with the other languages (SPEC §11 format)      |
| `<service>/cpp-results-pass-<1\|2>.csv`      | Every measured iteration of each pass, to compare pass 1 against pass 2 |
| `<service>/cpp-environment.txt`              | Operating system, processor, memory, compiler, build type, Unicode and library versions (SPEC §11) |
| `report/comparison.md`, `report/*.png`       | The comparison report and its charts                           |
| `tmp.noindex/`                               | Scratch datalakes and indexes, deleted and rebuilt by each run |

The results files have one row per structure, metric and size:

```
language,structure,metric,n_books,value,error,unit
cpp,json,query_time,100,4.217,0.047,µs/query
```

`error` is the half-width of the 95% confidence interval, `0` for exact values such as file counts, and empty
when there is a single sample (a quick run).

## Notes for comparing languages

- **Harness.** JMH does not exist for C++, so each benchmark executable is its own harness with the scheme of
  SPEC §11: it starts itself once per benchmark, structure, size and pass (`fork`/`exec`, `CreateProcess` on
  Windows), times with `std::chrono::steady_clock`, keeps every result observable through a `volatile` sink,
  and draws random books, authors and queries with `std::mt19937_64`.
- **Memory.** `memory_allocated` counts the bytes requested from the global `operator new`, which the benchmark
  executables replace; what the C libraries (ICU, SQLite, libcurl, the MongoDB C driver) allocate with `malloc`
  is not counted, as Java counts only its own heap. `build_memory` and `index_memory` are the bytes the
  allocator has in use, with no collection needed: `mallinfo2` on Linux, `malloc_zone_statistics` on macOS
  and the busy blocks of the process heaps on Windows.
- **Paths.** libstdc++, the standard library of GCC and the usual one on Linux, splits a `std::filesystem::path`
  into its components whenever one is built, which Java's `Path` does not. The datalake builds the paths of a
  book from text once and lists directories by name to keep that cost down, but a lookup by id with the `book`
  and `batch` layouts, which is little more than building two paths and checking that the body exists, still
  took about 0.6 µs in C++ against 0.4 µs in Java on Linux on the development machine (quick comparison, not a
  full run), while new books detection and the `time` lookup took about half as long in C++. libc++, the
  standard library of macOS, builds paths without splitting them.
- **Windows.** The benchmarks run there, but they are not comparable with a Unix run, and possibly not even
  across languages on the same Windows machine. With Microsoft Defender on, the first time a process opens a
  file to read it cost about 0.3 ms more on the development machine, for the C++ executables and Python alike
  but not for `java.exe`, which the antivirus apparently excludes there. So the `folders` updates, which read
  every term file of the new books, took 25 to 55% longer in C++ than in Java, about what that cost adds up
  to, while creating files cost the same in both. Exclude `benchmarks/` from real-time scanning, which applies
  to every process alike, or better, run on Linux or macOS as SPEC §11 requires. The Java indexer and query
  benchmarks do not finish on Windows: they delete SQLite databases that are still open, which Windows does
  not allow.
