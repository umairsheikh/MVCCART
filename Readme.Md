┌──────────────────────────────────────────────────────────┐
│         MVCCART Concurrency Control Benefits             │
├──────────────────────────────────────────────────────────┤
│                                                          │
│ 1. LOCK-FREE READS                                       │
│    • No lock acquisition                                │
│    • No blocking of writers                            │
│    • Multiple readers concurrent                       │
│                                                          │
│ 2. NON-BLOCKING WRITES                                   │
│    • CAS instead of mutex                              │
│    • Restarts instead of waiting                       │
│    • Parallel writes on different nodes                │
│                                                          │
│ 3. MVCC ISOLATION                                        │
│    • Each transaction sees consistent snapshot         │
│    • Old versions remain readable                      │
│    • Writers don't invalidate readers                  │
│                                                          │
│ 4. HIGH THROUGHPUT                                       │
│    • Zero context switches for lock waits              │
│    • Optimistic validation > pessimistic locking       │
│    • Scales well with many concurrent threads          │
│                                                          │
└──────────────────────────────────────────────────────────┘


MVCCART Concurrency Control - Simplified Summary
1. Transactional Framework
Each transaction in MVCCART is a single independent thread that executes concurrently with other transactions. Every transaction receives a unique ID that is atomically generated, allowing the system to track and coordinate multiple concurrent operations across the database.

2. Readers: Lock-Free, Snapshot-Based
Readers in MVCCART never acquire any locks; instead they simply load a snapshot of the data at a specific version and validate it after reading. This means readers cannot block writers or other readers, and writers cannot block readers, creating a truly non-blocking read experience.

3. Writers: CAS-Based Lock Management
Writers use Compare-and-Swap (CAS) atomic operations instead of traditional mutexes to acquire locks on tree nodes. CAS is a non-blocking atomic instruction that either succeeds in acquiring the lock or fails immediately, signaling the writer to restart rather than wait.

4. Version Field Structure
Each ART tree node contains a 64-bit atomic version field that encodes the lock state, obsolete flag, and version counter in specific bit positions. The lock bit indicates whether a node is currently locked by a writer, while the version counter increments each time a write completes, allowing readers to detect if data changed during their read.

5. Successful Write Operation
A writer first reads the current version of a node, then attempts a CAS operation to set the lock bit by changing the version from current value to current+2 (0b10). If the CAS succeeds, the writer acquires the lock, modifies the node data, and then releases the lock by incrementing the version again.

6. CAS Failure and Restart
If another writer has already modified a node between the time the first writer read the version and attempted the CAS, the CAS operation fails immediately and returns false. When CAS fails, the transaction sets a restart flag and begins again from the start, reading the new version and attempting to acquire the lock again with the updated version value.

7. Reader vs Writer Interaction
When a reader and writer access the same node, the reader loads the version without acquiring any lock and begins reading data. If the writer modifies the node during the reader's operation, the reader detects the version change during validation and gracefully restarts, allowing both operations to proceed without any blocking or waiting.

8. Multi-Writer Parallelism
Different writers can simultaneously modify different nodes in the ART tree because each node has its own independent CAS lock. Writer-1 acquiring a lock on Node-A does not prevent Writer-2 from acquiring a lock on Node-B, resulting in true parallel write operations on separate parts of the tree structure.

9. MVCC Isolation Guarantee
MVCCART maintains multiple versions of data through a linked snapshot chain where each transaction sees a consistent version of the entire database. Old snapshots remain accessible and unmodified even as new versions are created by writers, allowing readers at any version to proceed without interference.

10. Non-Blocking Restart Strategy
Instead of using condition variables or mutex waits that put threads to sleep, failed transactions simply restart their operation with the new version information. This optimistic approach eliminates costly context switches and keeps all threads actively working, resulting in better CPU utilization and throughput.

11. Lock-Free Read Advantages
Because readers acquire no locks and only validate versions after reading, the system experiences zero lock contention between readers and writers or among multiple readers. This allows the system to sustain high concurrent read throughput even while writers are actively modifying the tree structure.

