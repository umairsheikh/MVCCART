# MVCCART -- Lock-Free MVCC Adaptive Radix Tree

## Overview

MVCCART is a high-performance Adaptive Radix Tree (ART) enhanced with: -
Lock-free reads - CAS-based non-blocking writes - Multi-Version
Concurrency Control (MVCC) - Epoch-based isolation - High scalability on
multi-core systems

Forked from: https://github.com/armon/libart\
Repository: https://github.com/umairsheikh/MVCCART

------------------------------------------------------------------------

# Concurrency Control Benefits

## 1. Lock-Free Reads

-   No lock acquisition
-   No blocking of writers
-   Multiple readers concurrently

## 2. Non-Blocking Writes

-   CAS instead of mutex
-   Restarts instead of waiting
-   Parallel writes on different nodes

## 3. MVCC Isolation

-   Each transaction sees consistent snapshot
-   Old versions remain readable
-   Writers don't invalidate readers

## 4. High Throughput

-   Zero context switches for lock waits
-   Optimistic validation instead of pessimistic locking
-   Scales well with many concurrent threads

------------------------------------------------------------------------

# Architecture

## Transaction Model

-   Each transaction runs in its own thread
-   Unique transaction ID generated atomically
-   Fully concurrent execution

## Version Field Structure (64-bit)

-   Lock bit
-   Obsolete flag
-   Version counter
-   Type bits

## Write Protocol

1.  Read node version
2.  CAS to acquire write lock
3.  Modify node
4.  Increment version and release lock
5.  On CAS failure → restart transaction

## Reader Protocol

1.  Read version
2.  Read data
3.  Validate version
4.  If changed → restart

------------------------------------------------------------------------

# MVCC Snapshot Model

-   Each write creates a new snapshot
-   Snapshots form a linked chain
-   Readers always see a consistent version
-   Epoch-based garbage collection cleans unused versions

------------------------------------------------------------------------

# Building on TU Ilmenau Cloud

## SSH

ssh -p 2222 yourlogin@cloud01.prakinf.tu-ilmenau.de

## Clone

git clone https://github.com/umairsheikh/MVCCART

## Cleanup

Remove: - cmake-build-debug/ - CMakeFiles/ - cmake_install.cmake -
CMakeCache.txt

## Production CMake

Rename: CMakeLists_Production → CMakeLists.txt

Adjust Boost path inside file.

## Build

cmake . cmake --build MVCCART/ --target mvccart -- -j 4

## Run

./mvccart

If error: Set absolute path to test_data/words.txt in: rootpath_words
variable inside selected test file.

------------------------------------------------------------------------

# Using MVCCART in Your Project

## Headers

#include `<iostream>`{=html} #include \<boost/test/unit_test.hpp\>
#include \<boost/lexical_cast.hpp\> #include "mvcc/mvcc.hpp" #include
"ART/ARTFULCpp.h"

## Type Definitions

typedef pfabric::Tuple\<std::string,unsigned
long,int,std::string,double\> RecordType; typedef char KeyType\[20\];
typedef ARTFULCpp\<RecordType,KeyType\> ARTTupleContainer;

## Create Instance

auto ARTable1 = new ARTTupleContainer();

------------------------------------------------------------------------

# Concurrency Summary

MVCCART achieves parallelism using:

-   Optimistic transactions
-   Compare-and-Swap locking
-   Multi-version snapshot chains
-   Epoch coordination
-   Boost threads

It eliminates: - Blocking mutexes - Reader-writer contention - Context
switch overhead

While maintaining: - Strong isolation - High throughput - Multi-core
scalability