12. Comparison with Traditional Locking
Traditional databases use pessimistic locking where readers acquire shared locks that block writers, causing contention and context switches. MVCCART's optimistic lock-free approach with CAS and snapshot isolation achieves higher throughput and lower latency by allowing readers to proceed concurrently with writers without any blocking.

13. Epoch-Based Visibility Control
Transactions are organized into epochs to determine which versions are visible to which transactions. An epoch acts as a logical barrier that ensures transactions within an epoch see consistent snapshots, while epoch advancement allows garbage collection of old versions that are no longer needed by any active transaction.

14. High Concurrency Scalability
MVCCART's concurrency model scales well with increasing numbers of threads because there is no central lock that all threads must contend for. Each node has its own atomic version field, and readers use no locks at all, allowing the system to maintain high performance even with dozens of concurrent reader and writer threads.

15. Key Takeaway
MVCCART achieves lock-free, wait-free concurrency through a combination of atomic CAS operations for writers, snapshot-based reads with optimistic validation for readers, and multi-version data structures that allow transactions to operate on consistent snapshots. This design eliminates blocking between readers and writers while providing strong isolation guarantees required for transactional consistency.


Forked from http://github.com/armon/libart
C++ wrapping,  C++11 primitives, Boost SmartPointers, threads and tests were used and also pfabric "core/Tuple.hpp"

 Running Solution on multi-core machine cloud01.prakinf.tu-ilmenau.de cloud:

1-  SSH into server with credentials with login id:
    ssh -p 2222  yourlogin@cloud01.prakinf.tu-ilmenau.de
2-  Clone a new MVCCART repository from  Github:
    https://github.com/umairsheikh/MVCCART
3-  Please Delete cmake build files, i.e.. cmake-build-debug, CMakeFiles, cmake_install.cmake, CMakeCache.txt and
    I am sorry for it needed to be ignored, i will do it i promise.

4- Rename CMakeLists_Production to CMakeLists.txt , written for multicore boost environment for prakinf.tu-ilmenau awesome cloud.
  boost install path is hardcoded and should be set accordingly with your boost install path in a file  CMakeFile_Production.txt,
  which is to be renamed ofcourse to make it functional. (Original CMake is for the dev environment i have on MACOSX, It works
  fine and CMakes auto finds boost as well)

5- Update the CMakeLists.txt file, and choose the desired Test CPP file, to run from WorkloadTests or from CorrectnessTests
  folder relative path.

6- In your MVCCART path, and execute command -> Cmake .
- Compile the code through CMAKE using command:
    cmake --build MVCCART/ --target mvccart -- -j 4
- Running the solution  ./mvccart
  ERROR:: "Please set path in your WorkloadTests/...CPP file chosen" to run. I need to automate this too. Path should be
  absolute path to test_data/words.txt" file. Here it it to be updated in a variable "rootpath_words"



NOTE:: Project is a part of Pipefabric. It has only depency on pfabric "core/Tuple.hpp" referenced from ART/ArtCPP.hpp so far. It can be
very easy for pfabricers to setup this project with complete pfabric branch too. Just Make Pfabric branch, and add this repository
under Pfabric branch, and use "Core/Tuple.hpp" from the pfabric complete branch.  However, It can be directly imported
in Clion and readily be used without any extra headaches of pfabric as well, since a limited code from pfabric is already imported
in this repository and depencies were resolved for "core/Tuple.hpp"



Your own Development setup with your own ART Instances can be used as follows:

1- Add following headers, in you pfabric project. I

 #include <iostream>
 #include <boost/test/unit_test.hpp>
 #include <boost/lexical_cast.hpp>
 #include "mvcc/mvcc.hpp"
 #include "ART/ARTFULCpp.h"
 //#include "Transactions/WriteOnlyTemplates.h" Use templates already defined. from Transaction folder.

2- Define globals to test and reuse typdef is handy.

 typedef pfabric::Tuple<string,unsigned long, int,string, double> RecordType;
 typedef char KeyType[20];
 typedef ARTFULCpp<RecordType,KeyType> ARTTupleContainer;
 char KeysToStore[235890][20];
 std::vector<RecordType> vectorValues;


2.1- Tuple schema is defined like: typedef pfabric::Tuple<std::string,unsigned long, int, std::string, double> RecordType;

2.2- test_data/Words.txt file, is imported and all the keys are stored in an array as KeysToStore

2,3- Vector<RecordType> is created, as a process to test and store values, in order to compare the tuple values after
inserting to ART and updating the tuple attributes while updating operation. Here is how tuples are generated.
While index is the iterative index, while reading values from words.txt. Similar to HashMapTableTest.cpp.

RecordType tuple = RecordType(buf,
                             (unsigned long) index,
                             index + 100,
                             fmt::format("String/{}", buf),
                             index / 100.0);

vectorValues.push_back(tuple);


3- Adaptive Radix Tree instance can be used as such: While ARTTupleContainer is defined in globals.
auto ARTable1 =  new ARTTupleContainer();

5- Transactions are created and can be seen directly from here as specimen.
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/
https://github.com/umairsheikh/MVCCART/tree/master/CorrectnessTests
https://github.com/umairsheikh/MVCCART/blob/master/WorkloadTests/


Parallelism/Concurrency control:

Based on my analysis of the MVCCART repository, here's how write parallelism and concurrency are carried out:

Key Concurrency Mechanisms
1. Transaction-Based Parallelism
The system uses a transaction management framework where multiple writers run in parallel:

umairsheikh / MVCCART / Transactions / transactionManager.h
v1
template <typename TransactionFunc, typename ARTContainer>
class Transaction
{
public:
    size_t  Tid;
    boost::thread* TransactionThread;
Each transaction gets its own Thread ID (Tid) and runs in a separate boost::thread.

2. Optimistic Locking with Version Tracking
The Adaptive Radix Tree (ART) nodes use version-based optimistic locking:

umairsheikh / MVCCART / ART / ArtCPP.hpp
v1
typedef struct
{
    uint8_t type;
    uint8_t num_children;
    uint32_t partial_len;
    unsigned char partial[MAX_PREFIX_LEN];
64-bit atomic version field with:
2 bits for type
60 bits for version number
1 bit for lock state
1 bit for obsolete flag
3. Compare-and-Swap (CAS) Operations
Write operations use atomic CAS to upgrade locks without traditional mutexes:

umairsheikh / MVCCART / ART / ArtCPP.hpp
v2
bool upgradeToWriteLockOrRestart(art_node* node, uint64_t& v, bool &needRestart)
{
    if (!node->version.compare_exchange_strong(v, v + 0b10)) 
    {
        v = v + 0b10;
        needRestart = true;
Writers attempt to acquire locks via CAS; if it fails, the transaction restarts.

4. MVCC (Multi-Version Concurrency Control)
The system maintains multiple versions of data:

umairsheikh / MVCCART / mvcc / snapshot.hpp
v1
template <class ValueType>
struct snapshot
{
    using value_type = ValueType;
    
    smart_ptr::shared_ptr<snapshot> _older_snapshot;
Each write creates a new snapshot linked to older versions, allowing readers to access consistent older versions while writers create new ones.

5. Global Epoch Management
Transactions are organized into epochs for coordinated visibility:

umairsheikh / MVCCART / Transactions / Epochs.hpp
class Epoch
{
public:
    int EpochId;
    std::vector<size_t> TxnSet;
    int counter;
6. Atomic Transaction ID Generation
Writers get unique transaction IDs atomically without blocking:

umairsheikh / MVCCART / Transactions / transactionManager.h
v2
size_t get_new_transaction_ID()
{
    size_t Tid;
    nextTid.fetch_add(1, boost::memory_order_relaxed);
    Tid = nextTid.load(boost::memory_order_relaxed);
    return Tid;
Uses boost::atomic with relaxed memory ordering for high performance.

Summary
Write parallelism is achieved through:

Lock-free transactions using optimistic locking and version numbers
Compare-and-swap instead of blocking locks
Multi-versioning allowing concurrent readers and writers
Epoch-based isolation for transaction coordination
Boost thread pool for parallel execution
This approach allows multiple writers to proceed concurrently without blocking on traditional mutexes, using restarts when conflicts occur—a key pattern in high-performance concurrent data structures.



